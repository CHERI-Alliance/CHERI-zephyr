/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2026 University of Birmingham, modified to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>

/**
 * @brief Test sys_heap API with CHERI memory alignment
 *
 * @defgroup cheri_sys_heap_align_tests CHERI system heap lib align tests
 *
 * @ingroup cheri_tests
 *
 * This is a modified variation of the tests in
 * tests/lib/heap_align but are more CHERI-specific.
 */

#include "inline_funcs.h"
/* how many to print cheri bounds for */
#define PRINT_ALLOCS 2
int current_allocs = 1;


/* need to peek into some heap internals */
#include "../../../../lib/heap/heap.h"

#define HEAP_SZ 0x1000

uint8_t __aligned(CHUNK_UNIT) heapmem[HEAP_SZ];

/* Heap metadata sizes */
uint8_t *heap_start, *heap_end;
size_t heap_chunk_header_size;

/*
 * The align argument may contain a "rewind" bit.
 * See comment in sys_heap_aligned_alloc().
 */
static bool alignment_ok(void *ptr, size_t align)
{
	uintptr_t addr = (uintptr_t)ptr;
	size_t rew;

	/* split rewind bit from alignment */
	rew = LSB_GET(align);
	rew = (rew == align) ? 0 : rew;
	align -= rew;

	/* undo the pointer rewind */
	addr += rew;

	/* validate pointer alignment */
	return (addr & (align - 1)) == 0;
}

/* Note that this test is making whitebox assumptions about the
 * behavior of the heap in order to exercise coverage of the
 * underlying code: that chunk headers are 8 bytes, that heap chunks
 * are returned low-address to high, and that freed blocks are merged
 * immediately with adjacent free blocks.
 */
static void check_heap_align(struct sys_heap *h,
			     size_t prefix, size_t align, size_t size)
{
	void *p, *q, *r, *s;
	size_t suffix;

	p = sys_heap_alloc(h, prefix);
	zassert_true(prefix == 0 || p != NULL, "prefix allocation failed");

	q = sys_heap_aligned_alloc(h, align, size);
	zassert_true(q != NULL, "first aligned allocation failed");

#ifdef __CHERI_PURE_CAPABILITY__
	/* calculate the expected CHERI-alignment rounded
	 * up from the requested alignment
	 */
	size_t req_align = align;
	/* for CHERI there is a mimimum alignment in the sys_heap api */
	size_t min_align = sizeof(void *);
	/* calculate the cheri alignment from the requested size */
	size_t cheri_align_int = ~__builtin_cheri_representable_alignment_mask(WB_UP(size)) + 1;
	/* take whichever is the greater of the two alignments */
	size_t cheri_align_min = (min_align > cheri_align_int) ? min_align : cheri_align_int;
	/* take the requested alignment and round up to the nearest minimum cheri alignment */
	size_t cheri_align = ROUND_UP(req_align, cheri_align_min);

	align = cheri_align;

	/* check addr is aligned to base */
	size_t cheri_addr = __builtin_cheri_address_get(q);
	size_t cheri_base = __builtin_cheri_base_get(q);

	/* print out the first few, or if alignment fails */

	if ((cheri_addr != cheri_base) ||
	 (alignment_ok(q, align) == false) ||
	 (current_allocs < PRINT_ALLOCS)) {
		TC_PRINT("-------------Alignment-------------\n");
		print_cheri("q:\n", q);
		TC_PRINT("requested align: %zu\n", req_align);
		TC_PRINT("min_align: %zu\n", min_align);
		TC_PRINT("cheri_align_int: %zu\n", cheri_align_int);
		TC_PRINT("cheri_align_min: %zu\n", cheri_align_min);
		TC_PRINT("expected cheri align: %zu\n", cheri_align);
	}
	zassert_equal(cheri_addr, cheri_base, "cheri_addr != cheri_base");
#endif

	zassert_true(alignment_ok(q, align), "block not aligned");

	r = sys_heap_aligned_alloc(h, align, size);
	zassert_true(r != NULL, "second aligned allocation failed");
	zassert_true(alignment_ok(r, align), "block not aligned");

	/* Make sure ALL the split memory goes back into the heap and
	 * we can allocate the full remaining suffix
	 */
#ifdef __CHERI_PURE_CAPABILITY__
	/* when the memory is allocated for suffix it is aligned and rounded up to the
	 * CHERI representable length. This means trying to allocate the suffix directly
	 * might not actually fit into the remaining space once this has been accounted for.
	 * i.e rounded_up_suffix = __builtin_cheri_round_representable_length(WB_UP(suffix));
	 * plus alignment
	 * if we round down to fit there might still be a bit of memory left at the end.
	 *
	 * To work out the maximum size that can be allocated we need to
	 * 1. find the alignment and sub to lower boundary
	 * 2. round up the start to the alignment
	 * 3. round down the end to the alignment and
	 * 4. calc the size from end - start
	 *
	 * This means we can't really do the tests for CHERI in the same way as non-cheri.
	 * (where the test assumes suffix can be allocated and then the heap is empty.)
	 * Also it doesn't really make sense to do the above calculation because it doesn't
	 * really test anything CHERI-specific.
	 * what we can do here is check the suffix length assumed from the calculation
	 * with the suffix length measured from getting the cheri bounds of r
	 * this allows us to perform some extra bounds checks
	 */

	/* Calculate the suffix.
	 * For CHERI the size is rounded up firstly to fit a cap pointer (WB_UP),
	 * and then to CHERI rep length. It is then rounded up to chunk size
	 * Using r already takes into account any CHERI alignment.
	 */
	size_t cheri_round_size = __builtin_cheri_round_representable_length(WB_UP(size));
	uint8_t *round_up_addr = (uint8_t *)ROUND_UP((uintptr_t)r + cheri_round_size, CHUNK_UNIT);

	suffix = (heap_end - round_up_addr) - heap_chunk_header_size;

	/* Calculate the suffix from bounds */
	uint8_t *addr_from_bounds = (uint8_t *)((uintptr_t)r + __builtin_cheri_length_get(r));
	size_t suffix_from_bounds = heap_end - addr_from_bounds -  heap_chunk_header_size;

	/* print out the first few or if suffix check fails */
	if (suffix != suffix_from_bounds ||
	 (current_allocs < PRINT_ALLOCS)) {
		TC_PRINT("-------------Suffix-------------\n");
		TC_PRINT("size: %zu\n", size);
		TC_PRINT("cheri_round_size: %zu\n", cheri_round_size);
		TC_PRINT("round_up_addr: %p\n", round_up_addr);
		print_cheri("r:\n", r);
		TC_PRINT("heap_end: %p\n", heap_end);
		TC_PRINT("heap_chunk_header_size: %zu\n", heap_chunk_header_size);
		TC_PRINT("addr_from_bounds: %p\n", addr_from_bounds);
		TC_PRINT("suffix: %zu\n", suffix);
		TC_PRINT("suffix_from_bounds: %zu\n", suffix_from_bounds);
		current_allocs++;
	}
	zassert_equal(suffix, suffix_from_bounds, "suffix != suffix_from_bounds");
#else
	suffix = (heap_end - (uint8_t *)ROUND_UP((uintptr_t)r + size, CHUNK_UNIT))
		- heap_chunk_header_size;

	s = sys_heap_alloc(h, suffix);
	zassert_true(s != NULL, "suffix allocation failed (%zd/%zd/%zd)",
				prefix, align, size);
	zassert_true(sys_heap_validate(h), "heap invalid");
#endif
	sys_heap_free(h, p);
	sys_heap_free(h, q);
	sys_heap_free(h, r);
#ifndef __CHERI_PURE_CAPABILITY__
	sys_heap_free(h, s);

	/* Make sure it's still valid, and empty */
	zassert_true(sys_heap_validate(h), "heap invalid");
	p = sys_heap_alloc(h, heap_end - heap_start);
	zassert_true(p != NULL, "heap not empty");
	q = sys_heap_alloc(h, 1);
	zassert_true(q == NULL, "heap not full");
	sys_heap_free(h, p);
#endif
}

/**
 * @brief alignment and suffix testing with CHERI
 *
 * @details Verify that the system aligns memory
 * blocks correctly with CHERI. Asserts this against
 * what is expected.
 *
 * @ingroup cheri_sys_heap_align_tests
 */
ZTEST(cheri_lib_heap_align, test_aligned_alloc)
{
	struct sys_heap heap = {};
	void *p, *q;

	sys_heap_init(&heap, heapmem, HEAP_SZ);

	p = sys_heap_alloc(&heap, 1);
	zassert_true(p != NULL, "initial alloc failed");
	sys_heap_free(&heap, p);

	/* Heap starts where that first chunk was, and ends one 8-byte
	 * chunk header before the end of its memory
	 */
	heap_start = p;
	heap_end = heapmem + heap.heap->end_chunk * CHUNK_UNIT;
	heap_chunk_header_size = chunk_header_bytes(heap.heap);

	for (size_t align = 8; align < HEAP_SZ / 4; align *= 2) {
		for (size_t prefix = 0; prefix <= align; prefix += 8) {
			for (size_t size = 4; size <= align; size += 12) {
				check_heap_align(&heap, prefix, align, size);
				for (size_t rew = 4; rew < MIN(align, 32); rew *= 2) {
					check_heap_align(&heap, prefix,
							 align | rew, size);
				}
			}
		}
	}

	/* corner case on small heaps */
	p = sys_heap_aligned_alloc(&heap, 8, 12);
	memset(p, 0, 12);
	zassert_true(sys_heap_validate(&heap), "heap invalid");
	sys_heap_free(&heap, p);

	/* corner case with minimizing the overallocation before alignment */
	p = sys_heap_aligned_alloc(&heap, 16, 16);
	q = sys_heap_aligned_alloc(&heap, 16, 17);
	memset(p, 0, 16);
	memset(q, 0, 17);
	zassert_true(sys_heap_validate(&heap), "heap invalid");
	sys_heap_free(&heap, p);
	sys_heap_free(&heap, q);
}

ZTEST_SUITE(cheri_lib_heap_align, NULL, NULL, NULL, NULL, NULL);
