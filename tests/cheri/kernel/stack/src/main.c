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
 * @brief Test zephyr stacks with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_stack_tests CHERI Stack Tests
 *
 * @ingroup cheri_tests
 *
 * This module tests the following stacks with CHERI
 *
 *     User stacks
 *     kernel stacks
 *     Idle stacks
 *   using (thread_stack.h)
 *	K_THREAD_STACK_DEFINE, K_THREAD_STACK_ARRAY_DEFINE
 *	K_KERNEL_STACK_DEFINE, K_KERNEL_STACK_ARRAY_DEFINE
 *   and tighening of bounds in
 *	arch_new_thread (arch/riscv/core/thread.c)
 *
 */

/* CHERI memory allocation alignment:
 * See CHERI C++ programming guide.
 * Due to CHERI bounds compression, not all memory allocation requests can be exactly
 * bound to the requested alignment and length. To obtain exact bounds requires
 * increasing alignment and rounding up the length.
 *
 * For CHERI, the stack macros in thread_stack.h have been modified to align and
 * round to CHERI requirements. Run-time bounds tightening of stack arrays is
 * done at run-time using the CHERI recommended builtins. This is done at thread
 * initialisation in arch_new_thread.
 *
 * test_stack_define.c
 *
 * These tests validate that CHERI capability bounds are applied correctly
 * to stacks defined using K_THREAD_STACK_DEFINE,
 * K_KERNEL_STACK_DEFINE, and their array variants. It also checks array bounds
 * are tightened on thread initialisation.
 * These are tested with:
 *   stack_bounds_cheri()
 *   stack_threads_bounds_cheri()
 *
 * test_idle_stack.c
 *
 * These tests validate that CHERI capability bounds are applied correctly
 * to idle stacks defined by the kernel using K_THREAD_STACK_DEFINE,
 * K_KERNEL_STACK_DEFINE, and their array variants. It also checks array bounds
 * are tightened on thread initialisation.
 * These are tested with:
 *   idle_stack_bounds_cheri()
 *   idle_csp_bounds_cheri()
 */

/**
 * @brief Show that the stack bounds are correct for CHERI
 *
 * Print and assert CHERI bounds of both single stacks and an array
 * of stacks. For the stack arrays the bounds will cover
 * the whole array and is created using K_KERNEL_STACK_DEFINE
 * and K_KERNEL_STACK_ARRAY_DEFINE
 *
 * @ingroup cheri_stack_tests
 */
ZTEST(cheri_stack, test_a_stack_bounds_cheri)
{
stack_bounds_cheri();
}

/**
 * @brief Show that the current thread stack bounds are tight
 *
 * Check the bounds of the current stack csp thread.
 * Here the bounds should be reduced to a single element
 * if running with SMP.
 *
 * @ingroup cheri_stack_tests
 */
ZTEST(cheri_stack, test_b_stack_threads_bounds_cheri)
{
stack_threads_bounds_cheri();
}

/**
 * @brief Show that the idle thread stack bounds is correct for CHERI
 *
 * Check the bounds of the current idle stack. If running with SMP
 * the bounds will cover the whole array and is created
 * using K_KERNEL_STACK_DEFINE & K_KERNEL_STACK_ARRAY_DEFINE
 *
 * @ingroup cheri_stack_tests
 */
ZTEST(cheri_stack, test_c_idle_stack_bounds_cheri)
{

idle_stack_bounds_cheri();
}

/**
 * @brief Show that the current idle thread stack bounds are tight
 *
 * Check the bounds of the current idle stack csp thread.
 * Here the bounds should be reduced to a single element
 * if running with SMP.
 *
 * @ingroup cheri_stack_tests
 */
ZTEST(cheri_stack, test_d_idle_csp_bounds_cheri)
{
idle_csp_bounds_cheri();
}

ZTEST_SUITE(cheri_stack, NULL, NULL,
		NULL, NULL, NULL);
