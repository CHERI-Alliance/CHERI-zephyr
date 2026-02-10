/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CHERI - memory block test inputs */

/*
 * The alignment of a block for mem_blocks is defined as being a
 * power of two.
 * It gets rounded by the macro up to a minimum size of a pointer.
 *
 * The size of a block for mem_blocks is defined as being a power of two.
 * It gets rounded by the macro up to a minimum size of a pointer.
 *
 * Because the block size is always a power of two it doesn't need
 * CHERI rounding. We just need to ensure CHERI alignment for the
 * whole memory to guarantee exact bounds and alignment for
 * the total blocks.
 * Therefore these inputs work for both these equations:
 * (because cheri_round(len) = len)
 *   x = cheri_round( cheri_round(len) * num_blks )
 *   y = cheri_round( len * num_blks )
 *  There are NO edge cases where x != y,
 */

/* Test 1 inputs */
/*set size to get bounded memory region for CHERI 256 bytes - 512 bytes*/
#define NUMBLOCKS1   5
#define CHERI_ALIGN1 4
#define MINALIGN1   CHERI_ALIGN1
#define BLOCKLEN1   64

/* Test 2 inputs */
/*set size to get bounded memory slab region for CHERI 512 bytes - 1 KiB*/
#define NUMBLOCKS2   4
#define CHERI_ALIGN2 4
#define MINALIGN2   CHERI_ALIGN2
#define BLOCKLEN2    128

/* Test 3 inputs */
/*set size to get bounded memory slab region for CHERI 1 KiB - 2 KiB*/
#define NUMBLOCKS3   4
#define CHERI_ALIGN3 4
#define MINALIGN3   CHERI_ALIGN3
#define BLOCKLEN3    256

/* Test 4 inputs */
/*set size to get bounded memory slab region for CHERI 2 KiB - 4 KiB*/
#define NUMBLOCKS4   4
#define CHERI_ALIGN4 4
#define MINALIGN4   CHERI_ALIGN4
#define BLOCKLEN4    512

/* Test 5 inputs */
/*set size to get bounded memory slab region for CHERI 4 KiB - 8 KiB*/
#define NUMBLOCKS5   4
#define CHERI_ALIGN5 8
#define MINALIGN5   CHERI_ALIGN5
#define BLOCKLEN5    1024

/* Test 6 inputs */
/*set size to get bounded memory slab region for CHERI 32 KiB - 64 KiB*/
#define NUMBLOCKS6   4
#define CHERI_ALIGN6 16
#define MINALIGN6   CHERI_ALIGN6
#define BLOCKLEN6    8192

/*Test 7 inputs */
/*set size to get bounded memory slab region for CHERI 128 KiB - 256 KiB*/
#define NUMBLOCKS7   4
#define CHERI_ALIGN7 16
#define MINALIGN7   CHERI_ALIGN7
#define BLOCKLEN7    32768

/*Test 8 inputs */
/*set size to get bounded memory slab region for CHERI 512 KiB - 1 MiB*/
#define NUMBLOCKS8   8
#define CHERI_ALIGN8 16
#define MINALIGN8   CHERI_ALIGN8
#define BLOCKLEN8    65536

/*Test 9 inputs */
/*set size to get bounded memory slab region for CHERI 8 MiB - 16 MiB*/
#define NUMBLOCKS9   8
#define CHERI_ALIGN9 16
#define MINALIGN9   CHERI_ALIGN9
#define BLOCKLEN9    1048576
