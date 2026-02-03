/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_inputs.h"
#include "inline_funcs.h"
#include "func_defs.h"

/*
 * These functions are for testing the memory slab APIs with CHERI
 * when memory slab is defined in the form:
 * K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align);
 * The CHERI version of the macro is defined in kernel.h
 * For CHERI the memory slabs need CHERI alignment.
 *  x = cheri_round(cheri_round(len) * num_blks)
 * Each memory block is CHERI aligned and the length is rounded.
 * The slab is then CHERI aligned to the total slab length.
 * Exact bounds can be achieved for both the slab and the blocks
 * and has been tested for the edge case.
 * Slab lengths up to 16 MiB have been accommodated.
 */

/* define macro-based slab backing buffers and slab structure */

/* boundary cases */
K_MEM_SLAB_DEFINE(macro_slab1, BLOCKLEN1, NUMBLOCKS1, SLABALIGN1);
K_MEM_SLAB_DEFINE(macro_slab2, BLOCKLEN2, NUMBLOCKS2, SLABALIGN2);
K_MEM_SLAB_DEFINE(macro_slab3, BLOCKLEN3, NUMBLOCKS3, SLABALIGN3);
K_MEM_SLAB_DEFINE(macro_slab4, BLOCKLEN4, NUMBLOCKS4, SLABALIGN4);
K_MEM_SLAB_DEFINE(macro_slab5, BLOCKLEN5, NUMBLOCKS5, SLABALIGN5);
K_MEM_SLAB_DEFINE(macro_slab6, BLOCKLEN6, NUMBLOCKS6, SLABALIGN6);
K_MEM_SLAB_DEFINE(macro_slab7, BLOCKLEN7, NUMBLOCKS7, SLABALIGN7);
K_MEM_SLAB_DEFINE(macro_slab8, BLOCKLEN8, NUMBLOCKS8, SLABALIGN8);
K_MEM_SLAB_DEFINE(macro_slab9, BLOCKLEN9, NUMBLOCKS9, SLABALIGN9);
/* edge cases */
K_MEM_SLAB_DEFINE(macro_slab_l1, BLOCKLEN_l1, NUMBLOCKS_l1, SLABALIGN_l1);

/* Helper functions for the tests */

/*
 * allocate all blocks from the slab
 * validate that num_blocks can be allocated with exact
 * CHERI bounds
 */
void test_slab_get_all_blocks_cheri(void **ptr, struct k_mem_slab *slab, size_t block_len,
				    size_t slab_align, int num_blocks)
{
	TC_PRINT("  Requested length of each block: = %zu bytes, after WB_UP: %zu bytes\n",
		 block_len, (size_t)WB_UP(block_len));
#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * The CHERI bounds of each block are applied during the init
	 * function k_mem_slab_init. Calculate here the expected
	 * bounds length using the CHERI macros.
	 */
	size_t expected_block_len = CHERI_ROUND_UP(
		WB_UP(block_len), CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(slab_align)));

	TC_PRINT(
		"  Expected length of each block after CHERI alignment and rounding: = %zu bytes\n",
		expected_block_len);
#endif /* __CHERI_PURE_CAPABILITY__ */

	void *errptr; /* Pointer to block */

	for (int i = 0; i < num_blocks; i++) {

		/* Verify number of used blocks */
		zassert_equal(k_mem_slab_num_used_get(slab), i, "Failed k_mem_slab_num_used_get");

		/* Allocate 1 memory block */
		zassert_equal(k_mem_slab_alloc(slab, &ptr[i], K_NO_WAIT), 0,
			      "Failed k_mem_slab_alloc");

#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  ", i, ptr[i]);
		/* Verify the block is aligned and the bounds are of the expected length */
		assert_block_macro_cheri("checking....", slab_align, block_len, ptr[i]);
#else
		TC_PRINT("  block ptr[%d] = %p\n", i, ptr[i]);
#endif /* __CHERI_PURE_CAPABILITY__ */
	} /* for */

	/* Verify all blocks are now allocated */
	zassert_equal(k_mem_slab_num_used_get(slab), num_blocks, "Failed k_mem_slab_num_used_get");

	/* Try to get one more block and it should fail */
	zassert_equal(k_mem_slab_alloc(slab, &errptr, K_NO_WAIT), -ENOMEM,
		      "Failed k_mem_slab_alloc");
} /* test_slab_get_all_blocks_cheri */

/* Free all blocks back to the slab - validate that all blocks are freed */

void test_slab_free_all_blocks_cheri(void **ptr, struct k_mem_slab *slab, int num_blocks)
{
	for (int i = 0; i < num_blocks; i++) {
		/* Verify number of used blocks in the map */
		zassert_equal(k_mem_slab_num_used_get(slab), num_blocks - i,
			      "Failed k_mem_slab_num_used_get");

		/* Free memory block */
		k_mem_slab_free(slab, ptr[i]);

#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  freed", i, ptr[i]);
		TC_PRINT("\n");
#endif
	} /* for */

	/* Verify number of used blocks is now 0 */

	zassert_equal(k_mem_slab_num_used_get(slab), 0, "Failed k_mem_slab_num_used_get");

} /* test_slab_free_all_blocks_cheri */

/* test functions called by ZTEST */

/*
 * Checks macro-based backing buffers are CHERI aligned and tightly bound
 * runs a test within 9 different alignment boundaries, plus an edge
 * case, prints the slab length and alignment details, and
 * asserts these details against what is expected.
 */
void cheri_macro_slab_bounds(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking compile-time backing buffers are 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking compile-time backing buffers are 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif
#else
	TC_PRINT("Non-CHERI mode, no slab bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__
	/* print slab size and bounds information */
	print_slab_macro_cheri("Slab macro_slab1 test 256 bytes - 512 bytes........", SLABALIGN1,
			       BLOCKLEN1, NUMBLOCKS1, macro_slab1.buffer,
			       macro_slab1.info.block_size);
	print_slab_macro_cheri("Slab macro_slab2 test 512 bytes - 1 KiB........", SLABALIGN2,
			       BLOCKLEN2, NUMBLOCKS2, macro_slab2.buffer,
			       macro_slab2.info.block_size);
	print_slab_macro_cheri("Slab macro_slab3 test 1 KiB - 2 KiB........", SLABALIGN3, BLOCKLEN3,
			       NUMBLOCKS3, macro_slab3.buffer, macro_slab3.info.block_size);
	print_slab_macro_cheri("Slab macro_slab4 test 2 KiB - 4 KiB........", SLABALIGN4, BLOCKLEN4,
			       NUMBLOCKS4, macro_slab4.buffer, macro_slab4.info.block_size);
	print_slab_macro_cheri("Slab macro_slab5 test 4 KiB - 8 KiB........", SLABALIGN5, BLOCKLEN5,
			       NUMBLOCKS5, macro_slab5.buffer, macro_slab5.info.block_size);
	print_slab_macro_cheri("Slab macro_slab6 test 32 KiB - 64 KiB........", SLABALIGN6,
			       BLOCKLEN6, NUMBLOCKS6, macro_slab6.buffer,
			       macro_slab6.info.block_size);
	print_slab_macro_cheri("Slab macro_slab7 test 128 KiB - 256 KiB........", SLABALIGN7,
			       BLOCKLEN7, NUMBLOCKS7, macro_slab7.buffer,
			       macro_slab7.info.block_size);
	print_slab_macro_cheri("Slab macro_slab8 test 512 KiB - 1 MiB........", SLABALIGN8,
			       BLOCKLEN8, NUMBLOCKS8, macro_slab8.buffer,
			       macro_slab8.info.block_size);
	print_slab_macro_cheri("Slab macro_slab9 test 8 MiB - 16 MiB........", SLABALIGN9,
			       BLOCKLEN9, NUMBLOCKS9, macro_slab9.buffer,
			       macro_slab9.info.block_size);
	print_slab_macro_cheri("Slab macro_slab_l1 test edge case........", SLABALIGN_l1,
			       BLOCKLEN_l1, NUMBLOCKS_l1, macro_slab_l1.buffer,
			       macro_slab_l1.info.block_size);
#else
	TC_PRINT("Slab macro_slab1 = %p\n", macro_slab1.buffer);
	TC_PRINT("Slab macro_slab2 = %p\n", macro_slab2.buffer);
	TC_PRINT("Slab macro_slab3 = %p\n", macro_slab3.buffer);
	TC_PRINT("Slab macro_slab4 = %p\n", macro_slab4.buffer);
	TC_PRINT("Slab macro_slab5 = %p\n", macro_slab5.buffer);
	TC_PRINT("Slab macro_slab6 = %p\n", macro_slab6.buffer);
	TC_PRINT("Slab macro_slab7 = %p\n", macro_slab7.buffer);
	TC_PRINT("Slab macro_slab8 = %p\n", macro_slab8.buffer);
	TC_PRINT("Slab macro_slab9 = %p\n", macro_slab9.buffer);
	TC_PRINT("Slab macro_slab_l1 = %p\n", macro_slab_l1.buffer);
#endif

#ifdef __CHERI_PURE_CAPABILITY__
	/* test assertions */
	assert_slab_macro_cheri("checking macro_slab1.buffer.....", SLABALIGN1, BLOCKLEN1,
				NUMBLOCKS1, macro_slab1.buffer);
	assert_slab_macro_cheri("checking macro_slab2.buffer.....", SLABALIGN2, BLOCKLEN2,
				NUMBLOCKS2, macro_slab2.buffer);
	assert_slab_macro_cheri("checking macro_slab3.buffer.....", SLABALIGN3, BLOCKLEN3,
				NUMBLOCKS3, macro_slab3.buffer);
	assert_slab_macro_cheri("checking macro_slab4.buffer.....", SLABALIGN4, BLOCKLEN4,
				NUMBLOCKS4, macro_slab4.buffer);
	assert_slab_macro_cheri("checking macro_slab5.buffer.....", SLABALIGN5, BLOCKLEN5,
				NUMBLOCKS5, macro_slab5.buffer);
	assert_slab_macro_cheri("checking macro_slab6.buffer.....", SLABALIGN6, BLOCKLEN6,
				NUMBLOCKS6, macro_slab6.buffer);
	assert_slab_macro_cheri("checking macro_slab7.buffer.....", SLABALIGN7, BLOCKLEN7,
				NUMBLOCKS7, macro_slab7.buffer);
	assert_slab_macro_cheri("checking macro_slab8.buffer.....", SLABALIGN8, BLOCKLEN8,
				NUMBLOCKS8, macro_slab8.buffer);
	assert_slab_macro_cheri("checking macro_slab9.buffer.....", SLABALIGN9, BLOCKLEN9,
				NUMBLOCKS9, macro_slab9.buffer);
	/* edge case */
	assert_slab_macro_cheri("checking macro_slab_l1.buffer.....", SLABALIGN_l1, BLOCKLEN_l1,
				NUMBLOCKS_l1, macro_slab_l1.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/*
 * Checks all blocks can be allocated from the slab and freed.
 * runs a test within 9 different alignment boundaries, plus an edge
 * case, prints the block bounds and alignment details, and
 * asserts these details against what is expected.
 */
void cheri_macro_block_bounds(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking block allocations for 64 bit CHERI are aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking block allocations for 32 bit CHERI are aligned and tightly "
		 "bound........\n");
#endif
#else
	TC_PRINT("Non-CHERI mode, no block bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Pointer to memory block */

	void *ptr1[NUMBLOCKS1];
	void *ptr2[NUMBLOCKS2];
	void *ptr3[NUMBLOCKS3];
	void *ptr4[NUMBLOCKS4];
	void *ptr5[NUMBLOCKS5];
	void *ptr6[NUMBLOCKS6];
	void *ptr7[NUMBLOCKS7];
	void *ptr8[NUMBLOCKS8];
	void *ptr9[NUMBLOCKS9];
	void *ptr_l1[NUMBLOCKS_l1];

	/* not strictly necessary, but keeps coverity checks happy */

	(void)memset(ptr1, 0, sizeof(ptr1));
	(void)memset(ptr2, 0, sizeof(ptr2));
	(void)memset(ptr3, 0, sizeof(ptr3));
	(void)memset(ptr4, 0, sizeof(ptr4));
	(void)memset(ptr5, 0, sizeof(ptr5));
	(void)memset(ptr6, 0, sizeof(ptr6));
	(void)memset(ptr7, 0, sizeof(ptr7));
	(void)memset(ptr8, 0, sizeof(ptr8));
	(void)memset(ptr9, 0, sizeof(ptr9));
	(void)memset(ptr_l1, 0, sizeof(ptr_l1));

	/* Test k_mem_slab_alloc, get all blocks */

	TC_PRINT("Block allocation from macro_slab1, test 256 - 512 bytes.....\n");
	test_slab_get_all_blocks_cheri(ptr1, &macro_slab1, (size_t)BLOCKLEN1, (size_t)SLABALIGN1,
				       (int)NUMBLOCKS1);

	TC_PRINT("Block allocation from macro_slab2, test 512 bytes - 1 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr2, &macro_slab2, (size_t)BLOCKLEN2, (size_t)SLABALIGN2,
				       (int)NUMBLOCKS2);

	TC_PRINT("Block allocation from macro_slab3, test 1 KiB - 2kiB.....\n");
	test_slab_get_all_blocks_cheri(ptr3, &macro_slab3, (size_t)BLOCKLEN3, (size_t)SLABALIGN3,
				       (int)NUMBLOCKS3);

	TC_PRINT("Block allocation from macro_slab4, test 2 KiB - 4 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr4, &macro_slab4, (size_t)BLOCKLEN4, (size_t)SLABALIGN4,
				       (int)NUMBLOCKS4);

	TC_PRINT("Block allocation from macro_slab5, test 4 KiB - 8 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr5, &macro_slab5, (size_t)BLOCKLEN5, (size_t)SLABALIGN5,
				       (int)NUMBLOCKS5);

	TC_PRINT("Block allocation from macro_slab6, test 32 KiB - 64 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr6, &macro_slab6, (size_t)BLOCKLEN6, (size_t)SLABALIGN6,
				       (int)NUMBLOCKS6);

	TC_PRINT("Block allocation from macro_slab7, test 128 KiB - 256 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr7, &macro_slab7, (size_t)BLOCKLEN7, (size_t)SLABALIGN7,
				       (int)NUMBLOCKS7);

	TC_PRINT("Block allocation from macro_slab8, test 512 KiB - 1 MiB.....\n");
	test_slab_get_all_blocks_cheri(ptr8, &macro_slab8, (size_t)BLOCKLEN8, (size_t)SLABALIGN8,
				       (int)NUMBLOCKS8);

	TC_PRINT("Block allocation from macro_slab9, test 8 MiB  - 16 MiB.....\n");
	test_slab_get_all_blocks_cheri(ptr9, &macro_slab9, (size_t)BLOCKLEN9, (size_t)SLABALIGN9,
				       (int)NUMBLOCKS9);

	TC_PRINT("Block allocation from macro_slab_l1, test edge case.....\n");
	test_slab_get_all_blocks_cheri(ptr_l1, &macro_slab_l1, (size_t)BLOCKLEN_l1,
				       (size_t)SLABALIGN_l1, (int)NUMBLOCKS_l1);

	/* Test k_mem_slab_free, free all blocks */

	TC_PRINT("Freeing blocks to macro_slab1, test 256 bytes - 512 bytes.....\n");
	test_slab_free_all_blocks_cheri(ptr1, &macro_slab1, (int)NUMBLOCKS1);

	TC_PRINT("Freeing blocks to macro_slab2, test 512 bytes - 1 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr2, &macro_slab2, (int)NUMBLOCKS2);

	TC_PRINT("Freeing blocks to macro_slab3, test 1 KiB - 2kiB.....\n");
	test_slab_free_all_blocks_cheri(ptr3, &macro_slab3, (int)NUMBLOCKS3);

	TC_PRINT("Freeing blocks to macro_slab4, test 2 KiB - 4 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr4, &macro_slab4, (int)NUMBLOCKS4);

	TC_PRINT("Freeing blocks to macro_slab5, test 4 KiB - 8 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr5, &macro_slab5, (int)NUMBLOCKS5);

	TC_PRINT("Freeing blocks to macro_slab6, test 32 KiB - 64 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr6, &macro_slab6, (int)NUMBLOCKS6);

	TC_PRINT("Freeing blocks to macro_slab7, test 128 KiB - 256 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr7, &macro_slab7, (int)NUMBLOCKS7);

	TC_PRINT("Freeing blocks to macro_slab8, test 512 KiB - 1 MiB.....\n");
	test_slab_free_all_blocks_cheri(ptr8, &macro_slab8, (int)NUMBLOCKS8);

	TC_PRINT("Freeing blocks to macro_slab9, test 8 MiB - 16 MiB.....\n");
	test_slab_free_all_blocks_cheri(ptr9, &macro_slab9, (int)NUMBLOCKS9);

	TC_PRINT("Freeing blocks to macro_slab_l1, test edge case.....\n");
	test_slab_free_all_blocks_cheri(ptr_l1, &macro_slab_l1, (int)NUMBLOCKS_l1);

	/* Test re-allocation after free doesn't break capability or bounds */

	TC_PRINT("Repeat Block allocation from macro_slab5, test 4 KiB - 8 KiB.....\n");
	test_slab_get_all_blocks_cheri(ptr5, &macro_slab5, (size_t)BLOCKLEN5, (size_t)SLABALIGN5,
				       (int)NUMBLOCKS5);

	TC_PRINT("Repeat Freeing blocks to macro_slab5, test 4 KiB - 8 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr5, &macro_slab5, (int)NUMBLOCKS5);
}
