/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_internal.h"

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/internal/syscall_handler.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0
#define BA_SIZE CONFIG_DYNAMIC_THREAD_POOL_SIZE
#else
#define BA_SIZE 1
#endif /* CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0 */

struct dyn_cb_data {
	k_tid_t tid;
	k_thread_stack_t *stack;
};

static K_THREAD_STACK_ARRAY_DEFINE(dynamic_stack, CONFIG_DYNAMIC_THREAD_POOL_SIZE,
				   CONFIG_DYNAMIC_THREAD_STACK_SIZE);
SYS_BITARRAY_DEFINE_STATIC(dynamic_ba, BA_SIZE);

static k_thread_stack_t *z_thread_stack_alloc_pool(size_t size)
{
	int rv;
	size_t offset;
	k_thread_stack_t *stack;

	if (size > CONFIG_DYNAMIC_THREAD_STACK_SIZE) {
		LOG_DBG("stack size %zu is > pool stack size %d", size,
			CONFIG_DYNAMIC_THREAD_STACK_SIZE);
		return NULL;
	}

	rv = sys_bitarray_alloc(&dynamic_ba, 1, &offset);
	if (rv < 0) {
		LOG_DBG("unable to allocate stack from pool");
		return NULL;
	}

	__ASSERT_NO_MSG(offset < CONFIG_DYNAMIC_THREAD_POOL_SIZE);

	stack = (k_thread_stack_t *)&dynamic_stack[offset];
#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * For CHERI this is a static array of stacks.
	 * Here we can reduce the bounds to the one selected stack
	 * work out the cheri length and set the bounds.
	 */
	size_t cheri_len = __builtin_cheri_round_representable_length(size);

	stack = __builtin_cheri_bounds_set_exact(stack, cheri_len);
#endif
	return stack;
}

static k_thread_stack_t *z_thread_stack_alloc_dyn(size_t size, int flags)
{
	if ((flags & K_USER) == K_USER) {
#ifdef CONFIG_DYNAMIC_OBJECTS
		return k_object_alloc_size(K_OBJ_THREAD_STACK_ELEMENT, size);
#else
		/* Dynamic user stack needs a kobject, so if this option is not
		 * enabled we can't proceed.
		 */
		return NULL;
#endif /* CONFIG_DYNAMIC_OBJECTS */
	}

#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * For CHERI bounds, the returned allocation from sysheap takes into account
	 * chunk sizes, meta data and cheri rounding.
	 * The address of the pointer returned is therefore not aligned with the base.
	 * The address instead is aligned to what is requested here.
	 * We can't narrow the bounds further here because
	 * the meta data information and bounds are needed later during k_free
	 * when freeing the memory off the heap.
	 *
	 * The bounds are narrowed further during thread creation so
	 * we need to ensure the address is cheri aligned for what we need
	 * by the stack later - arch_new_thread() sets exact bounds on the buffer.
	 */
	size_t cheri_len = __builtin_cheri_round_representable_length(K_KERNEL_STACK_LEN(size));
	size_t cheri_align = ~__builtin_cheri_representable_alignment_mask(cheri_len) + 1;

	return z_thread_aligned_alloc(MAX(cheri_align, Z_KERNEL_STACK_OBJ_ALIGN), cheri_len);
#else
	return z_thread_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN, K_KERNEL_STACK_LEN(size));
#endif /* __CHERI_PURE_CAPABILITY__ */
}

k_thread_stack_t *z_impl_k_thread_stack_alloc(size_t size, int flags)
{
	k_thread_stack_t *stack = NULL;

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		stack = z_thread_stack_alloc_dyn(size, flags);
		if (stack == NULL && CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size);
		}
	} else if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_POOL)) {
		if (CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size);
		}

		if ((stack == NULL) && IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
			stack = z_thread_stack_alloc_dyn(size, flags);
		}
	}

	return stack;
}

#ifdef CONFIG_USERSPACE
static inline k_thread_stack_t *z_vrfy_k_thread_stack_alloc(size_t size, int flags)
{
	return z_impl_k_thread_stack_alloc(size, flags);
}
#include <zephyr/syscalls/k_thread_stack_alloc_mrsh.c>
#endif /* CONFIG_USERSPACE */

static void dyn_cb(const struct k_thread *thread, void *user_data)
{
	struct dyn_cb_data *const data = (struct dyn_cb_data *)user_data;

	if (data->stack == (k_thread_stack_t *)thread->stack_info.start) {
		__ASSERT(data->tid == NULL, "stack %p is associated with more than one thread!",
			 (void *)thread->stack_info.start);
		data->tid = (k_tid_t)thread;
	}
}

int z_impl_k_thread_stack_free(k_thread_stack_t *stack)
{
	struct dyn_cb_data data = {.stack = stack};

	/* Get a possible tid associated with stack */
	k_thread_foreach(dyn_cb, &data);

	if (data.tid != NULL) {
		if (!(z_is_thread_state_set(data.tid, _THREAD_DUMMY) ||
		      z_is_thread_state_set(data.tid, _THREAD_DEAD))) {
			LOG_ERR("tid %p is in use!", data.tid);
			return -EBUSY;
		}
	}

	if (CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
		if (IS_ARRAY_ELEMENT(dynamic_stack, stack)) {
			if (sys_bitarray_free(&dynamic_ba, 1, ARRAY_INDEX(dynamic_stack, stack))) {
				LOG_ERR("stack %p is not allocated!", stack);
				return -EINVAL;
			}

			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
#ifdef CONFIG_USERSPACE
		if (k_object_find(stack)) {
			k_object_free(stack);
		} else {
			k_free(stack);
		}
#else
		k_free(stack);
#endif /* CONFIG_USERSPACE */
	} else {
		LOG_DBG("Invalid stack %p", stack);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_stack_free(k_thread_stack_t *stack)
{
	/* The thread stack object must not be in initialized state.
	 *
	 * Thread stack objects are initialized when the thread is created
	 * and de-initialized when the thread is destroyed. Since we can't
	 * free a stack that is in use, we have to check that the caller
	 * has access to the object but that it is not in use anymore.
	 */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(stack, K_OBJ_THREAD_STACK_ELEMENT));

	return z_impl_k_thread_stack_free(stack);
}
#include <zephyr/syscalls/k_thread_stack_free_mrsh.c>
#endif /* CONFIG_USERSPACE */
