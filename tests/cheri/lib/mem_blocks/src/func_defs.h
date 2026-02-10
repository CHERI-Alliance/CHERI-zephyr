/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FUNC_DEFS_H
#define FUNC_DEFS_H

/* mem_block_macro.c */
void cheri_mem_block_bounds(void);
void cheri_mem_block_block_bounds(void);
void cheri_mem_block_ext_bounds(void);
void cheri_mem_block_ext_block_bounds(void);

void cheri_mem_block_bounds_orig_test(void);

#endif
