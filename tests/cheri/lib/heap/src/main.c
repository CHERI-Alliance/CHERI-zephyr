/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 University of Birmingham, modified to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/tc_util.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/heap_listener.h>
#include <inttypes.h>
#include <../lib/heap/heap.h>

/**
 * @brief Test sys_heap API with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_sys_heap_tests CHERI system heap lib tests
 *
 * @ingroup cheri_tests
 *
 * This is a modified variation of some of the tests in
 * tests/lib/heap but are tailored to be CHERI-specific
 * for bounds checking. More tests are also added.
 */

#include "inline_funcs.h"
/* how many allocations to print cheri bounds for */
#define PRINT_ALLOCS 8
int current_allocs = 1;

/* trust CONFIG_SRAM_SIZE */
# define MEMSZ (1024 * (size_t) CONFIG_SRAM_SIZE)

#define BIG_HEAP_SZ MIN(256 * 1024, MEMSZ / 3)
#define SMALL_HEAP_SZ MIN(BIG_HEAP_SZ, 2048)

/* With enabling SYS_HEAP_RUNTIME_STATS, the size of struct z_heap
 * will increase 16 bytes on 64 bit CPU.
 */
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
#define SOLO_FREE_HEADER_HEAP_SZ (80)
#else
#define SOLO_FREE_HEADER_HEAP_SZ (64)
#endif

#define SCRATCH_SZ (sizeof(heapmem) / 2)

/* backing memory heap - used in app/self-defined heaps */
/* The test memory.  Make them pointer arrays for robust alignment
 * behavior
 */

/* type void* so each element is 8/16 bytes for cheri */
void *heapmem[BIG_HEAP_SZ / sizeof(void *)];
void *scratchmem[SCRATCH_SZ / sizeof(void *)];

/* How many alloc/free operations are tested on each heap.  Two per
 * byte of heap sounds about right to get exhaustive coverage without
 * blowing too many cycles
 */
#define ITERATION_COUNT (2 * SMALL_HEAP_SZ)

/* Simple dumb hash function of the size and address */
static size_t fill_token(void *p, size_t sz)
{
	size_t pi = (size_t) p;

	return (pi * sz) ^ ((sz ^ 0xea6d) * ((pi << 11) | (pi >> 21)));
}

/* Puts markers at the start and end of a block to ensure that nothing
 * scribbled on it while it was allocated.  The first word is the
 * block size.  The second and last (if they fits) are a hashed "fill
 * token"
 */
static void fill_block(void *p, size_t sz)
{
	if (p == NULL) {
		return;
	}

	size_t tok = fill_token(p, sz);

	((size_t *)p)[0] = sz;

	if (sz >= 2 * sizeof(size_t)) {
		((size_t *)p)[1] = tok;
	}

	if (sz > 3*sizeof(size_t)) {
		((size_t *)p)[sz / sizeof(size_t) - 1] = tok;
	}
}

/* Checks markers just before freeing a block */
static void check_fill(void *p)
{
	size_t sz = ((size_t *)p)[0];
	size_t tok = fill_token(p, sz);

	zassert_true(sz > 0, "");

	if (sz >= 2 * sizeof(size_t)) {
		zassert_true(((size_t *)p)[1] == tok, "");
	}

	if (sz > 3 * sizeof(size_t)) {
		zassert_true(((size_t *)p)[sz / sizeof(size_t) - 1] == tok, "");
	}
}

void *testalloc(void *arg, size_t bytes)
{

	if (current_allocs < PRINT_ALLOCS) {
		TC_PRINT("%s: ", __func__);
		current_allocs++;
	}

	void *ret = sys_heap_alloc(arg, bytes);

	if (ret != NULL) {
		/* White box: the heap internals will allocate memory
		 * in 8 chunk units, no more than needed, but with a
		 * header prepended that is 4 or 8 bytes.  Use this to
		 * validate the block_size predicate.
		 */
		size_t blksz = sys_heap_usable_size(arg, ret);
		size_t addr = (size_t) ret;
		size_t chunk = ROUND_DOWN(addr - 1, 8);
		size_t hdr = addr - chunk;
#ifdef __CHERI_PURE_CAPABILITY__
		/* cheri modified rounds up to cap size(WB_UP) */
		size_t expect = ROUND_UP(__builtin_cheri_round_representable_length(
						WB_UP(bytes)) + hdr, 8) - hdr;
		size_t base = __builtin_cheri_base_get(ret);
		/* check addr is aligned with base */
		zassert_equal(base, addr,
			      "cheri base %zu not aligned to addr = %p",
			      base, addr);
#else
		size_t expect = ROUND_UP(bytes + hdr, 8) - hdr;
#endif
		if (current_allocs < PRINT_ALLOCS) {
			TC_PRINT("bytes = %zu expect = %zu actual = %zu\n",
			      bytes, expect, blksz);
		}

		zassert_equal(blksz, expect,
			      "wrong size block returned bytes = %zu expect = %zu actual = %zu",
			      bytes, expect, blksz);
	}

	if (current_allocs < PRINT_ALLOCS) {
		print_cheri("", ret);
	}
	fill_block(ret, bytes);
	sys_heap_validate(arg);
	return ret;
}

void *testalloc_aligned(void *heap, size_t bytes)
{
	/* set min alignment */
	size_t align = 1;

	/* allocate requested memory */
	void *blk = sys_heap_aligned_alloc(heap, align, bytes);

	/* perform check */
	size_t addr = (size_t) blk;
	size_t chunk = ROUND_DOWN(addr - 1, 8);
	size_t hdr = addr - chunk;

	size_t blksz = sys_heap_usable_size(heap, blk);

#ifdef __CHERI_PURE_CAPABILITY__
	/* cheri modified rounds up to cap size(WB_UP) */
	size_t expect = ROUND_UP(__builtin_cheri_round_representable_length(
				WB_UP(bytes)) + hdr, 8) - hdr;
	size_t base = __builtin_cheri_base_get(blk);
	/* check addr is aligned with base */
	zassert_equal(base, addr,
		"cheri base %zu not aligned to addr = %p",
		base, addr);
#else
	size_t expect = ROUND_UP(bytes + hdr, 8) - hdr;
#endif
	TC_PRINT("bytes = %zu expect = %zu actual = %zu\n",
			      bytes, expect, blksz);
	zassert_equal(expect, blksz,
		"expected size %zu not equal to actual = %p",
		expect, blksz);

	return blk;
}

void *testalloc_non_aligned(void *heap, size_t bytes)
{
	/* allocate requested memory */
	void *blk = sys_heap_alloc(heap, bytes);

	/* perform check */
	size_t addr = (size_t) blk;
	size_t chunk = ROUND_DOWN(addr - 1, 8);
	size_t hdr = addr - chunk;

	size_t blksz = sys_heap_usable_size(heap, blk);

#ifdef __CHERI_PURE_CAPABILITY__
	/* cheri modified rounds up to cap size(WB_UP) */
	size_t expect = ROUND_UP(__builtin_cheri_round_representable_length(
				WB_UP(bytes)) + hdr, 8) - hdr;
	size_t base = __builtin_cheri_base_get(blk);
	/* check addr is aligned with base */
	zassert_equal(base, addr,
		"cheri base %zu not aligned to addr = %p",
		base, addr);
#else
	size_t expect = ROUND_UP(bytes + hdr, 8) - hdr;
#endif
	TC_PRINT("bytes = %zu expect = %zu actual = %zu\n",
			      bytes, expect, blksz);
	zassert_equal(expect, blksz,
		"expected size %zu not equal to actual = %p",
		expect, blksz);

	return blk;
}

void testfree(void *arg, void *p)
{
	check_fill(p);
	sys_heap_free(arg, p);
	sys_heap_validate(arg);
}

static void log_result(size_t sz, struct z_heap_stress_result *r)
{
	uint32_t tot = r->total_allocs + r->total_frees;
	uint32_t avg = (uint32_t)((r->accumulated_in_use_bytes + tot/2) / tot);
	uint32_t avg_pct = (uint32_t)((100ULL * avg + sz / 2) / sz);
	uint32_t succ_pct = ((100ULL * r->successful_allocs + r->total_allocs / 2)
			  / r->total_allocs);

	TC_PRINT("successful allocs: %d/%d (%d%%), frees: %d,"
		 "  avg usage: %d/%d (%d%%)\n",
		 r->successful_allocs, r->total_allocs, succ_pct,
		 r->total_frees, avg, (int) sz, avg_pct);
}

/* Helper functions for the tests */

/* helper print functions */
void print_buff_heap_start(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking heap backing buffers are 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking heap backing buffers are 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif /* CONFIG_64BIT */

#else
	TC_PRINT("Non-CHERI mode, no bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void print_init_heap_start(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking heap is 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking heap is 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif /* CONFIG_64BIT */

#else
	TC_PRINT("Non-CHERI mode, no bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void assert_buffer_cheri(void *buffer, size_t buffer_size)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t actual_buffer_len = __builtin_cheri_length_get(buffer);
	size_t expected_buffer_len =
		__builtin_cheri_round_representable_length(buffer_size);

	size_t expected_buffer_align =
		~(__builtin_cheri_representable_alignment_mask(buffer_size)) + 1;
	uintptr_t actual_buffer_addr = (uintptr_t)buffer;

	zassert_equal(actual_buffer_len, expected_buffer_len,
		      "\nFailed %s - actual heap length %zu and "
		      "expected heap length %zu not the same", __func__,
		      actual_buffer_len, expected_buffer_len);

	zassert_true((actual_buffer_addr % expected_buffer_align) == 0,
		"buffer %p is not aligned to %zu bytes",
		buffer, expected_buffer_align);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/* Simple clobber detection */
void realloc_fill_block(uint8_t *p, size_t sz)
{
	uint8_t val = (uint8_t)((uintptr_t)p >> 3);

	for (int i = 0; i < sz; i++) {
		p[i] = (uint8_t)(val + i);
	}
}

bool realloc_check_block(uint8_t *data, uint8_t *orig, size_t sz)
{
	uint8_t val = (uint8_t)((uintptr_t)orig >> 3);

	for (int i = 0; i < sz; i++) {
		if (data[i] != (uint8_t)(val + i)) {
			return false;
		}
	}
	return true;
}

/* clobber deection with pointers/caps */
void realloc_fill_block_ptr(uint8_t *p, size_t sz)
{
	/* create dummy ptr */
	int dummy = 6;
	void *ptr = &dummy;

	TC_PRINT("dummy pointer: %p\n", ptr);

	/* Treat the memory as an array of pointer slots */
	size_t nslots = sz / sizeof(void *);
	void **slots = (void **)p;

	/* stores a full pointer/capability, tag preserved on CHERI */
	for (size_t i = 0; i < nslots; i++) {
		slots[i] = ptr;
	}
}

bool realloc_check_block_ptr(uint8_t *p, size_t sz)
{
	void *ptr;

	/* Treat the memory as an array of pointer slots */
	size_t nslots = sz / sizeof(void *);
	void **slots = (void **)p;

	for (size_t i = 0; i < nslots; i++) {
		ptr = slots[i];
		print_cheri("ptr read:\n", ptr);
#ifdef __CHERI_PURE_CAPABILITY__
		if (__builtin_cheri_tag_get(ptr) != 1) {
			return false;
		}
#else
		TC_PRINT("dummy pointer read: %p\n", ptr);
#endif
	}
	return true;
}

/**
 * @brief test backing buffer allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the backing buffers. Asserts these details against
 * what is expected.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_a_backing_buffer_bounds)
{
	print_buff_heap_start();

	TC_PRINT("heapmem size: %zu bytes\n", (size_t)BIG_HEAP_SZ);
	TC_PRINT("heapmem start: %p\n", heapmem);
	print_cheri("CHERI - heapmem:\n", &heapmem);
	assert_buffer_cheri(heapmem, BIG_HEAP_SZ);

	TC_PRINT("scratchmem size: %zu bytes\n", (size_t)SCRATCH_SZ);
	TC_PRINT("scratchmem start: %p\n", scratchmem);
	print_cheri("CHERI - scratchmem:\n", &scratchmem);
	assert_buffer_cheri(scratchmem, SCRATCH_SZ);
}

/**
 * @brief test init heap CHERI bounds
 *
 * @details Verify that the CHERI bounds of the heap
 * are set corectly. Asserts these details against
 * what is expected.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_b_init_heap_bounds)
{
	struct sys_heap heap;

	print_init_heap_start();

	TC_PRINT("Testing small (%d byte) heap\n", (int) SMALL_HEAP_SZ);

	/* During sys_heap_init the CHERI bounds of heapmem is transferred to heap
	 * and reduced to match SMALL_HEAP_SZ rounded up to CHERI rep length
	 */
	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");

	TC_PRINT("heap header: = %zu bytes\n", sizeof(struct z_heap));
	TC_PRINT("heap.heap:%p\n", heap.heap);
	TC_PRINT("heap.heap.end_chunk:%d chunks (8bytes)\n", heap.heap->end_chunk);
	print_cheri("heap:\n", heap.heap);

	assert_buffer_cheri(heap.heap, SMALL_HEAP_SZ);
}

/**
 * @brief test aligned allocation CHERI bounds
 *
 * @details Verify the CHERI bounds of an aligned
 * allocated block of memory. Asserts these details
 * against what is expected.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_c_aligned_alloc_bounds)
{
	struct sys_heap heap;

	print_init_heap_start();

	TC_PRINT("Testing small (%d byte) heap\n", (int) SMALL_HEAP_SZ);

	/* During sys_heap_init the CHERI bounds of heapmem is transferred to heap
	 * and reduced to match SMALL_HEAP_SZ rounded up to CHERI rep length
	 */
	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");

	TC_PRINT("heap header: = %zu bytes\n", sizeof(struct z_heap));
	TC_PRINT("heap.heap:%p\n", heap.heap);
	TC_PRINT("heap.heap.end_chunk:%d chunks (8bytes)\n", heap.heap->end_chunk);
	print_cheri("heap:\n", heap.heap);

	assert_buffer_cheri(heap.heap, SMALL_HEAP_SZ);

	void *block = testalloc_aligned(&heap, 1);

	TC_PRINT("block:%p\n", block);
	print_cheri("block:\n", block);
	assert_buffer_cheri(block, sizeof(void *));
}

/**
 * @brief test non-aligned allocation CHERI bounds
 *
 * @details Verify the CHERI bounds of a non-aligned
 * allocated block of memory. Asserts these details
 * against what is expected.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_d_non_aligned_alloc_bounds)
{
	struct sys_heap heap;

	print_init_heap_start();

	TC_PRINT("Testing small (%d byte) heap\n", (int) SMALL_HEAP_SZ);

	/* During sys_heap_init the CHERI bounds of heapmem is transferred to heap
	 * and reduced to match SMALL_HEAP_SZ rounded up to CHERI rep length
	 */
	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");

	TC_PRINT("heap header: = %zu bytes\n", sizeof(struct z_heap));
	TC_PRINT("heap.heap:%p\n", heap.heap);
	TC_PRINT("heap.heap.end_chunk:%d chunks (8bytes)\n", heap.heap->end_chunk);
	print_cheri("heap:\n", heap.heap);

	assert_buffer_cheri(heap.heap, SMALL_HEAP_SZ);

	void *block = testalloc_non_aligned(&heap, 511);

	TC_PRINT("block:%p\n", block);
	print_cheri("block:\n", block);
	assert_buffer_cheri(block, 512);
}

/**
 * @brief perform a stress test with CHERI
 *
 * @details Do a heavy test over a small heap,
 * with many iterations that need to reuse memory
 * repeatedly.  Target 50% fill, as that setting
 * tends to prevent runaway fragmentation and most
 * allocations continue to succeed in steady state.
 *
 * @ingroup cheri_sys_heap_tests
 */

ZTEST(cheri_lib_heap, test_d_small_heap)
{
	struct sys_heap heap;
	struct z_heap_stress_result result;

	TC_PRINT("Testing small (%d byte) heap\n", (int) SMALL_HEAP_SZ);

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");
	sys_heap_stress(testalloc, testfree, &heap,
			SMALL_HEAP_SZ, ITERATION_COUNT,
			scratchmem, sizeof(scratchmem),
			50, &result);

	log_result(SMALL_HEAP_SZ, &result);
}

/**
 * @brief test re-allocation with CHERI
 *
 * @details Verify when and went can't
 * reallocate memory in-place with
 * CHERI. Fill blocks with data.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_e_realloc)
{
	struct sys_heap heap;
	void *p1, *p2, *p3;

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);

	/* Allocate from an empty heap, then do small expand to fit in bounds
	 * validate that it happens in place. (within a chunk.)
	 */
	TC_PRINT("checking small inplace_realloc...\n");
	size_t bytes1 = 60;
	size_t bytes2 = 64;

	p1 = sys_heap_alloc(&heap, bytes1);
	realloc_fill_block(p1, bytes1);
	p2 = sys_heap_realloc(&heap, p1, bytes2);
	zassert_true(sys_heap_validate(&heap), "invalid heap");

	TC_PRINT("bytes to allocate: %zu\n", bytes1);
	print_cheri("p1:\n", p1);
	TC_PRINT("bytes to re-allocate: %zu\n", bytes2);
	print_cheri("p2:\n", p2);

	/* For CHERI we only expand / shrink in place for byte requests that
	 * still fit in the current bounds.
	 */
	zassert_true(p1 == p2,
		     "Realloc should have expanded in place %p -> %p",
		     p1, p2);

	zassert_true(realloc_check_block(p2, p1, 60), "data changed");


	/* Allocate, then shrink.  */
	TC_PRINT("checking large realloc shrink...\n");
	bytes1 = 128;
	bytes2 = 64;
	p1 = sys_heap_alloc(&heap, bytes1);
	realloc_fill_block(p1, bytes1);
	p2 = sys_heap_realloc(&heap, p1, bytes2);

	TC_PRINT("bytes to allocate: %zu\n", bytes1);
	print_cheri("p1:\n", p1);
	TC_PRINT("bytes to re-allocate: %zu\n", bytes2);
	print_cheri("p2:\n", p2);

	zassert_true(sys_heap_validate(&heap), "invalid heap");

	zassert_true(p1 == p2,
		     "Realloc should have shrunk in place %p -> %p",
		     p1, p2);

	zassert_true(realloc_check_block(p2, p1, 64), "data changed");

	/* Corner case with sys_heap_aligned_realloc() on 32-bit targets
	 * where actual memory doesn't match with given pointer
	 * (align_gap != 0).
	 */
	TC_PRINT("checking realloc expand...\n");
	p1 = sys_heap_aligned_alloc(&heap, 8, 32);
	realloc_fill_block(p1, 32);
	p2 = sys_heap_alloc(&heap, 32);
	realloc_fill_block(p2, 32);

	p3 = sys_heap_aligned_realloc(&heap, p1, 8, 36);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(realloc_check_block(p3, p1, 32), "data changed");
	zassert_true(realloc_check_block(p2, p2, 32), "data changed");

	print_cheri("p1:\n", p1);
	print_cheri("p3:\n", p3);

	realloc_fill_block(p3, 36);

	zassert_true(sys_heap_validate(&heap), "invalid heap");

	/* For CHERI we don't adjust in place for byte requests that
	 * don't fit in the current bounds, instead we force a normal
	 * allocation. This is also true for the corner case.
	 */

	zassert_true(p1 != p3,
		     "Realloc should have moved %p", p1);

	/* Test realloc with increasing alignment */
	TC_PRINT("checking alignment test...\n");
	p1 = sys_heap_aligned_alloc(&heap, 32, 32);
	p2 = sys_heap_aligned_alloc(&heap, 8, 32);
	p3 = sys_heap_aligned_realloc(&heap, p2, 8, 16);

	print_cheri("p1:\n", p1);
	print_cheri("p2:\n", p2);
	print_cheri("p3:\n", p3);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p2 == p3,
		     "Realloc should have expanded in place %p -> %p",
		     p2, p3);

	p3 = sys_heap_aligned_alloc(&heap, 32, 8);

	TC_PRINT("checking p3 allocation...\n");
	print_cheri("p3:\n", p3);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
#ifdef __CHERI_PURE_CAPABILITY__
	/* For CHERI, during realloc p2 is freed because is not done in place
	 * p2 addr and chunks are free to be reallocated so could be
	 * reallocated during p3 second assignment, where p2 = p3
	 * but also could be diff p2 != p3, both cases valid.
	 */
#else
	zassert_true(p2 != p3,
		     "Realloc should have moved %p", p2);
#endif
}

/**
 * @brief test writing CHERI capabilities
 *
 * @details Verify capability alignment.
 * Fill blocks with pointers/caps
 * and test for capability alignment
 * when writing and reading capabilities.
 *
 * @ingroup cheri_sys_heap_tests
 */
ZTEST(cheri_lib_heap, test_f_realloc_ptr)
{
	struct sys_heap heap;
	void *p1, *p2, *p3;

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);

	/* Allocate from an empty heap, then do small expand to fit in bounds
	 * fill block with pointers /caps and check valid when read
	 * them back out.
	 */
	TC_PRINT("checking writing pointers in block...\n");
	size_t bytes1 = 60;
	size_t bytes2 = 64;

	p1 = sys_heap_alloc(&heap, bytes1);
	realloc_fill_block_ptr(p1, bytes1);
	p2 = sys_heap_realloc(&heap, p1, bytes2);
	zassert_true(sys_heap_validate(&heap), "invalid heap");

	/* For CHERI we only expand / shrink in place for byte requests that
	 * still fit in the current bounds.
	 */
	zassert_true(p1 == p2,
		     "Realloc should have expanded in place %p -> %p",
		     p1, p2);

	zassert_true(realloc_check_block_ptr(p2, bytes1), "pointer not valid");
}

ZTEST_SUITE(cheri_lib_heap, NULL, NULL, NULL, NULL, NULL);
