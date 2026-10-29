/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_internal.h>

#include "func_defs.h"

/*
 * CHERI helper assert functions
 *
 * These assert functions do CHERI bounds and alignment checks
 */

#ifdef __CHERI_PURE_CAPABILITY__

/* CHERI helper functions for printing and asserting bounds information */

/*
 * Note: not created as inline functions due to suspect compiler / qemu bug
 * returning __builtin_cheri_round_representable_length values
 * from incorrect arch when used inline.
 */

/* check single stack bounds */
void assert_single_stack(char *cap,
			size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);

	size_t size_expected = __builtin_cheri_round_representable_length((size_t)size_bytes);

	zassert_equal(cap_len, size_expected, "bounds of stack not match expected stack size");
	zassert_equal(cap_addr, cap_base, "address not aligned to base");
}

/* check single stack bounds for sysheap*/
/*
 * For sys heap allocation the exact bounds returned
 * is not deterministic because fragmentation
 * varies and what memory blocks are available
 * for allocation.
 * As a sanity check we can still check the bounds:
 * (1) should be at least cheri representable size
 * (should also be less than the sysheap size, but will fail long before reaching here)
 * (3) we can also check the alignment of the returned address
 */
void assert_single_stack_sysheap(char *cap,
			size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);

	/* minimum cheri representable size */
	size_t size_minimum_expected = __builtin_cheri_round_representable_length(
					(size_t)size_bytes);
	size_t align_minimum_expected = ~(__builtin_cheri_representable_alignment_mask(
					size_minimum_expected)) + 1;

	printk("size_minimum_expected: %zu\n", (size_t)size_minimum_expected);
	printk("align_minimum_expected: %zu\n", (size_t)align_minimum_expected);
	zassert_true(cap_len >= size_minimum_expected, "bounds of stack too small");

	zassert_true((cap_addr % align_minimum_expected) == 0,
		"base 0x%lx not aligned to %zu",
		cap_addr, align_minimum_expected);
}


/* check csp tightened bounds of stack in running thread */
/* address not likely be same as base since active csp */
void assert_thread_stack(char *cap,
			size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);

	zassert_equal(cap_len, size_bytes, "bounds of stack not match stack size %zu, %zu",
		cap_len, size_bytes);
}

#endif
