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

	zassert_equal(cap_len, size_bytes, "bounds of stack not match stack size");
	zassert_equal(cap_addr, cap_base, "address not aligned to base");
}

/*
 * check stack array bounds
 * bounds cover all elements of array
 * address depends on which array element is passed
 */
void assert_array_stack(int index,
			char *cap,
			size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);

	/*
	 * CHERI round up element size_bytes * NUM_STACKS.
	 * This is because the compiler rounds up macro array length to be a
	 * CHERI representable size, so we must do the same here in our test
	 */
	size_t expected_cap_len =
		__builtin_cheri_round_representable_length(size_bytes * NUM_STACKS);

	zassert_equal(cap_len, expected_cap_len,
		"bounds of stack not match expected size %zu, %zu",
			cap_len, expected_cap_len);
	zassert_equal(cap_addr, cap_base + (size_bytes * index),
		"address not aligned to expected");
}

/*
 * check stack array bounds
 * bounds cover all elements of array (CPUs)
 * address depends on which array element (CPU) is passed
 */
void assert_array_stack_idle(int index,
			char *cap,
			size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);
	uintptr_t cap_addr = __builtin_cheri_address_get(cap);
	uintptr_t cap_base = __builtin_cheri_base_get(cap);

	/*
	 * CHERI round up element size_bytes * NUM_CPUS.
	 * This is because the compiler rounds up macro array length to be a
	 * CHERI representable size, so we must do the same here in our test
	 */
	size_t expected_cap_len = __builtin_cheri_round_representable_length(size_bytes * NUM_CPUS);

	zassert_equal(cap_len, expected_cap_len,
		"bounds of stack not match expected size %zu, %zu",
		cap_len, expected_cap_len);
	zassert_equal(cap_addr, cap_base + (size_bytes * index),
		"address not aligned to expected");
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
