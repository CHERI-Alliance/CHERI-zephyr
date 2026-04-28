/*
 * Copyright (c) 2026 University of Birmingham, added to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * These tests test the CHERI modified kernel/mempool.c heap functions
 * They test CHERI-specific aspects not covered under lib/heap/multiheap
 */

#include <zephyr/ztest.h>
#include <kernel_internal.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/multi_heap.h>
#include "test_mheap.h"

#include "cheri_funcs.h"

#define CHERI_TEST_STACK_SIZE \
	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE + \
	(BLK_NUM_MAX * sizeof(void *) * 2))

K_THREAD_STACK_DEFINE(test_tstack, CHERI_TEST_STACK_SIZE);
static struct k_thread test_tdata;

static const size_t blk_sizes[] = {
	BLK_SIZE_1,
	BLK_SIZE_2,
	BLK_SIZE_3,
	BLK_SIZE_4
};

static const size_t alignment[] = {
	ALIGN_1,
	ALIGN_2,
	ALIGN_3,
	ALIGN_4
};

static void malloc_handler(void *p1, void *p2, void *p3)
{
	void *block[2 * BLK_NUM_MAX];
	int num_allocs;

	size_t blk_size = *(size_t *)p1;

	TC_PRINT("--------- Allocate memory until empty with new block size ---------\n");

	for (num_allocs = 0; num_allocs < ARRAY_SIZE(block); num_allocs++) {
		block[num_allocs] = k_malloc(blk_size);
		if (block[num_allocs] == NULL) {
			break;
		}
		/* check bounds of allocation*/
		/* CHERI bounds will include block size + header + CHERI rounding */
		assert_cheri_mempool("block[num_allocs]\n", block[num_allocs], blk_size);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block[num_allocs]\n", block[num_allocs]);
	}

	TC_PRINT("blk_size: %zu, number of allocations: %zu\n", blk_size, (size_t)num_allocs);
	TC_PRINT("First allocation:\n");
	TC_PRINT("block[0] %p\n", block[0]);
	print_cheri("block[0]\n", block[0]);
	TC_PRINT("Second allocation:\n");
	TC_PRINT("block[1] %p\n", block[1]);
	print_cheri("block[1]\n", block[1]);

	/* check no overlapping bounds between first and second allocation
	 * skip test if we already run out of memory
	 */
	if (block[1] == NULL) {
		TC_PRINT("block[1] NULL, skipping overlap check\n");
	} else {
		assert_cheri_no_overlap("block[0], block[1]\n", block[0], block[1]);
	}

	for (int i = 0; i < num_allocs; i++) {
		k_free(block[i]);
	}
}

static void calloc_handler(void *p1, void *p2, void *p3)
{
	void *block[2 * BLK_NUM_MAX];
	int num_allocs;

	size_t blk_size = *(size_t *)p1;
	size_t numElements = 3;

	TC_PRINT("--------- Allocate memory until empty with new block size ---------\n");

	for (num_allocs = 0; num_allocs < ARRAY_SIZE(block); num_allocs++) {
		block[num_allocs] = k_calloc(numElements, blk_size);
		if (block[num_allocs] == NULL) {
			break;
		}
		/* check bounds of allocation*/
		/* CHERI bounds will include block size + header + CHERI rounding */
		assert_cheri_mempool("block[num_allocs]\n", block[num_allocs],
			 numElements * blk_size);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block[num_allocs]\n", block[num_allocs]);
		/* check memory is zero and can write to the contents */
		assert_cheri_zero_write("block[num_allocs]\n", block[num_allocs]);
	}

	TC_PRINT("blk_size: %zu, numElements: %zu, number of allocations: %zu\n",
		blk_size, numElements, (size_t)num_allocs);
	TC_PRINT("First allocation:\n");
	TC_PRINT("block[0] %p\n", block[0]);
	print_cheri("block[0]\n", block[0]);
	TC_PRINT("Second allocation:\n");
	TC_PRINT("block[1] %p\n", block[1]);
	print_cheri("block[1]\n", block[1]);

	/* check no overlapping bounds between first and second allocation
	 * skip test if we already run out of memory
	 */
	if (block[1] == NULL) {
		TC_PRINT("block[1] NULL, skipping overlap check\n");
	} else {
		assert_cheri_no_overlap("block[0], block[1]\n", block[0], block[1]);
	}

	for (int i = 0; i < num_allocs; i++) {
		k_free(block[i]);
	}
}

static void aligned_alloc_handler(void *p1, void *p2, void *p3)
{
	void *block[2 * BLK_NUM_MAX];
	int num_allocs;

	size_t alignment = *(size_t *)p1;

	TC_PRINT("--------- Allocate memory until empty with new alignment ---------\n");
	TC_PRINT("block size: %zu, new alignment: %zu,",
				(size_t)BLK_SIZE_3, (size_t)alignment);
#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT(" expected user alignment (at least ptr align): %zu",
				(size_t)MAX(alignment, sizeof(void *)));
#endif
	TC_PRINT("\n");

	for (num_allocs = 0; num_allocs < ARRAY_SIZE(block); num_allocs++) {
		block[num_allocs] = k_aligned_alloc(alignment, BLK_SIZE_3);
		if (block[num_allocs] == NULL) {
			break;
		}
		/* check bounds and alignment of first allocation*/
		/* CHERI bounds will include block size + header + alignment pad + cheri rounding */
		assert_cheri_mempool_align("block[num_allocs]\n",
			block[num_allocs], BLK_SIZE_3, alignment);
		/* check base and addr are both aligned to at least cap ptr size */
		assert_cheri_ptr_aligned("block[num_allocs]\n", block[num_allocs]);
	}

	TC_PRINT("number of allocations: %zu\n", (size_t)num_allocs);
	TC_PRINT("First allocation:\n");
	TC_PRINT("block[0] %p\n", block[0]);
	print_cheri("block[0]\n", block[0]);
	TC_PRINT("Second allocation:\n");
	TC_PRINT("block[1] %p\n", block[1]);
	print_cheri("block[1]\n", block[1]);

	/* check no overlapping bounds between first and second allocation
	 * skip test if we already run out of memory
	 */
	if (block[1] == NULL) {
		TC_PRINT("block[1] NULL, skipping overlap check\n");
	} else {
		assert_cheri_no_overlap("block[0], block[1]\n", block[0], block[1]);
	}

	for (int i = 0; i < num_allocs; i++) {
		k_free(block[i]);
	}
}

static void thread_alloc_in_thread_handler(void *p1, void *p2, void *p3)
{
	void *block[2 * BLK_NUM_MAX];
	int num_allocs;
	size_t blk_size = *(size_t *)p1;

	TC_PRINT("--------- Allocate memory until empty with new block size ---------\n");

	for (num_allocs = 0; num_allocs < ARRAY_SIZE(block); num_allocs++) {
		block[num_allocs] = z_thread_malloc(blk_size);
		if (block[num_allocs] == NULL) {
			break;
		}
		/* check bounds of allocation*/
		assert_cheri_mempool("block[num_allocs]\n",
			block[num_allocs], blk_size);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block[num_allocs]\n", block[num_allocs]);
	}

	TC_PRINT("blk_size: %zu, number of allocations: %zu\n", blk_size, (size_t)num_allocs);
	TC_PRINT("First allocation:\n");
	TC_PRINT("block[0] %p\n", block[0]);
	print_cheri("block[0]\n", block[0]);
	TC_PRINT("Second allocation:\n");
	TC_PRINT("block[1] %p\n", block[1]);
	print_cheri("block[1]\n", block[1]);

	/* check no overlapping bounds between first and second allocation
	 * skip test if we already run out of memory
	 */
	if (block[1] == NULL) {
		TC_PRINT("block[1] NULL, skipping overlap check\n");
	} else {
		assert_cheri_no_overlap("block[0], block[1]\n", block[0], block[1]);
	}

	for (int i = 0; i < num_allocs; i++) {
		k_free(block[i]);
	}
}

static void thread_aligned_alloc_in_thread_handler(void *p1, void *p2, void *p3)
{
	void *block[2 * BLK_NUM_MAX];
	int num_allocs;

	size_t alignment = *(size_t *)p1;

	TC_PRINT("--------- Allocate memory until empty with new alignment ---------\n");
	TC_PRINT("block size: %zu, new alignment: %zu,", (size_t)BLK_SIZE_3, (size_t)alignment);
#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT(" expected user alignment (at least ptr align): %zu",
				(size_t)MAX(alignment, sizeof(void *)));
#endif
	TC_PRINT("\n");

	for (num_allocs = 0; num_allocs < ARRAY_SIZE(block); num_allocs++) {
		block[num_allocs] = z_thread_aligned_alloc(alignment, BLK_SIZE_3);
		if (block[num_allocs] == NULL) {
			break;
		}
		/* check bounds of first allocation */
		/* CHERI bounds will include block size + header + alignment pad + cheri rounding */
		assert_cheri_mempool_align("block[num_allocs]\n",
			block[num_allocs], BLK_SIZE_3, alignment);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block[num_allocs]\n", block[num_allocs]);
	}

	TC_PRINT("blk_size: %zu, number of allocations: %zu\n",
		(size_t)BLK_SIZE_3, (size_t)num_allocs);
	TC_PRINT("First allocation:\n");
	TC_PRINT("block[0] %p\n", block[0]);
	print_cheri("block[0]\n", block[0]);
	TC_PRINT("Second allocation:\n");
	TC_PRINT("block[1] %p\n", block[1]);
	print_cheri("block[1]\n", block[1]);

	/* check no overlapping bounds between first and second allocation
	 * skip test if we already run out of memory
	 */
	if (block[1] == NULL) {
		TC_PRINT("block[1] NULL, skipping overlap check\n");
	} else {
		assert_cheri_no_overlap("block[0], block[1]\n", block[0], block[1]);
	}

	for (int i = 0; i < num_allocs; i++) {
		k_free(block[i]);
	}
}

static void realloc_handler(void *p1, void *p2, void *p3)
{
	void *block1, *block2;
	size_t num_allocs, i;
	size_t blk_size = *(size_t *)p1;

	/* First allocation */

	TC_PRINT("--------- Allocate memory with new block size ---------\n");
	TC_PRINT("blk_size: %zu, blk_size + header: %zu\n",
		(size_t)blk_size, (size_t)blk_size + sizeof(void *));

	block1 = k_realloc(NULL, blk_size);
	block2 = block1; /* save first allocation */

	TC_PRINT("block1 %p\n", block1);
	print_cheri("block1\n", block1);
	/* check bounds of allocation*/
	assert_cheri_mempool("block1", block1, blk_size);
	/* check valid */
	zassert_not_null(block1);

	/* Keep making the allocated buffer bigger until the heap is depleted
	 * For bounds expansion CHERI uses alloc + copy so the new and old
	 * should not overlap unless they fall into the same CHERI rounded bounds
	 */
	TC_PRINT("Expanding bounds and doing memory depletion....\n");

	for (num_allocs = 2; num_allocs < (2 * BLK_NUM_MAX); num_allocs++) {

		void *last_block1 = block1;

		TC_PRINT("%zu * blk_size: %zu\n", (size_t)num_allocs, num_allocs * blk_size);

		block1 = k_realloc(block1, num_allocs * blk_size);

		/* For CHERI we can't expand the bounds of an allocated pointer
		 * without going back to the original heap pointer first
		 * and then re-allocating. CHERI uses the default
		 * option to allocate new and then copy the data.
		 * We need to ensure the heap is big enough to do the copy
		 * to allow at least 1 re-allocation.
		 */

		if (block1 == NULL) {
			block1 = last_block1;
			break;
		}

		/* check bounds of allocation*/
		assert_cheri_mempool("block1\n", block1, num_allocs * blk_size);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block1\n", block1);
	}
	TC_PRINT("number of re-allocations: %zu\n", (size_t)num_allocs);
	TC_PRINT("Last allocation:\n");
	TC_PRINT("block1 %p\n", block1);
	print_cheri("block1\n", block1);
	/* check no overlapping bounds between first and last allocation
	 * Note: some reallocations in loop may overlap if still within bounds
	 * due to CHERI rounding
	 */
	assert_cheri_no_overlap("first, last\n", block2, block1);

	/* Now Keep making the allocated buffer smaller, all allocations
	 * should overlap
	 */
	TC_PRINT("Reducing bounds and checking for overlap....\n");

	for (i = num_allocs-1; i > 0; i--) {
		void *last_block1 = block1;

		TC_PRINT("%zu * blk_size: %zu\n", (size_t)i, i * blk_size);

		block1 = k_realloc(block1, i * blk_size);

		/* check bounds overlap in all cases */
		assert_cheri_overlap("block1, last_block1\n", block1, last_block1);
		/* check bounds of allocation*/
		assert_cheri_mempool("block1\n", block1, i * blk_size);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("block1\n", block1);
		zassert_not_null(block1);
	}

	TC_PRINT("Last allocation:\n");
	TC_PRINT("block1 %p\n", block1);
	print_cheri("block1\n", block1);

	/* Deallocate and null pointer */
	block1 = k_realloc(block1, 0);
	/* Return NULL after freed */
	zassert_is_null(block1);
}

/*test cases*/

/**
 * @brief Test to demonstrate CHERI-modified k_malloc() and k_free()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates from heap memory pool using k_malloc().
 * It repeats for different block sizes to check CHERI rounding
 * and alignment. It uses k_free() to free all the allocated memory.
 *
 * @see k_malloc()
 */
ZTEST(cheri_mheap_api, test_a_k_malloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	TC_PRINT("CONFIG_HEAP_MEM_POOL_SIZE: %i\n", CONFIG_HEAP_MEM_POOL_SIZE);

	for (int i = 0; i < ARRAY_SIZE(blk_sizes); i++) {
		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 malloc_handler,
				 (void *)&blk_sizes[i],  /* p1 = pointer to block size */
				  NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified k_calloc().
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates from heap memory pool using k_calloc().
 * It repeats for different block sizes to check CHERI rounding
 * and alignment. It checks the memory is zeroed and can be
 * written to. It uses k_free() to free all the allocated memory.
 *
 * @see k_calloc()
 */
ZTEST(cheri_mheap_api, test_b_k_calloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	TC_PRINT("CONFIG_HEAP_MEM_POOL_SIZE: %i\n", CONFIG_HEAP_MEM_POOL_SIZE);

	for (int i = 0; i < ARRAY_SIZE(blk_sizes); i++) {
		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 calloc_handler,
				 (void *)&blk_sizes[i],  /* p1 = pointer to block size */
				  NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified k_aligned_alloc().
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates from heap memory pool using k_aligned_alloc().
 * It repeats for different alignment sizes to check CHERI rounding
 * and alignment. It uses k_free() to free all the allocated memory.
 *
 * @see k_aligned_alloc()
 */
ZTEST(cheri_mheap_api, test_c_k_aligned_alloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	TC_PRINT("CONFIG_HEAP_MEM_POOL_SIZE: %i\n", CONFIG_HEAP_MEM_POOL_SIZE);

	for (int i = 0; i < ARRAY_SIZE(alignment); i++) {
		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 aligned_alloc_handler,
				 (void *)&alignment[i],  /* p1 = pointer to alignment */
				  NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified z_thread_malloc().
 *
 * @details The test allocates from heap memory pool using z_thread_malloc().
 * It repeats for different block sizes to check CHERI rounding
 * and alignment. It uses k_free() to free all the allocated memory.
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @see z_thread_malloc()
 */
ZTEST(cheri_mheap_api, test_d_z_thread_malloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(blk_sizes); i++) {

		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 thread_alloc_in_thread_handler,
				 (void *)&blk_sizes[i],  /* p1 = pointer to block size */
				 NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified z_thread_aligned_alloc().
 *
 * @details The test allocates from heap memory pool using
 * z_thread_aligned_alloc().
 * It repeats for different alignment sizes to check CHERI rounding
 * and alignment. It uses k_free() to free all the allocated memory.
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @see z_thread_aligned_alloc()
 */
ZTEST(cheri_mheap_api, test_e_z_thread_aligned_alloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(alignment); i++) {
		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 thread_aligned_alloc_in_thread_handler,
				 (void *)&alignment[i],  /* p1 = pointer to alignment */
				 NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified k_realloc().
 *
 * @details The test allocates from heap memory pool using
 * k_realloc().
 * It repeats for different block sizes to check CHERI rounding
 * and alignment. It uses k_free() to free all the allocated memory.
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @see k_realloc()
 */
ZTEST(cheri_mheap_api, test_f_k_realloc)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(blk_sizes); i++) {

		k_tid_t tid = k_thread_create(&test_tdata, test_tstack,
				 CHERI_TEST_STACK_SIZE,
				 realloc_handler,
				 (void *)&blk_sizes[i],  /* p1 = pointer to block size */
				 NULL, NULL,
				 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}
}
