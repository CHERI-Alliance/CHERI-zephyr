/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FUNC_DEFS_H
#define FUNC_DEFS_H

/* test inputs */

#define NUM_CPUS 2
#define NUM_STACKS 3
/* Choose a size that is likely to get rounded by CHERI representable length */
#if CONFIG_64BIT
#define STACK1 16389-CONFIG_TEST_EXTRA_STACK_SIZE
#else
#define STACK1 1032-CONFIG_TEST_EXTRA_STACK_SIZE
#endif
#define STEST_STACKSIZE (STACK1 + CONFIG_TEST_EXTRA_STACK_SIZE)

/* helper_funcs.h */

void assert_single_stack(char *cap,
			size_t size_bytes);

void assert_array_stack(int index,
			char *cap,
			size_t size_bytes);

void assert_thread_stack(char *cap,
			size_t size_bytes);

void assert_array_stack_idle(int index,
			char *cap,
			size_t size_bytes);

/* test_stack_define.c */

void stack_bounds_cheri(void);
void stack_threads_bounds_cheri(void);

/* test_idle_stack.c */

void idle_stack_bounds_cheri(void);
void idle_csp_bounds_cheri(void);

#endif
