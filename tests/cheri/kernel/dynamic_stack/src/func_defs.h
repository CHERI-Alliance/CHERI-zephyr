/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FUNC_DEFS_H
#define FUNC_DEFS_H

/* helper_funcs.h */

void assert_single_stack(char *cap,
			size_t size_bytes);

void assert_single_stack_sysheap(char *cap,
			size_t size_bytes);

void assert_thread_stack(char *cap,
			size_t size_bytes);

/* test_dynamic_stack.c */

void dynamic_stack_bounds_cheri_pool(void);
void dynamic_stack_bounds_cheri_sysheap(void);

#endif
