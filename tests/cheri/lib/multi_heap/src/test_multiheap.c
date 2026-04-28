/*
 * Copyright (c) 2026 University of Birmingham, added to support CHERI tests
 *
 * These tests test the CHERI modified heap and kernel/multi_heap.c heap functions
 * They test CHERI-specific aspects not covered under lib/heap/multiheap
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <kernel_internal.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/multi_heap.h>

#include "test_mheap.h"
#include "cheri_funcs.h"

/* multi heaps */
/* number of heaps */
#define N_MULTI_HEAPS 4
/* size of each heap: purposefully set an awkward size
 * use CHERI macros to round up. This ensures that when
 * the bounds split into individual heaps it can be done
 * cleanly. Alternatively stick to powers-of-2
 */
#ifdef __CHERI_PURE_CAPABILITY__
#define AWKWARD_SIZE 333
#define MHEAP_BYTES CHERI_ROUND_UP_TO_REP_LEN(WB_UP(AWKWARD_SIZE), 1)
#else
#define MHEAP_BYTES 256
#endif

static const size_t alignment[] = {
	ALIGN_1,
	ALIGN_2,
	ALIGN_3,
	ALIGN_4
};

/* multiheap structure */
static struct sys_multi_heap multi_heap;
/* backing buffer */
/* we align the backing buffer memory to 'at least' pointer size because
 * a single heap is internally CHERI aligned to at least a pointer size.
 * Without this, if the alignment of heap_mem comes out at 4 bytes say
 * you end up with a single heap size less than you asked for.
 */
__aligned(sizeof(void *)) static char heap_mem[N_MULTI_HEAPS][MHEAP_BYTES];
/* array of ptrs to each heap structure */
static struct sys_heap mheaps[N_MULTI_HEAPS];


void *multi_heap_choice(struct sys_multi_heap *mheap, void *cfg,
			size_t align, size_t size)
{
	struct sys_heap *h = cfg;

	return sys_heap_aligned_alloc(h, align, size);
}

/*test cases*/

/**
 * @brief Test to demonstrate CHERI-modified heap with sys_multi_heap_init()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test initialises multi-heaps and checks
 * CHERI rounding and alignment.
 *
 * @see sys_multi_heap_init()
 */
ZTEST(cheri_mheap_api, test_g_sys_multi_heap_init)
{
	/* backing buffer heap_mem */
	TC_PRINT("Number heaps: %zu, size each heap: %zu, backing buffer size: %zu\n",
		(size_t)N_MULTI_HEAPS, (size_t)MHEAP_BYTES, (size_t)MHEAP_BYTES * N_MULTI_HEAPS);
	TC_PRINT("heap_mem: %p\n", heap_mem);
	print_cheri("", heap_mem);
	/* check valid cap */
	assert_cheri_valid_tag("heap_mem - not valid\n", heap_mem);
	/* check correctly cheri rounded, aligned and bounded */
	assert_cheri_singleheap("", heap_mem, MHEAP_BYTES * N_MULTI_HEAPS);

	/* individual heaps */

	/* set allocation function to use for multi_heap */
	sys_multi_heap_init(&multi_heap, multi_heap_choice);

	/* initialise each heap structure in mheaps from backing buffer heap_mem */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {
		/* Reduce CHERI bounds from heap array &heap_mem[i][0]
		 * to a single heap for mheaps[i].heap
		 */
		sys_heap_init(&mheaps[i], &heap_mem[i][0], MHEAP_BYTES);
		/* register and sort heaps */
		sys_multi_heap_add_heap(&multi_heap, &mheaps[i], NULL);
	}

	/* print bounds of individual heaps */

	/* single heap memory from structure
	 * mheaps[i].heap is a pointer to z_heap struct,
	 * which sits at the start of a single heap memory
	 * It has CHERI bounds covering a single heap.
	 * The CHERI bounds for heap_mem[i] are inherited from
	 * the whole backing buffer.
	 */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {
		TC_PRINT("--------- Heap %zu ---------\n", (size_t)i);
		TC_PRINT("mheaps[%zu].heap: %p\n", (size_t)i, mheaps[i].heap);
		print_cheri("", mheaps[i].heap);
		/* check valid cap */
		assert_cheri_valid_tag("mheaps[i].heap - not valid\n", mheaps[i].heap);
		/* check correctly cheri rounded, aligned and bounded */
		assert_cheri_singleheap("", mheaps[i].heap, MHEAP_BYTES);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified heap with sys_multi_heap_alloc()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates blocks of memory from  multi-heaps
 * and checks CHERI rounding and alignment. Memory is freed using
 * sys_multi_heap_free().
 *
 * @see sys_multi_heap_alloc()
 */
ZTEST(cheri_mheap_api, test_h_sys_multi_heap_alloc)
{
	char *blocks[N_MULTI_HEAPS];

	TC_PRINT("--------- Allocate from each heap memory ---------\n");
	TC_PRINT("block_size: %zu\n", (size_t)MHEAP_BYTES / 2);

	/* allocate blocks */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {

		blocks[i] = sys_multi_heap_alloc(&multi_heap, &mheaps[i],
						 MHEAP_BYTES / 2);

		TC_PRINT("--------- Heap %zu ---------\n", (size_t)i);
		TC_PRINT("blocks[%zu]: %p\n", (size_t)i, blocks[i]);
		print_cheri("", blocks[i]);

		/* check bounds of allocation*/
		assert_cheri_singleheap("", blocks[i], MHEAP_BYTES / 2);
		/* check bounds contained within correct heap */
		assert_cheri_within_bounds("", mheaps[i].heap, blocks[i]);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("blocks[i]\n", blocks[i]);
#ifndef __CHERI_PURE_CAPABILITY__
		/* keep for non-CHERI build */
		zassert_not_null(blocks[i], "allocation failed");
		zassert_true(blocks[i] >= &heap_mem[i][0] &&
			     blocks[i] < &heap_mem[i+1][0],
			     "allocation not in correct heap");
#endif
	}

	/* Free blocks */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {
		sys_multi_heap_free(&multi_heap, blocks[i]);
	}

}

/**
 * @brief Test to demonstrate CHERI-modified heap with sys_multi_heap_realloc()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates and reallocates blocks of memory from
 *  multi-heaps checking shrinking and expansion, and checks CHERI
 * rounding and alignment. Memory is freed using sys_multi_heap_free().
 *
 * @see sys_multi_heap_realloc()
 */
ZTEST(cheri_mheap_api, test_i_sys_multi_heap_realloc)
{
	char *blocks[N_MULTI_HEAPS];

	for (int i = 0; i < N_MULTI_HEAPS; i++) {

		/* allocate block */

		blocks[i] = sys_multi_heap_alloc(&multi_heap, &mheaps[i],
					 MHEAP_BYTES / 4);

		TC_PRINT("--------- Heap %zu ---------\n", (size_t)i);
		TC_PRINT("blocks[%zu] - alloc some memory: %p\n", (size_t)i, blocks[i]);
		print_cheri("", blocks[i]);
		/* check valid cap */
		assert_cheri_valid_tag("blocks[i] - allocation failed\n", blocks[i]);

#ifndef __CHERI_PURE_CAPABILITY__
		/* keep for non-CHERI build */
		zassert_not_null(blocks[i], "allocation failed");
#endif

		/* in place reallocation same size */
		void *ptr = sys_multi_heap_realloc(&multi_heap, &mheaps[i],
			blocks[i], MHEAP_BYTES / 4);

		TC_PRINT("ptr - realloc same size: %p\n", ptr);
		print_cheri("", ptr);
		/* check valid cap */
		assert_cheri_valid_tag("ptr - allocation failed\n", ptr);
		/* check bounds within prev */
		assert_cheri_within_bounds("", blocks[i], ptr);
		/* check bounds of allocation*/
		assert_cheri_singleheap("", ptr, MHEAP_BYTES / 4);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("ptr\n", ptr);

		zassert_equal(ptr, blocks[i], "realloc moved pointer");

		/* in place reallocation smaller size */
		void *ptr2 = sys_multi_heap_realloc(&multi_heap, &mheaps[i],
			blocks[i], MHEAP_BYTES / 8);

		TC_PRINT("ptr2 realloc shrink size: %p\n", ptr2);
		print_cheri("", ptr2);
		/* check valid cap */
		assert_cheri_valid_tag("ptr2 - reallocation failed\n", ptr2);
		/* check bounds within prev */
		assert_cheri_within_bounds("", ptr, ptr2);
		/* check bounds of allocation*/
		assert_cheri_singleheap("", ptr2, MHEAP_BYTES / 8);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("ptr2\n", ptr2);

#ifndef __CHERI_PURE_CAPABILITY__
		/* keep for non-CHERI build */
		zassert_equal(ptr2, ptr, "realloc moved pointer");
#endif

		/* in place reallocation increase size - alloc and copy for CHERI */
		blocks[i] = sys_multi_heap_realloc(&multi_heap, &mheaps[i],
			ptr2, MHEAP_BYTES / 4);

		TC_PRINT("blocks[%zu] realloc increase size: %p\n", (size_t)i, blocks[i]);
		print_cheri("", blocks[i]);
		/* check valid cap */
		assert_cheri_valid_tag("blocks[i] - heap not big enough to re-alloc\n", blocks[i]);
		/* check bounds do not overlap */
		assert_cheri_no_overlap("", ptr2, blocks[i]);
		/* check bounds still within this heap */
		assert_cheri_within_bounds("", mheaps[i].heap, blocks[i]);
		/* check bounds of allocation*/
		assert_cheri_singleheap("", blocks[i], MHEAP_BYTES / 4);
		/* check base and addr are both aligned to a cap ptr size */
		assert_cheri_ptr_aligned("blocks[i]\n", blocks[i]);

#ifndef __CHERI_PURE_CAPABILITY__
		/* keep for non-CHERI build - realloc increase in place */
		zassert_equal(blocks[i], ptr2, "realloc moved pointer");
#endif
	}

	/* Free blocks */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {
		sys_multi_heap_free(&multi_heap, blocks[i]);
	}
}

/**
 * @brief Test to demonstrate CHERI-modified heap with sys_multi_heap_aligned_alloc()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates blocks of memory from  multi-heaps.
 * It repeats for different alignment sizes to check CHERI rounding
 * and alignment. Memory is freed using sys_multi_heap_free().
 *
 * @see sys_multi_heap_aligned_alloc()
 */
ZTEST(cheri_mheap_api, test_j_sys_multi_heap_aligned_alloc)
{
	char *blocks[N_MULTI_HEAPS];

	/* do for all alignments */
	for (int j = 0; j < ARRAY_SIZE(alignment); j++) {
		TC_PRINT("--------- New alignment for allocations: %zu ---------\n", alignment[j]);
		/* allocate blocks */
		for (int i = 0; i < N_MULTI_HEAPS; i++) {

			blocks[i] = sys_multi_heap_aligned_alloc(&multi_heap, &mheaps[i],
						 alignment[j], MHEAP_BYTES / 2);

			TC_PRINT("--------- Heap %zu ---------\n", (size_t)i);
			TC_PRINT("blocks[%zu]: %p\n", (size_t)i, blocks[i]);
			print_cheri("", blocks[i]);
			/* check valid cap */
			assert_cheri_valid_tag("blocks[i] - not allocated\n", blocks[i]);
			/* check bounds of allocation*/
			assert_cheri_singleheap("blocks[i]", blocks[i], MHEAP_BYTES / 2);
			/* check bounds contained within correct heap */
			assert_cheri_within_bounds("", mheaps[i].heap, blocks[i]);
			/* check base and addr are both aligned to a cap ptr size */
			assert_cheri_ptr_aligned("blocks[i]\n", blocks[i]);

#ifndef __CHERI_PURE_CAPABILITY__
			/* keep for non-CHERI build */
			zassert_not_null(blocks[i], "allocation failed");
			zassert_true(blocks[i] >= &heap_mem[i][0] &&
			     blocks[i] < &heap_mem[i+1][0],
			     "allocation not in correct heap");
#endif
		}

		/* Free blocks */
		for (int i = 0; i < N_MULTI_HEAPS; i++) {
			sys_multi_heap_free(&multi_heap, blocks[i]);
		}
	}
}

/**
 * @brief Test to demonstrate CHERI-modified heap with sys_multi_heap_aligned_realloc()
 *
 * @ingroup cheri_k_heap_api_tests
 *
 * @details The test allocates and reallocates blocks of memory from
 * multi-heaps checking shrinking and expansion. It repeats for
 * different alignment sizes to check CHERI rounding and alignment.
 * Memory is freed using sys_multi_heap_free().
 *
 * @see sys_multi_heap_aligned_realloc()
 */
ZTEST(cheri_mheap_api, test_k_sys_multi_heap_aligned_realloc)
{
	char *blocks[N_MULTI_HEAPS];

/* do for all alignments */
	for (int j = 0; j < ARRAY_SIZE(alignment); j++) {
		TC_PRINT("--------- New alignment for allocations: %zu ---------\n", alignment[j]);
		for (int i = 0; i < N_MULTI_HEAPS; i++) {
			TC_PRINT("--------- Heap %zu ---------\n", (size_t)i);

			/* allocate block */

			blocks[i] = sys_multi_heap_aligned_alloc(&multi_heap, &mheaps[i],
					 alignment[j], MHEAP_BYTES / 4);

			TC_PRINT("blocks[%zu] - alloc some memory: %p\n", (size_t)i, blocks[i]);
			print_cheri("", blocks[i]);
			/* check valid cap */
			assert_cheri_valid_tag("blocks[i] - allocation failed\n", blocks[i]);

#ifndef __CHERI_PURE_CAPABILITY__
			/* keep for non-CHERI build */
			zassert_not_null(blocks[i], "allocation failed");
#endif

			/* in place reallocation same size */
			void *ptr = sys_multi_heap_aligned_realloc(&multi_heap, &mheaps[i],
					blocks[i], alignment[j], MHEAP_BYTES / 4);


			TC_PRINT("ptr - realloc same size: %p\n", ptr);
			print_cheri("", ptr);
			/* check valid cap */
			assert_cheri_valid_tag("ptr - allocation failed\n", ptr);
			/* check bounds within prev */
			assert_cheri_within_bounds("", blocks[i], ptr);
			/* check bounds of allocation*/
			assert_cheri_singleheap("", ptr, MHEAP_BYTES / 4);
			/* check base and addr are both aligned to a cap ptr size */
			assert_cheri_ptr_aligned("ptr\n", ptr);

			zassert_equal(ptr, blocks[i], "realloc moved pointer");

			/* in place reallocation smaller size */
			void *ptr2 = sys_multi_heap_aligned_realloc(&multi_heap, &mheaps[i],
						blocks[i], alignment[j], MHEAP_BYTES / 8);

			TC_PRINT("ptr2 realloc shrink size: %p\n", ptr2);
			print_cheri("", ptr2);
			/* check valid cap */
			assert_cheri_valid_tag("ptr2 - reallocation failed\n", ptr2);
			/* check bounds within prev */
			assert_cheri_within_bounds("", ptr, ptr2);
			/* check bounds of allocation*/
			assert_cheri_singleheap("", ptr2, MHEAP_BYTES / 8);
			/* check base and addr are both aligned to a cap ptr size */
			assert_cheri_ptr_aligned("ptr2\n", ptr2);

#ifndef __CHERI_PURE_CAPABILITY__
			/* keep for non-CHERI build */
			zassert_equal(ptr2, ptr, "realloc moved pointer");
#endif

			/* in place reallocation increase size - alloc and copy for CHERI */
			blocks[i] = sys_multi_heap_aligned_realloc(&multi_heap, &mheaps[i],
					ptr2, alignment[j], MHEAP_BYTES / 4);

			TC_PRINT("blocks[%zu] realloc increase size: %p\n", (size_t)i, blocks[i]);
			print_cheri("", blocks[i]);
			/* check valid cap */
			assert_cheri_valid_tag("blocks[i] - heap not big enough to re-alloc\n",
								blocks[i]);
			/* check bounds do not overlap */
			assert_cheri_no_overlap("", ptr2, blocks[i]);
			/* check bounds still within this heap */
			assert_cheri_within_bounds("", mheaps[i].heap, blocks[i]);
			/* check bounds of allocation*/
			assert_cheri_singleheap("", blocks[i], MHEAP_BYTES / 4);
			/* check base and addr are both aligned to a cap ptr size */
			assert_cheri_ptr_aligned("blocks[i]\n", blocks[i]);

#ifndef __CHERI_PURE_CAPABILITY__
			/* keep for non-CHERI build realloc increase in place */
			zassert_equal(blocks[i], ptr2, "realloc moved pointer");
#endif
		}

		/* Free blocks */
		for (int i = 0; i < N_MULTI_HEAPS; i++) {
			sys_multi_heap_free(&multi_heap, blocks[i]);
		}
	}
}
