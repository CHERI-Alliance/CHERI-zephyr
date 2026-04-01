/*
 * Copyright (c) 2026 University of Birmingham, modified for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/heap_listener.h>
#include <inttypes.h>
#include <../lib/heap/heap.h>

#include "inline_funcs.h"

#define HEAP_SIZE	512
#define ALLOC1		150
#define ALLOC2		100
#define ALLOC3		8
#define CHUNK_SIZE	8

/**
 * @brief Test sys_heap API with CHERI memory alignment
 *
 * @defgroup cheri_sys_heap_kheap_tests CHERI system heap & kheap tests
 *
 * @ingroup cheri_tests
 *
 * This tests basic k_heap allocation and compares to sys_heap
 * allocation which uses the same underlying library. Checks
 * CHERI bounds and alignment.
 */

/* Kernel heap from macro */
K_HEAP_DEFINE(kernel_heap, HEAP_SIZE);

/* Raw sys_heap from backing buffer */
static char sys_heap_mem[HEAP_SIZE];
static struct sys_heap heap;

static void check_cheri_alloc_len(void *p, size_t len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cherilen = __builtin_cheri_length_get(p);
	/* round up to at least cap size */
	size_t round_up = WB_UP(len);
	/* then round to cheri bounds */
	size_t cheriround = __builtin_cheri_round_representable_length(round_up);

	printk("cheriround: %zu\n", cheriround);
	zassert_equal(cherilen, cheriround,
			"alloc bounds %zu do not match expected length %zu", cherilen, cheriround);
#endif
}

static void check_cheri_heap_len(void *p, size_t len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cherilen = __builtin_cheri_length_get(p);
	/* round down to chunk size first */
	size_t round_down = ROUND_DOWN(len, CHUNK_SIZE);
	/* then round to cheri bounds */
	size_t cheriround = __builtin_cheri_round_representable_length(round_down);

	zassert_equal(cherilen, cheriround,
			"heap bounds %zu do not match expected length %zu", cherilen, cheriround);
#endif
}

static void check_cheri_buffer_len(void *p, size_t len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cherilen = __builtin_cheri_length_get(p);
	size_t cheriround = __builtin_cheri_round_representable_length(len);

	zassert_equal(cherilen, cheriround,
			"heap bounds %zu do not match expected length %zu", cherilen, cheriround);
#endif
}

static void check_cheri_align(void *p)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cheribase = __builtin_cheri_base_get(p);
	size_t cheriAddr = __builtin_cheri_address_get(p);

	zassert_equal(cheribase, cheriAddr,
			"base not aligned to addr");
#endif
}

static void compare_heaps(void)
{
	TC_PRINT("=== comparing heaps ===\n");
	TC_PRINT("Requested heap size: %zu\n", (size_t)HEAP_SIZE);
#ifdef __CHERI_PURE_CAPABILITY__
	size_t sys_heap_cherilen = __builtin_cheri_length_get(&sys_heap_mem);
	size_t kheap_cherilen = __builtin_cheri_length_get(kernel_heap.heap.heap);

	TC_PRINT("sys_heap CHERI bounds size: %zu\n", sys_heap_cherilen);
	TC_PRINT("k_heap CHERI bounds size: %zu\n", kheap_cherilen);
		zassert_equal(sys_heap_cherilen, kheap_cherilen,
		"heap bounds do not match");
#endif
}

static void print_stats(const char *label, struct sys_heap *hp)
{
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(hp, &stats);

	TC_PRINT("%s allocated bytes %zu, free bytes %zu\n",
		label, stats.allocated_bytes, stats.free_bytes);
}

static void test_sys_heap_path(void)
{
	void *p, *p2;

	TC_PRINT("=== Testing sys_heap ===\n");
	print_cheri("sys_heap_mem backing buffer\n", &sys_heap_mem);
	check_cheri_buffer_len(&sys_heap_mem, HEAP_SIZE);
	check_cheri_align(&sys_heap_mem);

	sys_heap_init(&heap, sys_heap_mem, HEAP_SIZE);
	print_cheri("heap memory initiated\n", heap.heap);
	check_cheri_heap_len(heap.heap, HEAP_SIZE);
	check_cheri_align(heap.heap);
	print_stats("SYS_HEAP", &heap);

	p = sys_heap_alloc(&heap, ALLOC1);
	print_cheri("p150\n", p);
	check_cheri_alloc_len(p, ALLOC1);
	check_cheri_align(p);
	print_stats("SYS_HEAP", &heap);

	p = sys_heap_realloc(&heap, p, ALLOC2);
	print_cheri("p100\n", p);
	check_cheri_alloc_len(p, ALLOC2);
	check_cheri_align(p);
	print_stats("SYS_HEAP", &heap);
	zassert_true(p != NULL, "memory not re-allocated, "
			"check the heap size is big enough, "
			"needs to be re-allocated from heap in CHERI");

	p2 = sys_heap_alloc(&heap, ALLOC3);
	print_cheri("p8\n", p2);
	check_cheri_alloc_len(p2, ALLOC3);
	check_cheri_align(p2);
	print_stats("SYS_HEAP", &heap);

	sys_heap_free(&heap, p);
	sys_heap_free(&heap, p2);
	print_stats("SYS_HEAP", &heap);
}

static void test_k_heap_path(void)
{
	void *p, *p2;

	TC_PRINT("=== Testing k_heap ===\n");
	print_cheri("kernel_heap memory initiated\n", kernel_heap.heap.heap);
	check_cheri_heap_len(kernel_heap.heap.heap, HEAP_SIZE);
	check_cheri_align(kernel_heap.heap.heap);
	print_stats("K_HEAP", &kernel_heap.heap);

	p = k_heap_alloc(&kernel_heap, ALLOC1, K_NO_WAIT);
	print_cheri("p150\n", p);
	check_cheri_alloc_len(p, ALLOC1);
	check_cheri_align(p);
	print_stats("K_HEAP", &kernel_heap.heap);

	p = k_heap_realloc(&kernel_heap, p, ALLOC2, K_NO_WAIT);
	print_cheri("p100\n", p);
	check_cheri_alloc_len(p, ALLOC2);
	check_cheri_align(p);
	print_stats("K_HEAP", &kernel_heap.heap);
	zassert_true(p != NULL, "memory not re-allocated, "
			"check the heap size is big enough, "
			"needs to be re-allocated from heap in CHERI");

	p2 = k_heap_alloc(&kernel_heap, ALLOC3, K_NO_WAIT);
	print_cheri("p8\n", p2);
	check_cheri_alloc_len(p2, ALLOC3);
	check_cheri_align(p2);
	print_stats("K_HEAP", &kernel_heap.heap);

	k_heap_free(&kernel_heap, p);
	k_heap_free(&kernel_heap, p2);
	print_stats("K_HEAP", &kernel_heap.heap);
}


/**
 * @brief compare heaps with CHERI
 *
 * @details Verify that the system assigns memory
 * correctly with CHERI. Asserts this against
 * what is expected.
 *
 * @ingroup cheri_sys_heap_kheap_tests
 */
ZTEST(cheri_lib_heap_sys_heap, test_compare_heaps)
{
	compare_heaps();
	test_sys_heap_path();
	test_k_heap_path();
}

ZTEST_SUITE(cheri_lib_heap_sys_heap, NULL, NULL, NULL, NULL, NULL);
