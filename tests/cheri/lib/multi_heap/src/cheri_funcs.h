/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* CHERI helper functions for printing and checks for twister tests */

static inline void print_cheri(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t tag = __builtin_cheri_tag_get(cap);

	TC_PRINT("%s", label);
	TC_PRINT("CHERI addr:   0x%lx\n", (unsigned long)cap_addr);
	TC_PRINT("CHERI base:   0x%lx\n", (unsigned long)cap_base);
	TC_PRINT("CHERI top:   0x%lx\n", (unsigned long)cap_base+cap_len);
	TC_PRINT("CHERI length: %zu\n", cap_len);
	TC_PRINT("CHERI tag: %zu\n", tag);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/* mempool assertions */

static void assert_cheri_mempool(const char *label, void *cap, size_t req_len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t cap_tag  = __builtin_cheri_tag_get(cap);
	/* round up to ptr/cap size */
	size_t req_len_round_up = WB_UP(req_len);
	/* round up to CHERI requirements, include the header */
	size_t req_len_round = __builtin_cheri_round_representable_length
		(req_len_round_up + sizeof(void *));

	/* For the mempool api the usable data is preceded by a header
	 * which is a pointer to a structure k_heap
	 */
	zassert_equal(cap_addr, cap_base + sizeof(void *),
		"%s bounds base or addr not as expected", label);
	zassert_equal(cap_len, req_len_round,
		"%s not bounded %zu to req. length %zu",
			label, cap_len, req_len_round);
	zassert_not_equal(cap_tag, 0, "%s cheri tag failed", label);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_mempool_align(const char *label, void *cap, size_t req_len, size_t align)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t cap_tag  = __builtin_cheri_tag_get(cap);
	size_t req_len_round = WB_UP(req_len);

	req_len_round = __builtin_cheri_round_representable_length(req_len_round + sizeof(void *));

	/* take into account alignment requests */
	size_t ptr_align = sizeof(void *);
	size_t effective_align = MAX(align, ptr_align);

	/* For the mempool api the usable data is preceded by a header
	 * which is a pointer to a structure k_heap
	 */

	/* cap_addr must be aligned to the effective alignment */
	zassert_equal(cap_addr & (effective_align - 1), 0,
		"%s cap addr not aligned to %zu",
		label, effective_align);

	/* header must be immediately before cap_addr */
	zassert_equal((cap_addr - sizeof(void *)) & (ptr_align - 1), 0,
		"%s header not pointer-aligned",
		label);

	/* cap_addr must be at least header-size past the base */
	zassert_true(cap_addr >= cap_base + sizeof(void *),
		"%s cap addr before header space",
		label);

	zassert_not_equal(cap_tag, 0, "%s cheri tag failed", label);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_invalid_tag(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cap_tag  = __builtin_cheri_tag_get(cap);

	zassert_equal(cap_tag, 0, "%s cheri tag unexpectedly succeeded", label);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_valid_tag(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	size_t cap_tag  = __builtin_cheri_tag_get(cap);

	zassert_equal(cap_tag, 1, "%s cheri invalid tag", label);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_no_overlap(const char *label, void *cap1, void *cap2)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_base1 = __builtin_cheri_base_get(cap1);
	size_t cap_top1  = cap_base1 + __builtin_cheri_length_get(cap1);
	uintptr_t cap_base2 = __builtin_cheri_base_get(cap2);
	size_t cap_top2  = cap_base2 + __builtin_cheri_length_get(cap2);

	bool no_overlap = (cap_top2 <= cap_base1) ||
				(cap_base2 >= cap_top1);
	zassert_true(no_overlap, "%s bounds overlap detected", label);

#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_overlap(const char *label, void *cap1, void *cap2)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_base1 = __builtin_cheri_base_get(cap1);
	size_t cap_top1  = cap_base1 + __builtin_cheri_length_get(cap1);
	uintptr_t cap_base2 = __builtin_cheri_base_get(cap2);
	size_t cap_top2  = cap_base2 + __builtin_cheri_length_get(cap2);

	bool no_overlap = (cap_top2 <= cap_base1) ||
				(cap_base2 >= cap_top1);
	zassert_false(no_overlap, "%s bounds overlap not detected", label);

#endif /* __CHERI_PURE_CAPABILITY__ */
}
static void assert_cheri_ptr_aligned(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t align = sizeof(void *);

	bool addr_aligned = ((cap_addr & (align - 1)) == 0);
	bool base_aligned = ((cap_base & (align - 1)) == 0);

	zassert_true(addr_aligned,
		"%s capability address is misaligned (addr=%#lx, align=%zu)",
		label, cap_addr, align);

	zassert_true(base_aligned,
		"%s capability base is misaligned (base=%#lx, align=%zu)",
		label, cap_base, align);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

static void assert_cheri_zero_write(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	/* check memory allocation is zeroed.
	 * check we can write to the contents
	 */

	/* cast to a byte pointer */
	uint8_t *p = cap;
	/* get the full length from base */
	size_t cheri_len = __builtin_cheri_length_get(cap);
	/* subtract the header pointer */
	cheri_len = cheri_len - sizeof(void *);
	/* check zeroed and write */
	for (int i = 0; i < cheri_len; i++) {
		zassert_equal(p[i], 0);
		p[i] = 1;
	}
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/* multi-heap assertions */

static void assert_cheri_singleheap(const char *label, void *cap, size_t req_len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t cap_tag  = __builtin_cheri_tag_get(cap);
	size_t req_len_round_up = WB_UP(req_len);
	size_t req_len_round = __builtin_cheri_round_representable_length
		(req_len_round_up);

	zassert_equal(cap_addr, cap_base,
		"%s bounds base and addr not aligned", label);
	zassert_equal(cap_len, req_len_round,
		"%s not bounded %zu to req. length %zu",
			label, cap_len, req_len_round);
	zassert_not_equal(cap_tag, 0, "%s cheri tag failed", label);
#endif /* __CHERI_PURE_CAPABILITY__ */
}


static void assert_cheri_within_bounds(const char *label,
					void *container,
					void *inner)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cont_base = __builtin_cheri_base_get(container);
	size_t cont_len     = __builtin_cheri_length_get(container);
	uintptr_t cont_top  = cont_base + cont_len;

	uintptr_t inner_base = __builtin_cheri_base_get(inner);
	size_t inner_len     = __builtin_cheri_length_get(inner);
	uintptr_t inner_top  = inner_base + inner_len;

	bool within =
		(inner_base >= cont_base) &&
		(inner_top  <= cont_top);

	zassert_true(within,
		"%s bounds not fully contained:\n"
		"container=[%p..%p), inner=[%p..%p)",
		label,
		(void *)cont_base, (void *)cont_top,
		(void *)inner_base, (void *)inner_top);
#endif /* __CHERI_PURE_CAPABILITY__ */
}
