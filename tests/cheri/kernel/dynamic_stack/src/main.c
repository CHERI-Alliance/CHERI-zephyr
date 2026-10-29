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

/**
 * @brief Test zephyr dynamic stacks with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_dynamic_stack_tests CHERI Stack Dynamic Tests
 *
 * @ingroup cheri_tests
 *
 * This module tests the following stacks with CHERI
 *
 *     dynamic stacks allocated from a heap pool
 *     dynamic stacks allocated from the system heap
 *     using (dynamic.c)
 *
 */

/*
 * These tests validate that CHERI capability bounds are applied correctly
 * to dynamic stacks and when used in a thread.
 * Dynamic stacks are defined using either a stack heap pool via
 * K_HEAP_DEFINE, (CONFIG_DYNAMIC_THREAD_PREFER_POOL)
 * or from the system heap (CONFIG_DYNAMIC_THREAD_PREFER_ALLOC).
 * These are tested with:
 * test_dynamic_stack.c
 *   dynamic_stack_bounds_cheri_pool()
 *   dynamic_stack_bounds_cheri_sysheap()
 *
 * Tests are based on some tests from
 * tests/kernel/threads/dynamic_thread_stack and
 * tests/cheri/kernel/stack with CHERI specific additions.
 * It also tests different stack sizes for CHERI rounding.
 */

/**
 * @brief Show that the stack bounds are correct for CHERI
 *
 * Print and assert CHERI bounds of stacks allocated from a
 * heap pool.
 *
 * @ingroup cheri_dynamic_stack_tests
 */
ZTEST(cheri_dynamic_stack, test_dynamic_pool_stack_bounds_cheri)
{
dynamic_stack_bounds_cheri_pool();
}

/**
 * @brief Show that the stack bounds are correct for CHERI
 *
 * Print and assert CHERI bounds of stacks allocated from
 * the system heap.
 *
 * @ingroup cheri_dynamic_stack_tests
 */
ZTEST(cheri_dynamic_stack, test_dynamic_sysheap_stack_bounds_cheri)
{
dynamic_stack_bounds_cheri_sysheap();
}

ZTEST_SUITE(cheri_dynamic_stack, NULL, NULL,
		NULL, NULL, NULL);
