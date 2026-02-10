/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

#include "test_inputs.h"
#include "inline_funcs.h"
#include "func_defs.h"

#define PRINT_EXT 0 /* print extra information */

/*
 * These functions are for testing the memory block APIs with CHERI
 * when memory is defined in the form:

 * SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, buf_align);
 * and
 * static uint8_t __aligned(WB_UP(balign)) buf[blk_sz * num_blks];
 * SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf);
 * The CHERI version of the macros are defined in mem_blocks.h
 *
 * For CHERI the memory buffers need CHERI alignment and rounding.
 *  x = cheri_round(len * num_blks)
 * Because the block size is always a power of two the individual
 * blocks do not need CHERI rounding.
 * We just need to ensure CHERI alignment for the
 * whole memory to guarantee exact bounds and alignment for
 * the total blocks.
 *
 * lengths up to 16 MiB have been accommodated.
 */

/* define macro-based mem_blocks */

SYS_MEM_BLOCKS_DEFINE(macro_mem_block1, BLOCKLEN1, NUMBLOCKS1, MINALIGN1);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block2, BLOCKLEN2, NUMBLOCKS2, MINALIGN2);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block3, BLOCKLEN3, NUMBLOCKS3, MINALIGN3);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block4, BLOCKLEN4, NUMBLOCKS4, MINALIGN4);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block5, BLOCKLEN5, NUMBLOCKS5, MINALIGN5);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block6, BLOCKLEN6, NUMBLOCKS6, MINALIGN6);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block7, BLOCKLEN7, NUMBLOCKS7, MINALIGN7);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block8, BLOCKLEN8, NUMBLOCKS8, MINALIGN8);
SYS_MEM_BLOCKS_DEFINE(macro_mem_block9, BLOCKLEN9, NUMBLOCKS9, MINALIGN9);

/* define static backing buffers and ext macro for mem_blocks */

static uint8_t __aligned(WB_UP(MINALIGN1)) mem_block_ext1_buf[BLOCKLEN1 * NUMBLOCKS1];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext1, BLOCKLEN1, NUMBLOCKS1,
					  mem_block_ext1_buf);
static uint8_t __aligned(WB_UP(MINALIGN2)) mem_block_ext2_buf[BLOCKLEN2 * NUMBLOCKS2];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext2, BLOCKLEN2, NUMBLOCKS2,
					  mem_block_ext2_buf);
static uint8_t __aligned(WB_UP(MINALIGN3)) mem_block_ext3_buf[BLOCKLEN3 * NUMBLOCKS3];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext3, BLOCKLEN3, NUMBLOCKS3,
					  mem_block_ext3_buf);
static uint8_t __aligned(WB_UP(MINALIGN4)) mem_block_ext4_buf[BLOCKLEN4 * NUMBLOCKS4];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext4, BLOCKLEN4, NUMBLOCKS4,
					  mem_block_ext4_buf);
static uint8_t __aligned(WB_UP(MINALIGN5)) mem_block_ext5_buf[BLOCKLEN5 * NUMBLOCKS5];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext5, BLOCKLEN5, NUMBLOCKS5,
					  mem_block_ext5_buf);
static uint8_t __aligned(WB_UP(MINALIGN6)) mem_block_ext6_buf[BLOCKLEN6 * NUMBLOCKS6];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext6, BLOCKLEN6, NUMBLOCKS6,
					  mem_block_ext6_buf);
static uint8_t __aligned(WB_UP(MINALIGN7)) mem_block_ext7_buf[BLOCKLEN7 * NUMBLOCKS7];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext7, BLOCKLEN7, NUMBLOCKS7,
					  mem_block_ext7_buf);
static uint8_t __aligned(WB_UP(MINALIGN8)) mem_block_ext8_buf[BLOCKLEN8 * NUMBLOCKS8];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext8, BLOCKLEN8, NUMBLOCKS8,
					  mem_block_ext8_buf);
static uint8_t __aligned(WB_UP(MINALIGN9)) mem_block_ext9_buf[BLOCKLEN9 * NUMBLOCKS9];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_ext9, BLOCKLEN9, NUMBLOCKS9,
					  mem_block_ext9_buf);

/* define original test set up */

#define BLK_SZ     64
#define NUM_BLOCKS 8

SYS_MEM_BLOCKS_DEFINE(mem_block_01, BLK_SZ, NUM_BLOCKS, 4);

static uint8_t mem_block_02_buf[BLK_SZ * NUM_BLOCKS];
SYS_MEM_BLOCKS_DEFINE_STATIC_WITH_EXT_BUF(mem_block_02,
					  BLK_SZ, NUM_BLOCKS,
					  mem_block_02_buf);


/* standard bound check from mem_block tests */
static bool check_buffer_bound(sys_mem_blocks_t *mem_block, void *ptr)
{
	uint8_t *start, *end, *ptr_u8;

	start = mem_block->buffer;
	end = start + (BIT(mem_block->info.blk_sz_shift) *
		       mem_block->info.num_blocks);

	ptr_u8 = (uint8_t *)ptr;

	if ((ptr_u8 >= start) && (ptr_u8 < end)) {
		return true;
	} else {
		return false;
	}
}

/* get all blocks and check CHERI bounds */
static void test_mem_block_get_all_blocks_cheri(void **blocks, sys_mem_blocks_t *mem_block,
					size_t block_len, size_t min_align, int num_blocks)
{
	int i, ret;
	int val;

	TC_PRINT("  Requested length of each block: = %zu bytes, after WB_UP: %zu bytes\n",
		 block_len, (size_t)WB_UP(block_len));
#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * For block sizes specified in powers of two, the CHERI representable
	 * length should be the same as the WB_UP(block_len) .
	 * print it here using the CHERI macros.
	 */
	size_t expected_block_len = CHERI_ROUND_UP_TO_REP_LEN(WB_UP(block_len), WB_UP(min_align));

	TC_PRINT(
		"  Expected length of each block after CHERI alignment and rounding: = %zu bytes\n",
		expected_block_len);
#endif /* __CHERI_PURE_CAPABILITY__ */

		for (i = 0; i < num_blocks; i++) {
			ret = sys_mem_blocks_alloc(mem_block, 1, &blocks[i]);

			zassert_equal(ret, 0,
				      "sys_mem_blocks_alloc failed (%d)", ret);

			zassert_true(check_buffer_bound(mem_block, &blocks[i][0]),
				     "allocated memory is out of bound");

			ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
			zassert_equal(ret, 0, "API failure");
			zassert_equal(val, 1,
				      "sys_mem_blockss_alloc bitmap failed");

#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  ", i, blocks[i]);
		/* Verify the block is aligned and the bounds are of the expected length */
		assert_block_macro_cheri("checking....", min_align, block_len, blocks[i]);
#else
		TC_PRINT("  block blocks[%d] = %p\n", i, blocks[i]);
#endif /* __CHERI_PURE_CAPABILITY__ */

		}

		if (num_blocks >= num_blocks) {
			ret = sys_mem_blocks_alloc(mem_block, 1, &blocks[i]);
			zassert_equal(ret, -ENOMEM,
				"sys_mem_blocks_alloc should fail with -ENOMEM but not");
		}
} /* test_mem_block_get_all_blocks_cheri */

/* Free all blocks */

void test_mem_block_free_all_blocks_cheri(void **blocks, sys_mem_blocks_t *mem_block,
					int num_blocks)
{
	int ret, val;

	for (int i = 0; i < num_blocks; i++) {
		ret = sys_mem_blocks_free(mem_block, 1, &blocks[i]);
		zassert_equal(ret, 0,
			      "sys_mem_blocks_free failed (%d)", ret);

		ret = sys_bitarray_test_bit(mem_block->bitmap,
						    i, &val);
		zassert_equal(ret, 0, "API failure");
		zassert_equal(val, 0,
				      "sys_mem_blocks_free bitmap failed");

#ifdef __CHERI_PURE_CAPABILITY__
		if (PRINT_EXT > 0) {
			print_block_cheri("  freed", i, blocks[i]);
			TC_PRINT("\n");
		}
#endif
	} /* for */

} /* test_mem_block_free_all_blocks_cheri */


/* test functions called by ZTEST */

void cheri_mem_block_bounds(void)
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
	TC_PRINT("Non-CHERI mode, no bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__
	/* print buffer size and bounds information */
	print_mem_block_macro_cheri("buffer test 256 bytes - 512 bytes........", MINALIGN1,
			BLOCKLEN1, NUMBLOCKS1, &macro_mem_block1);
	print_mem_block_macro_cheri("buffer test 512 bytes - 1 KiB........", MINALIGN2,
			BLOCKLEN2, NUMBLOCKS2, &macro_mem_block2);
	print_mem_block_macro_cheri("buffer test 1 KiB - 2 KiB........", MINALIGN3,
			BLOCKLEN3, NUMBLOCKS3, &macro_mem_block3);
	print_mem_block_macro_cheri("buffer test 2 KiB - 4 KiB........", MINALIGN4,
			BLOCKLEN4, NUMBLOCKS4, &macro_mem_block4);
	print_mem_block_macro_cheri("buffer test 4 KiB - 8 KiB........", MINALIGN5,
			BLOCKLEN5, NUMBLOCKS5, &macro_mem_block5);
	print_mem_block_macro_cheri("buffer test 32 KiB - 64 KiB........", MINALIGN6,
			BLOCKLEN6, NUMBLOCKS6, &macro_mem_block6);
	print_mem_block_macro_cheri("buffer test 128 KiB - 256 KiB........", MINALIGN7,
			BLOCKLEN7, NUMBLOCKS7, &macro_mem_block7);
	print_mem_block_macro_cheri("buffer test 512 KiB - 1 MiB........", MINALIGN8,
			BLOCKLEN8, NUMBLOCKS8, &macro_mem_block8);
	print_mem_block_macro_cheri("buffer test 8 MiB - 16 MiB........", MINALIGN9,
			BLOCKLEN9, NUMBLOCKS9, &macro_mem_block9);
#else
	TC_PRINT("mem_block  = %p\n", macro_mem_block1.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block2.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block3.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block4.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block5.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block6.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block7.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block8.buffer);
	TC_PRINT("mem_block  = %p\n", macro_mem_block9.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__
	/* test assertions */

	assert_mem_block_macro_cheri("checking macro_mem_block1.buffer.....", MINALIGN1, BLOCKLEN1,
				NUMBLOCKS1, macro_mem_block1.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block2.buffer.....", MINALIGN2, BLOCKLEN2,
				NUMBLOCKS2, macro_mem_block2.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block3.buffer.....", MINALIGN3, BLOCKLEN3,
				NUMBLOCKS3, macro_mem_block3.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block4.buffer.....", MINALIGN4, BLOCKLEN4,
				NUMBLOCKS4, macro_mem_block4.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block5.buffer.....", MINALIGN5, BLOCKLEN5,
				NUMBLOCKS5, macro_mem_block5.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block6.buffer.....", MINALIGN6, BLOCKLEN6,
				NUMBLOCKS6, macro_mem_block6.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block7.buffer.....", MINALIGN7, BLOCKLEN7,
				NUMBLOCKS7, macro_mem_block7.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block8.buffer.....", MINALIGN8, BLOCKLEN8,
				NUMBLOCKS8, macro_mem_block8.buffer);
	assert_mem_block_macro_cheri("checking macro_mem_block9.buffer.....", MINALIGN9, BLOCKLEN9,
				NUMBLOCKS9, macro_mem_block9.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void cheri_mem_block_block_bounds(void)
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

	/* get all blocks */

	TC_PRINT("Block allocation from macro_mem_block1, test 256 - 512 bytes.....\n");
	test_mem_block_get_all_blocks_cheri(ptr1, &macro_mem_block1, (size_t)BLOCKLEN1,
		(size_t)MINALIGN1, (int)NUMBLOCKS1);
	TC_PRINT("Block allocation from macro_mem_block2, test 512 bytes - 1 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr2, &macro_mem_block2, (size_t)BLOCKLEN2,
		(size_t)MINALIGN2, (int)NUMBLOCKS2);
	TC_PRINT("Block allocation from macro_mem_block3, test 1 KiB - 2 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr3, &macro_mem_block3, (size_t)BLOCKLEN3,
		(size_t)MINALIGN3, (int)NUMBLOCKS3);
	TC_PRINT("Block allocation from macro_mem_block4, test 2 KiB - 4 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr4, &macro_mem_block4, (size_t)BLOCKLEN4,
		(size_t)MINALIGN4, (int)NUMBLOCKS4);
	TC_PRINT("Block allocation from macro_mem_block5, test 4 KiB - 8 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr5, &macro_mem_block5, (size_t)BLOCKLEN5,
		(size_t)MINALIGN5, (int)NUMBLOCKS5);
	TC_PRINT("Block allocation from macro_mem_block6, test 32 KiB - 64 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr6, &macro_mem_block6, (size_t)BLOCKLEN6,
		(size_t)MINALIGN6, (int)NUMBLOCKS6);
	TC_PRINT("Block allocation from macro_mem_block7, test 128 KiB - 256 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr7, &macro_mem_block7, (size_t)BLOCKLEN7,
		(size_t)MINALIGN7, (int)NUMBLOCKS7);
	TC_PRINT("Block allocation from macro_mem_block8, test 512 KiB - 1 MiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr8, &macro_mem_block8, (size_t)BLOCKLEN8,
		(size_t)MINALIGN8, (int)NUMBLOCKS8);
	TC_PRINT("Block allocation from macro_mem_block9, test 8 MiB - 16 MiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr9, &macro_mem_block9, (size_t)BLOCKLEN9,
		(size_t)MINALIGN9, (int)NUMBLOCKS9);

	/* free all blocks */

	TC_PRINT("Freeing blocks to macro_mem_block1, test 256 bytes - 512 bytes.....\n");
	test_mem_block_free_all_blocks_cheri(ptr1, &macro_mem_block1, (int)NUMBLOCKS1);
	TC_PRINT("Freeing blocks to macro_mem_block2, test 512 bytes - 1 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr2, &macro_mem_block2, (int)NUMBLOCKS2);
	TC_PRINT("Freeing blocks to macro_mem_block3, test 1 KiB - 2 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr3, &macro_mem_block3, (int)NUMBLOCKS3);
	TC_PRINT("Freeing blocks to macro_mem_block4, test 2 KiB - 4 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr4, &macro_mem_block4, (int)NUMBLOCKS4);
	TC_PRINT("Freeing blocks to macro_mem_block5, test 4 KiB - 8 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr5, &macro_mem_block5, (int)NUMBLOCKS5);
	TC_PRINT("Freeing blocks to macro_mem_block6, test 32 KiB - 64 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr6, &macro_mem_block6, (int)NUMBLOCKS6);
	TC_PRINT("Freeing blocks to macro_mem_block7, test 128 KiB - 256 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr7, &macro_mem_block7, (int)NUMBLOCKS7);
	TC_PRINT("Freeing blocks to macro_mem_block8, test 512 KiB - 1 MiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr8, &macro_mem_block8, (int)NUMBLOCKS8);
	TC_PRINT("Freeing blocks to macro_mem_block9, test 8 MiB - 16 MiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr9, &macro_mem_block9, (int)NUMBLOCKS9);
}

void cheri_mem_block_ext_bounds(void)
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
	TC_PRINT("Non-CHERI mode, no bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__
	/* print buffer size and bounds information */
	print_mem_block_macro_cheri("buffer test 256 bytes - 512 bytes........", MINALIGN1,
		BLOCKLEN1, NUMBLOCKS1, &mem_block_ext1);
	print_mem_block_macro_cheri("buffer test 512 bytes - 1 KiB........", MINALIGN2,
		BLOCKLEN2, NUMBLOCKS2, &mem_block_ext2);
	print_mem_block_macro_cheri("buffer test 1 KiB - 2 KiB........", MINALIGN3,
		BLOCKLEN3, NUMBLOCKS3, &mem_block_ext3);
	print_mem_block_macro_cheri("buffer test 2 KiB - 4 KiB........", MINALIGN4,
		BLOCKLEN4, NUMBLOCKS4, &mem_block_ext4);
	print_mem_block_macro_cheri("buffer test 4 KiB - 8 KiB........", MINALIGN5,
		BLOCKLEN5, NUMBLOCKS5, &mem_block_ext5);
	print_mem_block_macro_cheri("buffer test 32 KiB - 64 KiB........", MINALIGN6,
		BLOCKLEN6, NUMBLOCKS6, &mem_block_ext6);
	print_mem_block_macro_cheri("buffer test 128 KiB - 256 KiB........", MINALIGN7,
		BLOCKLEN7, NUMBLOCKS7, &mem_block_ext7);
	print_mem_block_macro_cheri("buffer test 512 KiB - 1 MiB........", MINALIGN8,
		BLOCKLEN8, NUMBLOCKS8, &mem_block_ext8);
	print_mem_block_macro_cheri("buffer test 8 MiB - 16 MiB........", MINALIGN9,
		BLOCKLEN9, NUMBLOCKS9, &mem_block_ext9);
#else
	TC_PRINT("mem_block  = %p\n", mem_block_ext1.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext2.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext3.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext4.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext5.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext6.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext7.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext8.buffer);
	TC_PRINT("mem_block  = %p\n", mem_block_ext9.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__
	/* test assertions */

	assert_mem_block_macro_cheri("checking mem_block_ext1.buffer.....", MINALIGN1, BLOCKLEN1,
				NUMBLOCKS1, mem_block_ext1.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext2.buffer.....", MINALIGN2, BLOCKLEN2,
				NUMBLOCKS2, mem_block_ext2.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext3.buffer.....", MINALIGN3, BLOCKLEN3,
				NUMBLOCKS3, mem_block_ext3.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext4.buffer.....", MINALIGN4, BLOCKLEN4,
				NUMBLOCKS4, mem_block_ext4.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext5.buffer.....", MINALIGN5, BLOCKLEN5,
				NUMBLOCKS5, mem_block_ext5.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext6.buffer.....", MINALIGN6, BLOCKLEN6,
				NUMBLOCKS6, mem_block_ext6.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext7.buffer.....", MINALIGN7, BLOCKLEN7,
				NUMBLOCKS7, mem_block_ext7.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext8.buffer.....", MINALIGN8, BLOCKLEN8,
				NUMBLOCKS8, mem_block_ext8.buffer);
	assert_mem_block_macro_cheri("checking mem_block_ext9.buffer.....", MINALIGN9, BLOCKLEN9,
				NUMBLOCKS9, mem_block_ext9.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void cheri_mem_block_ext_block_bounds(void)
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

	/* get all blocks */

	TC_PRINT("Block allocation from mem_block_ext1, test 256 - 512 bytes.....\n");
	test_mem_block_get_all_blocks_cheri(ptr1, &mem_block_ext1, (size_t)BLOCKLEN1,
		(size_t)MINALIGN1, (int)NUMBLOCKS1);
	TC_PRINT("Block allocation from mem_block_ext2, test 512 bytes - 1 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr2, &mem_block_ext2, (size_t)BLOCKLEN2,
		(size_t)MINALIGN2, (int)NUMBLOCKS2);
	TC_PRINT("Block allocation from mem_block_ext3, test 1 KiB - 2 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr3, &mem_block_ext3, (size_t)BLOCKLEN3,
		(size_t)MINALIGN3, (int)NUMBLOCKS3);
	TC_PRINT("Block allocation from mem_block_ext4, test 2 KiB - 4 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr4, &mem_block_ext4, (size_t)BLOCKLEN4,
		(size_t)MINALIGN4, (int)NUMBLOCKS4);
	TC_PRINT("Block allocation from mem_block_ext5, test 4 KiB - 8 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr5, &mem_block_ext5, (size_t)BLOCKLEN5,
		(size_t)MINALIGN5, (int)NUMBLOCKS5);
	TC_PRINT("Block allocation from mem_block_ext6, test 32 KiB - 64 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr6, &mem_block_ext6, (size_t)BLOCKLEN6,
		(size_t)MINALIGN6, (int)NUMBLOCKS6);
	TC_PRINT("Block allocation from mem_block_ext7, test 128 KiB - 256 KiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr7, &mem_block_ext7, (size_t)BLOCKLEN7,
		(size_t)MINALIGN7, (int)NUMBLOCKS7);
	TC_PRINT("Block allocation from mem_block_ext8, test 512 KiB - 1 MiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr8, &mem_block_ext8, (size_t)BLOCKLEN8,
		(size_t)MINALIGN8, (int)NUMBLOCKS8);
	TC_PRINT("Block allocation from mem_block_ext9, test 8 MiB - 16 MiB.....\n");
	test_mem_block_get_all_blocks_cheri(ptr9, &mem_block_ext9, (size_t)BLOCKLEN9,
		(size_t)MINALIGN9, (int)NUMBLOCKS9);

	/* free all blocks */

	TC_PRINT("Freeing blocks to mem_block_ext1, test 256 bytes - 512 bytes.....\n");
	test_mem_block_free_all_blocks_cheri(ptr1, &mem_block_ext1, (int)NUMBLOCKS1);
	TC_PRINT("Freeing blocks to mem_block_ext2, test 512 bytes - 1 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr2, &mem_block_ext2, (int)NUMBLOCKS2);
	TC_PRINT("Freeing blocks to mem_block_ext3, test 1 KiB - 2 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr3, &mem_block_ext3, (int)NUMBLOCKS3);
	TC_PRINT("Freeing blocks to mem_block_ext4, test 2 KiB - 4 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr4, &mem_block_ext4, (int)NUMBLOCKS4);
	TC_PRINT("Freeing blocks to mem_block_ext5, test 4 KiB - 8 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr5, &mem_block_ext5, (int)NUMBLOCKS5);
	TC_PRINT("Freeing blocks to mem_block_ext6, test 32 KiB - 64 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr6, &mem_block_ext6, (int)NUMBLOCKS6);
	TC_PRINT("Freeing blocks to mem_block_ext7, test 128 KiB - 256 KiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr7, &mem_block_ext7, (int)NUMBLOCKS7);
	TC_PRINT("Freeing blocks to mem_block_ext8, test 512 KiB - 1 MiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr8, &mem_block_ext8, (int)NUMBLOCKS8);
	TC_PRINT("Freeing blocks to mem_block_ext9, test 8 MiB - 16 MiB.....\n");
	test_mem_block_free_all_blocks_cheri(ptr9, &mem_block_ext9, (int)NUMBLOCKS9);
}

void cheri_mem_block_bounds_orig_test(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking buffers from original mem_block test are 64 bit "
		"CHERI aligned and tightly bound........\n");
#else
	TC_PRINT("Checking buffers from original mem_block test are 32 bit "
		"CHERI aligned and tightly bound........\n");
#endif
#else
	TC_PRINT("Non-CHERI mode, no bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

#ifdef __CHERI_PURE_CAPABILITY__

	void *ptr1[NUM_BLOCKS];
	void *ptr2[NUM_BLOCKS];

	/* print and verify buffer size and bounds information */
	print_mem_block_macro_cheri("buffer mem_block_01........", 4,
			BLK_SZ, NUM_BLOCKS, &mem_block_01);
	assert_mem_block_macro_cheri("checking mem_block_01.buffer.....", 4, BLK_SZ,
				NUM_BLOCKS, mem_block_01.buffer);

	print_mem_block_macro_cheri("buffer mem_block_02........", 1,
			BLK_SZ, NUM_BLOCKS, &mem_block_02);
	assert_mem_block_macro_cheri("checking mem_block_01.buffer.....", 1, BLK_SZ,
				NUM_BLOCKS, mem_block_02.buffer);

	/* get all blocks */

	TC_PRINT("Block allocation from mem_block_01.....\n");
	test_mem_block_get_all_blocks_cheri(ptr1, &mem_block_01, (size_t)BLK_SZ,
		(size_t)4, (int)NUM_BLOCKS);
	TC_PRINT("Block allocation from mem_block_02.....\n");
	test_mem_block_get_all_blocks_cheri(ptr2, &mem_block_02, (size_t)BLK_SZ, (size_t)1,
		(int)NUM_BLOCKS);

	/* free all blocks */

	TC_PRINT("Freeing blocks to mem_block_01.....\n");
	test_mem_block_free_all_blocks_cheri(ptr1, &mem_block_01, (int)NUM_BLOCKS);
	TC_PRINT("Freeing blocks to mem_block_02.....\n");
	test_mem_block_free_all_blocks_cheri(ptr2, &mem_block_02, (int)NUM_BLOCKS);
#endif
}
