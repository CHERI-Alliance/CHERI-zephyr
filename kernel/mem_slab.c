/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>
/* private kernel APIs */
#include <ksched.h>
#include <wait_q.h>

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
static struct k_obj_type obj_type_mem_slab;

#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB

static int k_mem_slab_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	__ASSERT((obj_core != NULL) && (stats != NULL), "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);
	memcpy(stats, &slab->info, sizeof(slab->info));
	k_spin_unlock(&slab->lock, key);

	return 0;
}

static int k_mem_slab_stats_query(struct k_obj_core *obj_core, void *stats)
{
	__ASSERT((obj_core != NULL) && (stats != NULL), "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;
	struct sys_memory_stats *ptr = stats;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);
	ptr->free_bytes = (slab->info.num_blocks - slab->info.num_used) *
			  slab->info.block_size;
	ptr->allocated_bytes = slab->info.num_used * slab->info.block_size;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	ptr->max_allocated_bytes = slab->info.max_used * slab->info.block_size;
#else
	ptr->max_allocated_bytes = 0;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */
	k_spin_unlock(&slab->lock, key);

	return 0;
}

static int k_mem_slab_stats_reset(struct k_obj_core *obj_core)
{
	__ASSERT(obj_core != NULL, "NULL parameter");

	struct k_mem_slab *slab;
	k_spinlock_key_t   key;

	slab = CONTAINER_OF(obj_core, struct k_mem_slab, obj_core);
	key = k_spin_lock(&slab->lock);

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->info.max_used = slab->info.num_used;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

	k_spin_unlock(&slab->lock, key);

	return 0;
}

static struct k_obj_core_stats_desc mem_slab_stats_desc = {
	.raw_size = sizeof(struct k_mem_slab_info),
	.query_size = sizeof(struct sys_memory_stats),
	.raw   = k_mem_slab_stats_raw,
	.query = k_mem_slab_stats_query,
	.reset = k_mem_slab_stats_reset,
	.disable = NULL,
	.enable = NULL,
};
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */

/**
 * @brief Initialize kernel memory slab subsystem.
 *
 * Perform any initialization of memory slabs that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each slab.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p slab contains invalid configuration and/or values.
 */
static int create_free_list(struct k_mem_slab *slab)
{
	char *p;

	/* blocks must be word aligned */
	CHECKIF(((slab->info.block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0U) {
		return -EINVAL;
	}

	slab->free_list = NULL;
	p = slab->buffer + slab->info.block_size * (slab->info.num_blocks - 1);

#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * For CHERI, tighten the bounds of a single block to info.block_size
	 * we use exact bounds here because we should have already aligned and
	 * rounded up the block size to a CHERI representable length
	 * during setup / static allocation. This means it will trap if
	 * it can't set the bounds exactly.
	 */
	uintptr_t p_addr;
	char *blk;

	while (p >= slab->buffer) {
		*(char **)p = slab->free_list;

		/* p points to highest block first - get this address */
		p_addr = __builtin_cheri_address_get(p);
		/* set a new cap with this as its base */
		blk = __builtin_cheri_address_set(p, p_addr);
		/* tighten the bounds to the length of a block only */
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
		/* relax internal bounds of a blk */
		/* Use with caution! - this may create overlap of bounds between blocks */
		blk = __builtin_cheri_bounds_set(blk, (size_t)slab->info.block_size);
#else
		/* set strict bounds - a trap will occur if the bounds can't be set exactly */
		blk = __builtin_cheri_bounds_set_exact(blk, (size_t)slab->info.block_size);
#endif	/* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
		/* store cap in the free list */
		slab->free_list = blk;
		p -= slab->info.block_size;
	}
	return 0;
#else
	while (p >= slab->buffer) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p -= slab->info.block_size;
	}
	return 0;
#endif
}

/**
 * @brief Complete initialization of statically defined memory slabs.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return 0 on success, fails otherwise.
 */
static int init_mem_slab_obj_core_list(void)
{
	int rc = 0;

	/* Initialize mem_slab object type */

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
	z_obj_type_init(&obj_type_mem_slab, K_OBJ_TYPE_MEM_SLAB_ID,
			offsetof(struct k_mem_slab, obj_core));
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
	k_obj_type_stats_init(&obj_type_mem_slab, &mem_slab_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */

	/* Initialize statically defined mem_slabs */

	STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		rc = create_free_list(slab);
		if (rc < 0) {
			goto out;
		}
		k_object_init(slab);

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
		k_obj_core_init_and_link(K_OBJ_CORE(slab), &obj_type_mem_slab);
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
		k_obj_core_stats_register(K_OBJ_CORE(slab), &slab->info,
					  sizeof(struct k_mem_slab_info));
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */
	}

out:
	return rc;
}

SYS_INIT(init_mem_slab_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc;

#ifdef __CHERI_PURE_CAPABILITY__
	/* For CHERI the block size needs rounding to its representable length for exact bounds */
	size_t block_size_rep;
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
	/*
	 * relax internal bounds of a blk - this allows overlap of bounds between blocks
	 * to maintain block size and the number of blocks asked for. This issue is observed
	 * when requested block sizes require rounding upwards to CHERI representable lengths
	 * and then do not fit within the slab size so an error is returned.
	 * Use with caution! - this may create overlap of bounds between blocks
	 */
	block_size_rep = block_size;
#else
	/* round block size to its representable length for exact bounds */
	block_size_rep = __builtin_cheri_round_representable_length(block_size);
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */

	/*
	 * check the number of blocks can fit into the slab size created after
	 * CHERI block rounding. Here we can either return an error if it
	 * doesn't fit forcing the caller to supply block/slab sizes that work,
	 * or we can reduce the number of blocks available.
	 */
	uint32_t num_blocks_int = num_blocks;
	size_t slab_len = __builtin_cheri_length_get(buffer);
#ifdef CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE
	/*
	 * If we want to maintain exact CHERI bounds per block after representable rounding
	 * and not want to return an error we can reduce the number of available blocks
	 * to fit within the bounds of the slab. The caller will need to check the number
	 * available and not assume the number available is what was requested.
	 */
	while (slab_len < block_size_rep*num_blocks_int) {
		num_blocks_int = num_blocks_int-1;
	}
#else
	/*
	 * otherwise if the number of blocks do not fit in the slab size after CHERI rounding
	 * return an error
	 */
	CHECKIF(slab_len < block_size_rep*num_blocks_int) {
		return -EINVAL;
	}
#endif /* CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE */
	slab->info.num_blocks = num_blocks_int;
	slab->info.block_size = block_size_rep;
	slab->buffer = buffer;
	slab->info.num_used = 0U;
	slab->lock = (struct k_spinlock) {};
#else
	slab->info.num_blocks = num_blocks;
	slab->info.block_size = block_size;
	slab->buffer = buffer;
	slab->info.num_used = 0U;
	slab->lock = (struct k_spinlock) {};
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	slab->info.max_used = 0U;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}

#ifdef CONFIG_OBJ_CORE_MEM_SLAB
	k_obj_core_init_and_link(K_OBJ_CORE(slab), &obj_type_mem_slab);
#endif /* CONFIG_OBJ_CORE_MEM_SLAB */
#ifdef CONFIG_OBJ_CORE_STATS_MEM_SLAB
	k_obj_core_stats_register(K_OBJ_CORE(slab), &slab->info,
				  sizeof(struct k_mem_slab_info));
#endif /* CONFIG_OBJ_CORE_STATS_MEM_SLAB */

	z_waitq_init(&slab->wait_q);
	k_object_init(slab);
out:
	SYS_PORT_TRACING_OBJ_INIT(k_mem_slab, slab, rc);

	return rc;
}

static bool slab_ptr_is_good(struct k_mem_slab *slab, const void *ptr)
{
	if (!IS_ENABLED(CONFIG_MEM_SLAB_POINTER_VALIDATE)) {
		return true;
	}

	const char *p = ptr;
	ptrdiff_t offset = p - slab->buffer;

	return (offset >= 0) &&
	       (offset < (slab->info.block_size * slab->info.num_blocks)) &&
	       ((offset % slab->info.block_size) == 0);
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&slab->lock);
	int result;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mem_slab, alloc, slab, timeout);

	if (slab->free_list != NULL) {
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->info.num_used++;
		__ASSERT((slab->free_list == NULL &&
			  slab->info.num_used == slab->info.num_blocks) ||
			 slab_ptr_is_good(slab, slab->free_list),
			 "slab corruption detected");

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
		slab->info.max_used = MAX(slab->info.num_used,
					  slab->info.max_used);
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

		result = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT) ||
		   !IS_ENABLED(CONFIG_MULTITHREADING)) {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	} else {
		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_mem_slab, alloc, slab, timeout);

		/* wait for a free block or timeout */
		result = z_pend_curr(&slab->lock, key, &slab->wait_q, timeout);
		if (result == 0) {
			*mem = _current->base.swap_data;
		}

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, alloc, slab, timeout, result);

		return result;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, alloc, slab, timeout, result);

	k_spin_unlock(&slab->lock, key);

	return result;
}

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
	if (!slab_ptr_is_good(slab, mem)) {
		__ASSERT(false, "Invalid memory pointer provided");
		k_panic();
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_mem_slab, free, slab);
	if (unlikely(slab->free_list == NULL) && IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct k_thread *pending_thread = z_unpend_first_thread(&slab->wait_q);

		if (unlikely(pending_thread != NULL)) {
			SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

			z_thread_return_value_set_with_data(pending_thread, 0, mem);
			z_ready_thread(pending_thread);
			z_reschedule(&slab->lock, key);
			return;
		}
	}
	*(char **) mem = slab->free_list;
	slab->free_list = (char *) mem;
	slab->info.num_used--;

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_mem_slab, free, slab);

	k_spin_unlock(&slab->lock, key);
}

int k_mem_slab_runtime_stats_get(struct k_mem_slab *slab, struct sys_memory_stats *stats)
{
	if ((slab == NULL) || (stats == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	stats->allocated_bytes = slab->info.num_used * slab->info.block_size;
	stats->free_bytes = (slab->info.num_blocks - slab->info.num_used) *
			    slab->info.block_size;
#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
	stats->max_allocated_bytes = slab->info.max_used *
				     slab->info.block_size;
#else
	stats->max_allocated_bytes = 0;
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */

	k_spin_unlock(&slab->lock, key);

	return 0;
}

#ifdef CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION
int k_mem_slab_runtime_stats_reset_max(struct k_mem_slab *slab)
{
	if (slab == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&slab->lock);

	slab->info.max_used = slab->info.num_used;

	k_spin_unlock(&slab->lock, key);

	return 0;
}
#endif /* CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION */
