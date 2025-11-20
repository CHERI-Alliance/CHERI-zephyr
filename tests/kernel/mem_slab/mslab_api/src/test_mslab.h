/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MSLAB_H__
#define __TEST_MSLAB_H__

#ifdef __CHERI_PURE_CAPABILITY__
/* for CHERI purecap, memory slabs need CHERI alignment.
 * To ensure that each memory block is similarly aligned
 * to this boundary, slab_block_size must also be a multiple
 * of slab_align.
 */
#define CHERI_ALIGN 16 /* work for 64/32 bit */

#define TIMEOUT 2000
#define BLK_NUM 3
#define BLK_ALIGN CHERI_ALIGN
#define BLK_SIZE CHERI_ALIGN*2
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#else

#define TIMEOUT 2000
#define BLK_NUM 3
#define BLK_ALIGN 8
#define BLK_SIZE 16
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#endif /*__CHERI_PURE_CAPABILITY__*/

extern void tmslab_alloc_free(void *data);
extern void *mslab_setup(void);

#endif /*__TEST_MSLAB_H__*/
