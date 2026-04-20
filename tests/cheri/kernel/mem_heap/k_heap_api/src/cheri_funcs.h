/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* CHERI helper function for printing inline for performance and no call overhead */

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

static void assert_cheri(const char *label, void *cap, size_t req_len)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t cap_tag  = __builtin_cheri_tag_get(cap);
	size_t req_len_round = __builtin_cheri_round_representable_length(req_len);

	zassert_equal(cap_addr, cap_base, "%s bounds base not aligned to addr", label);
	zassert_equal(cap_len, req_len_round, "%s not bounded to correct length", label);
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
