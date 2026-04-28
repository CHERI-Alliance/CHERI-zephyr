/*
 * Copyright (c) 2026 University of Birmingham, added to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* set sizes to ensure cheri rounding and
 * alignment calculations are exercised
 */
#define BLK_SIZE_1 7
#define BLK_SIZE_2 68
#define BLK_SIZE_3 133
#define BLK_SIZE_4 260
#define ALIGN_1 1
#define ALIGN_2 8
#define ALIGN_3 16
#define ALIGN_4 32

#define BLK_SIZE_MIN 64
#define BLK_NUM_MAX (K_HEAP_MEM_POOL_SIZE / BLK_SIZE_MIN)
#define BLK_NUM_MIN (K_HEAP_MEM_POOL_SIZE / (BLK_SIZE_MIN << 2))
