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
 *  static char __aligned(align) init_slab_buffer[length * num_blocks];
 *  static struct k_mem_slab init_slab;
 * For CHERI the memory slabs need CHERI alignment and length
 * rounding. Here the compiler automatically rounds up and aligns the whole slab
 * buffer according to CHERI requirements, however it does not
 * take into account rounding needed of individual blocks.
 *  y = cheri_round(len * num_blks)
 *
 * as detailed in main.c we test:
 * - general bounds within 9 alignment boundaries
 * - plus options when exact bounds cannot be guaranteed (edge cases):
 * a) return an error if the number of cheri-length-rounded blocks
 *    do not fit in the slab at run time. This is the default option.
 * b) reduce the number of blocks available in the slab at run-time.
 * c) implement relaxed bounds of each block - not recommended.
 * d) use macros defined for K_MEM_SLAB_DEFINE to round up the
 *    block length so it can always be fitted into the slab.
 *
 * Note 1: we round up using WB_UP because block length needs to be a multiple of
 * the alignment, and also it needs to be aligned to the size of
 * a pointer/capability. The init function fails if this is
 * not the case.
 * Note 2: in the CHERI case the requested alignment becomes the minimum
 * alignment. The buffer size is automatically CHERI rounded and aligned
 * Note 3: for 64-bit, exact representability is guaranteed up to 4KiB
 * using WB_UP(ALIGN) can represent exact bounds up to this len and alignment:
 * 8185, 16
 *
 */

/* memory slabs for 9 alignment boundaries*/
static char __aligned(WB_UP(SLABALIGN1)) init_slab_buffer1[WB_UP(BLOCKLEN1) * NUMBLOCKS1];
static struct k_mem_slab init_slab_struct1;
static char __aligned(WB_UP(SLABALIGN2)) init_slab_buffer2[WB_UP(BLOCKLEN2) * NUMBLOCKS2];
static struct k_mem_slab init_slab_struct2;
static char __aligned(WB_UP(SLABALIGN3)) init_slab_buffer3[WB_UP(BLOCKLEN3) * NUMBLOCKS3];
static struct k_mem_slab init_slab_struct3;
static char __aligned(WB_UP(SLABALIGN4)) init_slab_buffer4[WB_UP(BLOCKLEN4) * NUMBLOCKS4];
static struct k_mem_slab init_slab_struct4;
static char __aligned(WB_UP(SLABALIGN5)) init_slab_buffer5[WB_UP(BLOCKLEN5) * NUMBLOCKS5];
static struct k_mem_slab init_slab_struct5;
static char __aligned(WB_UP(SLABALIGN6)) init_slab_buffer6[WB_UP(BLOCKLEN6) * NUMBLOCKS6];
static struct k_mem_slab init_slab_struct6;
static char __aligned(WB_UP(SLABALIGN7)) init_slab_buffer7[WB_UP(BLOCKLEN7) * NUMBLOCKS7];
static struct k_mem_slab init_slab_struct7;
static char __aligned(WB_UP(SLABALIGN8)) init_slab_buffer8[WB_UP(BLOCKLEN8) * NUMBLOCKS8];
static struct k_mem_slab init_slab_struct8;
static char __aligned(WB_UP(SLABALIGN9)) init_slab_buffer9[WB_UP(BLOCKLEN9) * NUMBLOCKS9];
static struct k_mem_slab init_slab_struct9;

/*
 * memory slab for testing edge cases a), b), c)
 * (when requested block size does not fit neatly in
 * the slab after CHERI rounding)
 */
static char __aligned(WB_UP(SLABALIGN_l1)) init_slab_buffer_l1[WB_UP(BLOCKLEN_l1) * (NUMBLOCKS_l1)];
static struct k_mem_slab init_slab_struct_l1;

#ifdef __CHERI_PURE_CAPABILITY__
/*
 * memory slab for testing edge case d)
 * (use CHERI macro to guarantee block length is a
 * representable size, which will push out the slab
 * size to make it fit)
 */
static char __aligned(WB_UP(SLABALIGN_l1))
init_slab_buffer_l2[CHERI_BLK_REP_LEN(BLOCKLEN_l1, SLABALIGN_l1) * (NUMBLOCKS_l1)];
static struct k_mem_slab init_slab_struct_l2;
#endif

/* Helper functions for the tests */

/* helper print functions */
void print_init_slab_start(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
#if CONFIG_64BIT
	TC_PRINT("Checking k_mem_slab_init slab buffers are 64 bit CHERI aligned and relaxed "
		 "bound........\n");
#else
	TC_PRINT("Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and relaxed "
		 "bound........\n");
#endif /* CONFIG_64BIT */
#else
#if CONFIG_64BIT
	TC_PRINT("Checking k_mem_slab_init slab buffers are 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif /* CONFIG_64BIT */
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
#else
	TC_PRINT("Non-CHERI mode, no slab bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
}
void print_init_block_start(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
#if CONFIG_64BIT
	TC_PRINT("Checking k_mem_slab_init allocated blocks are 64 bit CHERI aligned and relaxed "
		 "bound........\n");
#else
	TC_PRINT("Checking k_mem_slab_init allocated blocks are 32 bit CHERI aligned and relaxed "
		 "bound........\n");
#endif /* CONFIG_64BIT */
#else
#if CONFIG_64BIT
	TC_PRINT("Checking k_mem_slab_init allocated blocks are 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking k_mem_slab_init allocated blocks are 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif /* CONFIG_64BIT */
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
#else
	TC_PRINT("Non-CHERI mode, no block bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
}
#ifdef __CHERI_PURE_CAPABILITY__
void assert_slab_init_cheri(const char *label, size_t slab_align, size_t block_len,
			    size_t num_blocks, void *buffer, size_t block_len_rep)
{
	size_t wb_block_len = WB_UP(block_len);

	uintptr_t actual_slab_addr = (uintptr_t)buffer;
	uintptr_t actual_slab_base = __builtin_cheri_base_get(buffer);
	size_t actual_slab_len = __builtin_cheri_length_get(buffer);

	size_t expected_slab_rep_len =
		__builtin_cheri_round_representable_length(wb_block_len * num_blocks);
	size_t needed_slab_rep_len_blk =
		__builtin_cheri_round_representable_length(block_len_rep * num_blocks);

	TC_PRINT("%s\n", label);

	zassert_equal((unsigned long)actual_slab_base, (unsigned long)actual_slab_addr,
		      "\nFailed %s - blk_ptr base %lx and addr %lx not the same", __func__,
		      (unsigned long)actual_slab_base, (unsigned long)actual_slab_addr);

	TC_PRINT("actual_slab_len: %zu, expected_slab_rep_len: %zu, needed_slab_rep_len: %zu\n",
		 actual_slab_len, expected_slab_rep_len, needed_slab_rep_len_blk);

	zassert_equal(actual_slab_len, expected_slab_rep_len,
		"Failed %s - incorrect slab size", __func__);

	zassert_equal(actual_slab_len, needed_slab_rep_len_blk,
		"Failed %s - incorrect slab size", __func__);
}
#endif /* __CHERI_PURE_CAPABILITY__ */

/*
 * allocate all blocks from the slab
 * validate that num_blocks can be allocated with
 * exact CHERI bounds
 */
void test_slab_get_all_init_blocks_cheri(void **ptr, struct k_mem_slab *slab, size_t block_len,
					 size_t slab_align, int num_blocks)
{
	TC_PRINT("  Requested length of each block: = %d bytes, after WB_UP: %d bytes\n",
		 (int)block_len, (int)WB_UP(block_len));
#ifdef __CHERI_PURE_CAPABILITY__
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
	TC_PRINT("  Expected length of each block with no CHERI rounding during init: = %d bytes\n",
		 (int)slab->info.block_size);
#else
	TC_PRINT("  Expected length of each block after CHERI builtins rounding during init: = %d "
		 "bytes\n",
		 (int)slab->info.block_size);
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
#endif /* __CHERI_PURE_CAPABILITY__ */

	void *errptr; /* Pointer to block */

	for (int i = 0; i < num_blocks; i++) {

		/* Verify number of used blocks */
		zassert_equal(k_mem_slab_num_used_get(slab), i, "Failed k_mem_slab_num_used_get");

		/* Get 1 memory block */
		zassert_equal(k_mem_slab_alloc(slab, &ptr[i], K_NO_WAIT), 0,
			      "Failed k_mem_slab_alloc");

#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  ", i, ptr[i]);
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
		size_t actual_block_len = __builtin_cheri_length_get(ptr[i]);
		size_t expected_block_len =
			__builtin_cheri_round_representable_length(WB_UP(block_len));
		zassert_equal(actual_block_len, expected_block_len,
			      "\nFailed block check - blk_ptr not correct length");
		TC_PRINT("block length: good\n");
#else
		assert_block_init_cheri("checking....", slab_align, block_len, ptr[i],
					slab->info.block_size);
#endif
#else
		TC_PRINT("  block ptr[%d] = %p\n", i, ptr[i]);
#endif /* __CHERI_PURE_CAPABILITY__ */
	} /* for */

	/* expect all blocks are used */
	zassert_equal(k_mem_slab_num_used_get(slab), num_blocks, "Failed k_mem_slab_num_used_get");

	/* Try to get one more block and it should fail */
	zassert_equal(k_mem_slab_alloc(slab, &errptr, K_NO_WAIT), -ENOMEM,
		      "Failed k_mem_slab_alloc");
} /* test_slab_get_all_init_blocks_cheri */

#ifdef CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE
/*  b) reduce the number of blocks available in the slab at run-time. */

/*
 * allocate all blocks from the slab with a reduced number of blocks
 * validate that num_blocks-1 can be allocated with exact CHERI bounds
 */
void test_slab_get_all_init_blocks_cheri_reduced(void **ptr, struct k_mem_slab *slab,
						 size_t block_len, size_t slab_align,
						 int num_blocks)
{
	TC_PRINT("  Requested number of blocks: %d, Requested length of each block: = %d bytes, "
		 "after WB_UP: %d bytes\n",
		 (int)num_blocks, (int)block_len, (int)WB_UP(block_len));
#ifdef __CHERI_PURE_CAPABILITY__

	TC_PRINT("  Expected length of each block after CHERI builtins rounding during init: = %d "
		 "bytes\n",
		 (int)slab->info.block_size);
#endif /* __CHERI_PURE_CAPABILITY__ */

	void *errptr; /* Pointer to block */
	int i = 0;    /* block index */

	/* allocate blocks until none left */
	while (k_mem_slab_alloc(slab, &ptr[i], K_NO_WAIT) == 0) {

		/* Verify number of used blocks in the map */
		zassert_equal(k_mem_slab_num_used_get(slab), i + 1,
			      "Failed k_mem_slab_num_used_get");
#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  ", i, ptr[i]);
		assert_block_init_cheri("checking....", slab_align, block_len, ptr[i],
					slab->info.block_size);
		i++;
	} /* while */
	TC_PRINT("  allocated %d blocks\n", i);
	/* Verify number of allocated blocks */
	zassert_equal(i, num_blocks - 1, "Failed incorrect number of blocks allocated");

#else
		TC_PRINT("  block ptr[%d] = %p\n", i, ptr[i]);
		i++;
	} /* while */
	TC_PRINT("  allocated %d blocks\n", i);
	/* Verify number of allocated blocks */
	zassert_equal(i, num_blocks, "Failed incorrect number of blocks allocated");
#endif /* __CHERI_PURE_CAPABILITY__ */

} /* test_slab_get_all_init_blocks_cheri_reduced */

/* Free all blocks back to the slab - validate that all blocks are freed */
void test_slab_free_all_init_blocks_cheri_reduced(void **ptr, struct k_mem_slab *slab,
						  int num_blocks)
{
	int i = 0; /* block index */

	while (k_mem_slab_num_used_get(slab) > 0) {
		/* Free memory block */
		k_mem_slab_free(slab, ptr[i]);
#ifdef __CHERI_PURE_CAPABILITY__
		print_block_cheri("  freed", i, ptr[i]);
		TC_PRINT("\n");
		i++;
	} /* while */
	TC_PRINT("  freed %d blocks\n", i);
	/* Verify number of freed blocks */
	zassert_equal(i, num_blocks - 1, "Failed incorrect number of blocks freed");
#else
		TC_PRINT("  freed block ptr[%d] = %p\n", i, ptr[i]);
		i++;
	} /* while */
	TC_PRINT("  freed %d blocks\n", i);
	/* Verify number of freed blocks */
	zassert_equal(i, num_blocks, "Failed incorrect number of blocks freed");
#endif
} /* test_slab_free_all_init_blocks_cheri_reduced */
#endif /* CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE */

/* test functions called by ZTEST */

/*
 * Checks init-based backing buffers are CHERI aligned and tightly bound
 * runs a test within 9 different alignment boundaries,
 * prints the slab length and alignment details, and
 * asserts these details against what is expected.
 */
void cheri_init_slab_bounds(void)
{

#ifdef __CHERI_PURE_CAPABILITY__
#if CONFIG_64BIT
	TC_PRINT("Checking k_mem_slab_init slab buffers are 64 bit CHERI aligned and tightly "
		 "bound........\n");
#else
	TC_PRINT("Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and tightly "
		 "bound........\n");
#endif
#else
	TC_PRINT("Non-CHERI mode, no slab bounds to test........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* if a block_size is not aligned to pointer size, slab init return error */
	zassert_not_equal(k_mem_slab_init(&init_slab_struct1, init_slab_buffer1, WB_UP(BLOCKLEN1),
					  NUMBLOCKS1),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct2, init_slab_buffer2, WB_UP(BLOCKLEN2),
					  NUMBLOCKS2),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct3, init_slab_buffer3, WB_UP(BLOCKLEN3),
					  NUMBLOCKS3),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct4, init_slab_buffer4, WB_UP(BLOCKLEN4),
					  NUMBLOCKS4),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct5, init_slab_buffer5, WB_UP(BLOCKLEN5),
					  NUMBLOCKS5),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct6, init_slab_buffer6, WB_UP(BLOCKLEN6),
					  NUMBLOCKS6),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct7, init_slab_buffer7, WB_UP(BLOCKLEN7),
					  NUMBLOCKS7),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct8, init_slab_buffer8, WB_UP(BLOCKLEN8),
					  NUMBLOCKS8),
			  -EINVAL, "block size not aligned to pointer size");
	zassert_not_equal(k_mem_slab_init(&init_slab_struct9, init_slab_buffer9, WB_UP(BLOCKLEN9),
					  NUMBLOCKS9),
			  -EINVAL, "block size not aligned to pointer size");

#ifdef __CHERI_PURE_CAPABILITY__
	/* print slab size and bounds information */
	print_slab_init_cheri("Slab init_slab_struct1, test 256 - 512 bytes........", SLABALIGN1,
			      BLOCKLEN1, NUMBLOCKS1, init_slab_struct1.buffer,
			      init_slab_struct1.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct2, test 512 bytes - 1 KiB........", SLABALIGN2,
			      BLOCKLEN2, NUMBLOCKS2, init_slab_struct2.buffer,
			      init_slab_struct2.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct3, test 1 KiB - 2kiB........", SLABALIGN3,
			      BLOCKLEN3, NUMBLOCKS3, init_slab_struct3.buffer,
			      init_slab_struct3.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct4, test 2 KiB - 4 KiB........", SLABALIGN4,
			      BLOCKLEN4, NUMBLOCKS4, init_slab_struct4.buffer,
			      init_slab_struct4.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct5, test 4 KiB - 8 KiB........", SLABALIGN5,
			      BLOCKLEN5, NUMBLOCKS5, init_slab_struct5.buffer,
			      init_slab_struct5.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct6, test 32 KiB - 64 KiB........", SLABALIGN6,
			      BLOCKLEN6, NUMBLOCKS6, init_slab_struct6.buffer,
			      init_slab_struct6.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct7, test 128 KiB - 256 KiB........", SLABALIGN7,
			      BLOCKLEN7, NUMBLOCKS7, init_slab_struct7.buffer,
			      init_slab_struct7.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct8, test 512 KiB - 1 MiB........", SLABALIGN8,
			      BLOCKLEN8, NUMBLOCKS8, init_slab_struct8.buffer,
			      init_slab_struct8.info.block_size);
	print_slab_init_cheri("Slab init_slab_struct9, test 8 MiB  - 16 MiB........", SLABALIGN9,
			      BLOCKLEN9, NUMBLOCKS9, init_slab_struct9.buffer,
			      init_slab_struct9.info.block_size);
#else
	TC_PRINT("Slab init_slab_struct1 = %p\n", init_slab_struct1.buffer);
	TC_PRINT("Slab init_slab_struct2 = %p\n", init_slab_struct2.buffer);
	TC_PRINT("Slab init_slab_struct3 = %p\n", init_slab_struct3.buffer);
	TC_PRINT("Slab init_slab_struct4 = %p\n", init_slab_struct4.buffer);
	TC_PRINT("Slab init_slab_struct5 = %p\n", init_slab_struct5.buffer);
	TC_PRINT("Slab init_slab_struct6 = %p\n", init_slab_struct6.buffer);
	TC_PRINT("Slab init_slab_struct7 = %p\n", init_slab_struct7.buffer);
	TC_PRINT("Slab init_slab_struct8 = %p\n", init_slab_struct8.buffer);
	TC_PRINT("Slab init_slab_struct9 = %p\n", init_slab_struct9.buffer);
#endif /* __CHERI_PURE_CAPABILITY__ */
#ifdef __CHERI_PURE_CAPABILITY__
	/* test assertions */
	assert_slab_init_cheri("checking init_slab_struct1.buffer.....", SLABALIGN1, BLOCKLEN1,
			       NUMBLOCKS1, init_slab_struct1.buffer,
			       init_slab_struct1.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct2.buffer.....", SLABALIGN2, BLOCKLEN2,
			       NUMBLOCKS2, init_slab_struct2.buffer,
			       init_slab_struct2.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct3.buffer.....", SLABALIGN3, BLOCKLEN3,
			       NUMBLOCKS3, init_slab_struct3.buffer,
			       init_slab_struct3.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct4.buffer.....", SLABALIGN4, BLOCKLEN4,
			       NUMBLOCKS4, init_slab_struct4.buffer,
			       init_slab_struct4.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct5.buffer.....", SLABALIGN5, BLOCKLEN5,
			       NUMBLOCKS5, init_slab_struct5.buffer,
			       init_slab_struct5.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct6.buffer.....", SLABALIGN6, BLOCKLEN6,
			       NUMBLOCKS6, init_slab_struct6.buffer,
			       init_slab_struct6.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct7.buffer.....", SLABALIGN7, BLOCKLEN7,
			       NUMBLOCKS7, init_slab_struct7.buffer,
			       init_slab_struct7.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct8.buffer.....", SLABALIGN8, BLOCKLEN8,
			       NUMBLOCKS8, init_slab_struct8.buffer,
			       init_slab_struct8.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct9.buffer.....", SLABALIGN9, BLOCKLEN9,
			       NUMBLOCKS9, init_slab_struct9.buffer,
			       init_slab_struct9.info.block_size);
#endif /* __CHERI_PURE_CAPABILITY__ */
	/* check number of blocks */
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct1), NUMBLOCKS1);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct2), NUMBLOCKS2);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct3), NUMBLOCKS3);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct4), NUMBLOCKS4);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct5), NUMBLOCKS5);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct6), NUMBLOCKS6);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct7), NUMBLOCKS7);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct8), NUMBLOCKS8);
	zassert_equal(k_mem_slab_num_free_get(&init_slab_struct9), NUMBLOCKS9);
}

/*
 * Checks all blocks can be allocated from the slab and freed.
 * runs a test within 9 different alignment boundaries,
 * prints the block bounds and alignment details, and
 * asserts these details against what is expected.
 */
void cheri_init_block_bounds(void)
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

	/* Test k_mem_slab_alloc, get all blocks */

	TC_PRINT("Block allocation from init_slab_struct1, test 256 - 512 bytes.....\n");
	test_slab_get_all_init_blocks_cheri(ptr1, &init_slab_struct1, (size_t)BLOCKLEN1,
					    (size_t)SLABALIGN1, (int)NUMBLOCKS1);

	TC_PRINT("Block allocation from init_slab_struct2, test 512 bytes - 1 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr2, &init_slab_struct2, (size_t)BLOCKLEN2,
					    (size_t)SLABALIGN2, (int)NUMBLOCKS2);

	TC_PRINT("Block allocation from init_slab_struct3, test 1 KiB - 2kiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr3, &init_slab_struct3, (size_t)BLOCKLEN3,
					    (size_t)SLABALIGN3, (int)NUMBLOCKS3);

	TC_PRINT("Block allocation from init_slab_struct4, test 2 KiB - 4 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr4, &init_slab_struct4, (size_t)BLOCKLEN4,
					    (size_t)SLABALIGN4, (int)NUMBLOCKS4);

	TC_PRINT("Block allocation from init_slab_struct5, test 4 KiB - 8 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr5, &init_slab_struct5, (size_t)BLOCKLEN5,
					    (size_t)SLABALIGN5, (int)NUMBLOCKS5);

	TC_PRINT("Block allocation from init_slab_struct6, test 32 KiB - 64 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr6, &init_slab_struct6, (size_t)BLOCKLEN6,
					    (size_t)SLABALIGN6, (int)NUMBLOCKS6);

	TC_PRINT("Block allocation from init_slab_struct7, test 128 KiB - 256 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr7, &init_slab_struct7, (size_t)BLOCKLEN7,
					    (size_t)SLABALIGN7, (int)NUMBLOCKS7);

	TC_PRINT("Block allocation from init_slab_struct8, test 512 KiB - 1 MiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr8, &init_slab_struct8, (size_t)BLOCKLEN8,
					    (size_t)SLABALIGN8, (int)NUMBLOCKS8);

	TC_PRINT("Block allocation from init_slab_struct9, test 8 MiB  - 16 MiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr9, &init_slab_struct9, (size_t)BLOCKLEN9,
					    (size_t)SLABALIGN9, (int)NUMBLOCKS9);

	/* Test k_mem_slab_free, free all blocks */

	TC_PRINT("Freeing blocks to init_slab_struct1, test 256 bytes - 512 bytes.....\n");
	test_slab_free_all_blocks_cheri(ptr1, &init_slab_struct1, NUMBLOCKS1);

	TC_PRINT("Freeing blocks to init_slab_struct2, test 512 bytes - 1 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr2, &init_slab_struct2, NUMBLOCKS2);

	TC_PRINT("Freeing blocks to init_slab_struct3, test 1 KiB - 2kiB.....\n");
	test_slab_free_all_blocks_cheri(ptr3, &init_slab_struct3, NUMBLOCKS3);

	TC_PRINT("Freeing blocks to init_slab_struct4, test 2 KiB - 4 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr4, &init_slab_struct4, NUMBLOCKS4);

	TC_PRINT("Freeing blocks to init_slab_struct5, test 4 KiB - 8 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr5, &init_slab_struct5, NUMBLOCKS5);

	TC_PRINT("Freeing blocks to init_slab_struct6, test 32 KiB - 64 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr6, &init_slab_struct6, NUMBLOCKS6);

	TC_PRINT("Freeing blocks to init_slab_struct7, test 128 KiB - 256 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr7, &init_slab_struct7, NUMBLOCKS7);

	TC_PRINT("Freeing blocks to init_slab_struct8, test 512 KiB - 1 MiB.....\n");
	test_slab_free_all_blocks_cheri(ptr8, &init_slab_struct8, NUMBLOCKS8);

	TC_PRINT("Freeing blocks to init_slab_struct9, test 8 MiB - 16 MiB.....\n");
	test_slab_free_all_blocks_cheri(ptr9, &init_slab_struct9, NUMBLOCKS9);

	/* Test re-allocation after free doesn't break capability or bounds */

	TC_PRINT("Repeat Block allocation from init_slab_struct5, test 4 KiB - 8 KiB.....\n");
	test_slab_get_all_init_blocks_cheri(ptr5, &init_slab_struct5, (size_t)BLOCKLEN5,
					    (size_t)SLABALIGN5, (int)NUMBLOCKS5);

	TC_PRINT("Repeat Freeing blocks to init_slab_struct5, test 4 KiB - 8 KiB.....\n");
	test_slab_free_all_blocks_cheri(ptr5, &init_slab_struct5, NUMBLOCKS5);
}

/*
 * Checks an edge case with option a)
 * Verifies an error is returned if the number of cheri-length-rounded blocks
 * do not fit in the slab at run time. This is the default option.
 */
void cheri_init_slab_bounds_exact_fail(void)
{
	print_init_slab_start();

#ifdef __CHERI_PURE_CAPABILITY__
	/* expect to fail with -EINVAL (CHERI block size doesn't fit in the slab) */

	zassert_equal(k_mem_slab_init(&init_slab_struct_l1, init_slab_buffer_l1, WB_UP(BLOCKLEN_l1),
				      NUMBLOCKS_l1),
		      -EINVAL, "test error - expected to fail k_mem_slab_init");

	TC_PRINT("CHERI block size DOES NOT fit in the slab, returned buffer: %p\n",
		 init_slab_struct_l1.buffer);

#endif /* __CHERI_PURE_CAPABILITY__ */
}

/*
 * Solves edge case with option d)
 * Verifies the use of macros defined for K_MEM_SLAB_DEFINE
 * to round up the block length so it can be fitted into the slab.
 */

void cheri_init_slab_bounds_exact_macro_pass(void)
{
	print_init_slab_start();

#ifdef __CHERI_PURE_CAPABILITY__
	/* expect to pass (CHERI block size does now fit in the slab) */

	zassert_equal(k_mem_slab_init(&init_slab_struct_l2, init_slab_buffer_l2, WB_UP(BLOCKLEN_l1),
				      NUMBLOCKS_l1),
		      0, "test error - expected to pass k_mem_slab_init");

	TC_PRINT("CHERI block size DOES fit in the slab, returned buffer: %p\n",
		 init_slab_struct_l2.buffer);

#endif /* __CHERI_PURE_CAPABILITY__ */
}
void cheri_init_block_bounds_exact_macro_pass(void)
{
	print_init_block_start();
#ifdef __CHERI_PURE_CAPABILITY__
	void *ptr_l2[NUMBLOCKS_l1];
	(void)memset(ptr_l2, 0, sizeof(ptr_l2));

	TC_PRINT("Block allocation from init_slab_struct_l2.....\n");
	test_slab_get_all_init_blocks_cheri(ptr_l2, &init_slab_struct_l2, (size_t)BLOCKLEN_l1,
					    (size_t)SLABALIGN_l1, (int)NUMBLOCKS_l1);

	TC_PRINT("Freeing blocks to init_slab_struct_l2, .....\n");
	test_slab_free_all_blocks_cheri(ptr_l2, &init_slab_struct_l2, NUMBLOCKS_l1);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/*
 * Solves edge case with option b)
 * reduce the number of blocks available in the slab at run-time.
 * this may be tolerated in some set up scenarios.
 */

#ifdef CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE
void cheri_init_slab_num_blks_reduced(void)
{
	print_init_slab_start();
#ifdef __CHERI_PURE_CAPABILITY__
	/* if a block_size is not aligned to pointer size, slab init return error */

	zassert_not_equal(k_mem_slab_init(&init_slab_struct_l1, init_slab_buffer_l1,
					  WB_UP(BLOCKLEN_l1), NUMBLOCKS_l1),
			  -EINVAL, "block size not aligned to pointer size");

	/* print slab size and bounds information */

	print_slab_init_cheri("Slab init_slab_struct_l1 ........", SLABALIGN_l1, BLOCKLEN_l1,
			      NUMBLOCKS_l1, init_slab_struct_l1.buffer,
			      init_slab_struct_l1.info.block_size);
	assert_slab_init_cheri_reduced("checking init_slab_struct1.buffer.....", SLABALIGN_l1,
				       BLOCKLEN_l1, NUMBLOCKS_l1, init_slab_struct_l1.buffer,
				       init_slab_struct_l1.info.block_size);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void cheri_init_block_num_blks_reduced(void)
{
	print_init_block_start();
#ifdef __CHERI_PURE_CAPABILITY__
	/* Pointer to memory block */

	void *ptr_l1[NUMBLOCKS_l1];

	/* not strictly necessary, but keeps coverity checks happy */

	(void)memset(ptr_l1, 0, sizeof(ptr_l1));

	TC_PRINT("Block allocation from init_slab_struct_l1.....\n");
	test_slab_get_all_init_blocks_cheri_reduced(ptr_l1, &init_slab_struct_l1,
						    (size_t)BLOCKLEN_l1, (size_t)SLABALIGN_l1,
						    (int)NUMBLOCKS_l1);

	TC_PRINT("Freeing blocks to init_slab_struct_l1, .....\n");
	test_slab_free_all_init_blocks_cheri_reduced(ptr_l1, &init_slab_struct_l1, NUMBLOCKS_l1);
#endif /* __CHERI_PURE_CAPABILITY__ */
}
#endif /* CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE */

/*
 * Solves edge case with option c)
 * implement relaxed bounds of each block whilst maintaining
 *    overall slab bounds/requested blocks, but not recommened
 *    due to potential of allowing overflows into adjacent blocks.
 */

#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
void cheri_init_slab_bounds_relaxed(void)
{
	print_init_slab_start();
#ifdef __CHERI_PURE_CAPABILITY__
	/* if a block_size is not aligned to pointer size, slab init return error */
	zassert_not_equal(k_mem_slab_init(&init_slab_struct_l1, init_slab_buffer_l1,
					  WB_UP(BLOCKLEN_l1), NUMBLOCKS_l1),
			  -EINVAL, "block size not aligned to pointer size");
	/* print slab size and bounds information */
	print_slab_init_cheri("Slab init_slab_struct_l1 ........", SLABALIGN_l1, BLOCKLEN_l1,
			      NUMBLOCKS_l1, init_slab_struct_l1.buffer,
			      init_slab_struct_l1.info.block_size);
	assert_slab_init_cheri("checking init_slab_struct1.buffer.....", SLABALIGN_l1, BLOCKLEN_l1,
			       NUMBLOCKS_l1, init_slab_struct_l1.buffer,
			       init_slab_struct_l1.info.block_size);
#endif /* __CHERI_PURE_CAPABILITY__ */
}

void cheri_init_block_bounds_relaxed(void)
{
	print_init_block_start();
#ifdef __CHERI_PURE_CAPABILITY__
	void *ptr_l1[NUMBLOCKS_l1];
	(void)memset(ptr_l1, 0, sizeof(ptr_l1));

	TC_PRINT("Block allocation from init_slab_struct_l1.....\n");
	test_slab_get_all_init_blocks_cheri(ptr_l1, &init_slab_struct_l1, (size_t)BLOCKLEN_l1,
					    (size_t)SLABALIGN_l1, (int)NUMBLOCKS_l1);

	TC_PRINT("Freeing blocks to init_slab_struct_l1, .....\n");
	test_slab_free_all_blocks_cheri(ptr_l1, &init_slab_struct_l1, NUMBLOCKS_l1);
#endif /* __CHERI_PURE_CAPABILITY__ */
}
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
