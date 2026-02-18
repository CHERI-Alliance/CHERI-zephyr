/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* CHERI helper function for printing inline */

static inline void print_cheri(const char *label, void *cap)
{
#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);
	size_t cap_len  = __builtin_cheri_length_get(cap);
	size_t tag = __builtin_cheri_tag_get(cap);

	printk("%s", label);
	printk("CHERI addr:   0x%lx\n", (unsigned long)cap_addr);
	printk("CHERI base:   0x%lx\n", (unsigned long)cap_base);
	printk("CHERI top:   0x%lx\n", (unsigned long)cap_base+cap_len);
	printk("CHERI length: %zu\n", cap_len);
	printk("CHERI tag: %zu\n", tag);
	printk("bytes before end of stack: %lu\n", (unsigned long)(cap_base+cap_len-cap_addr));
#endif /* __CHERI_PURE_CAPABILITY__ */
}
