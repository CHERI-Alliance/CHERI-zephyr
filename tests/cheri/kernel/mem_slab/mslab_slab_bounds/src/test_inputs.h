/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CHERI - memory slab test inputs */

/*
 * These test inputs happen to work for both these equations:
 *   x = cheri_round( cheri_round(len) * num_blks )
 *   y = cheri_round( len * num_blks )
 *  For edge cases where x != y, see edge case inputs below
 */

/* Test 1 inputs */
/*set size to get bounded memory slab region for CHERI 256 bytes - 512 bytes*/
#define NUMBLOCKS1   5
#define CHERI_ALIGN1 4
#define SLABALIGN1   CHERI_ALIGN1
#define BLOCKLEN1    CHERI_ALIGN1 * 17

/* Test 2 inputs */
/*set size to get bounded memory slab region for CHERI 512 bytes - 1 KiB*/
#define NUMBLOCKS2   4
#define CHERI_ALIGN2 4
#define SLABALIGN2   CHERI_ALIGN2
#define BLOCKLEN2    CHERI_ALIGN2 * 57

/* Test 3 inputs */
/*set size to get bounded memory slab region for CHERI 1 KiB - 2 KiB*/
#define NUMBLOCKS3   4
#define CHERI_ALIGN3 4
#define SLABALIGN3   CHERI_ALIGN3
#define BLOCKLEN3    CHERI_ALIGN3 * 117

/* Test 4 inputs */
/*set size to get bounded memory slab region for CHERI 2 KiB - 4 KiB*/
#define NUMBLOCKS4   4
#define CHERI_ALIGN4 4
#define SLABALIGN4   CHERI_ALIGN4
#define BLOCKLEN4    CHERI_ALIGN4 * 227

/* Test 5 inputs */
/*set size to get bounded memory slab region for CHERI 4 KiB - 8 KiB*/
#define NUMBLOCKS5   4
#define CHERI_ALIGN5 8
#define SLABALIGN5   CHERI_ALIGN5
#define BLOCKLEN5    1068

/* Test 6 inputs */
/*set size to get bounded memory slab region for CHERI 32 KiB - 64 KiB*/
#define NUMBLOCKS6   4
#define CHERI_ALIGN6 16
#define SLABALIGN6   CHERI_ALIGN6
#define BLOCKLEN6    CHERI_ALIGN6 * 573

/*Test 7 inputs */
/*set size to get bounded memory slab region for CHERI 128 KiB - 256 KiB*/
#define NUMBLOCKS7   4
#define CHERI_ALIGN7 16
#define SLABALIGN7   CHERI_ALIGN7
#define BLOCKLEN7    CHERI_ALIGN7 * 2390

/*Test 8 inputs */
/*set size to get bounded memory slab region for CHERI 512 KiB - 1 MiB*/
#define NUMBLOCKS8   4
#define CHERI_ALIGN8 16
#define SLABALIGN8   CHERI_ALIGN8
#define BLOCKLEN8    CHERI_ALIGN8 * 8192

/*Test 9 inputs */
/*set size to get bounded memory slab region for CHERI 8 MiB - 16 MiB*/
#define NUMBLOCKS9   4
#define CHERI_ALIGN9 16
#define SLABALIGN9   CHERI_ALIGN9
#define BLOCKLEN9    CHERI_ALIGN9 * 176890

/*
 * These test inputs test edge cases where x != y:
 *   x = cheri_round( cheri_round(len) * num_blks )
 *   y = cheri_round( len * num_blks )
 * they are used for testing the following options
 * When memory slab is defined in the form:
 *  static char __aligned(align) init_slab_buffer[length * num_blocks];
 *  static struct k_mem_slab init_slab;
 * Options:
 *   CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
 *	- relax the bounds of a block to maintain the requested size
 *	- whilst maintaining tight bounds of the slab only
 *	- not recommened  - gives overlapping bounds between blocks
 *   CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE
 *	- reduce the number of available blocks
 *	- whilst maintaining strict exact bounds of a block
 *
 * When memory slab is defined in the form:
 * K_MEM_SLAB_DEFINE, the cheri macros automatically round
 * up to CHERI length / alignment requirements based on eq. x
 *
 * These test input numbers are obtained from
 * running cheri_get_slab_length_edge_cases() in helper_funcs.c
 *
 * used in tests:
 *   cheri_init_slab_bounds_exact_fail() - shows the allocation fails
 *   cheri_init_slab_bounds_relaxed() - shows the allocation passes with relaxed bounds
 *   cheri_init_slab_num_blks_reduced() - shows the allocation passes with a reduced number of
 * blocks cheri_init_slab_bounds_exact_macro_pass() - shows the allocation passes when a cheri macro
 * is used to round the block length cheri_macro_slab_bounds() - shows the allocation passes for
 * K_MEM_SLAB_DEFINE
 */
#if CONFIG_64BIT
#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
/* 64bit codasp, len 16389 blks 3, 49280 > 49216 */
#define NUMBLOCKS_l1 3
#define SLABALIGN_l1 16
#define BLOCKLEN_l1  16389
#else
/* 64bit cambs - cheribuild, len 16389 blks 3, 49280 > 49216 */
#define NUMBLOCKS_l1 3
#define SLABALIGN_l1 16
#define BLOCKLEN_l1  16389
#endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#else
#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
/* 32bit codasp, len 1032 blks 3, 3136 > 3104 */
#define NUMBLOCKS_l1 3
#define SLABALIGN_l1 8
#define BLOCKLEN_l1  1032
#else
/* 32bit cambs - cheribuild, len 136 blks 3, 448 > 416 */
#define NUMBLOCKS_l1 3
#define SLABALIGN_l1 8
#define BLOCKLEN_l1  136
#endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#endif /* CONFIG_64BIT */
