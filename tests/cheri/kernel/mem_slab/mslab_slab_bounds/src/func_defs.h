/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FUNC_DEFS_H
#define FUNC_DEFS_H

/* helper_funcs.c */
void cheri_scan_cheri_alignment_thresholds(void);
void cheri_test_cheri_macros(void);
void cheri_get_slab_length_edge_cases(void);

/* slab_macro.c */
void test_slab_free_all_blocks_cheri(void **ptr, struct k_mem_slab *slab, int num_blocks);
void cheri_macro_slab_bounds(void);
void cheri_macro_block_bounds(void);

/* slab_init.c */
void cheri_init_slab_bounds(void);
void cheri_init_block_bounds(void);
void cheri_init_slab_bounds_exact_fail(void);
void cheri_init_slab_bounds_exact_macro_pass(void);
void cheri_init_block_bounds_exact_macro_pass(void);
void cheri_init_slab_bounds_relaxed(void);
void cheri_init_block_bounds_relaxed(void);
void cheri_init_slab_num_blks_reduced(void);
void cheri_init_block_num_blks_reduced(void);

#endif
