/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#ifdef __CHERI_PURE_CAPABILITY__

/* CHERI helper function for printing inline for performance and no call overhead */

static inline void print_cheri(const char *label, void *cap)
{
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
}

#endif /* __CHERI_PURE_CAPABILITY__ */
