/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#ifdef __CHERI_PURE_CAPABILITY__
/* CHERI helper functions for printing and asserting bounds information */

/* For macro slab */
static inline void print_slab_macro_cheri(const char *label, size_t slab_align, size_t block_len,
					  size_t num_blocks, void *buffer, size_t block_len_rep)
{
	uintptr_t base = __builtin_cheri_base_get(buffer);
	size_t length = __builtin_cheri_length_get(buffer);
	size_t perms = __builtin_cheri_perms_get(buffer);
	size_t tag = __builtin_cheri_tag_get(buffer);

	TC_PRINT("%s\n", label);

	TC_PRINT("  Requested block alignment: %zu, after WB_UP: %zu, and after CHERI aligned: "
		 "%zu\n",
		 slab_align, (size_t)WB_UP(slab_align),
		 (size_t)CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(slab_align)));

	TC_PRINT("  Requested block size: %zu, after WB_UP: %zu after macro rounding to CHERI "
		 "representable length: %zu,after CHERI builtins rounding during init: %zu\n",
		 block_len, (size_t)WB_UP(block_len),
		 (size_t)CHERI_ROUND_UP(WB_UP(block_len),
					CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(slab_align))),
		 block_len_rep);

	TC_PRINT(
		"  Requested slab alignment: %zu, after WB_UP: %zu, and after CHERI aligned: %zu\n",
		slab_align, (size_t)WB_UP(slab_align),
		(size_t)CHERI_ALIGN_FOR_LEN(num_blocks * CHERI_BLK_REP_LEN(block_len, slab_align),
					    WB_UP(slab_align)));

	TC_PRINT("  Requested slab size: %zu, after WB_UP: %zu and after round up to CHERI "
		 "representable length: %zu\n",
		 (num_blocks * block_len), (num_blocks * (size_t)WB_UP(block_len)),
		 (size_t)CHERI_SLAB_REP_LEN(num_blocks, block_len, slab_align));

	TC_PRINT("    Results in buffer: %p with bounds 0x%lx to 0x%lx, and size %zu bytes, perms: "
		 "0x%lx, tag: 0x%lx\n",
		 buffer, (unsigned long)base, (unsigned long)(base + length), (size_t)length,
		 (unsigned long)perms, (unsigned long)tag);
}

static inline void print_block_cheri(const char *label, int block_num, void *blk_ptr)
{
	uintptr_t base = __builtin_cheri_base_get(blk_ptr);
	size_t length = __builtin_cheri_length_get(blk_ptr);
	size_t tag = __builtin_cheri_tag_get(blk_ptr);

	TC_PRINT("%s ", label);
	TC_PRINT("block: ptr[%d] = %p with bounds 0x%lx to 0x%lx and size %zu bytes, tag: 0x%lx, ",
		 block_num, blk_ptr, (unsigned long)base, (unsigned long)(base + length),
		 (size_t)length, (unsigned long)tag);
}

static inline void assert_slab_macro_cheri(const char *label, size_t slab_align, size_t block_len,
					   size_t num_blocks, void *buffer)
{
	uintptr_t actual_slab_addr = (uintptr_t)buffer;
	size_t actual_slab_len = __builtin_cheri_length_get(buffer);

	size_t expected_slab_align = CHERI_ALIGN_FOR_LEN(
		num_blocks * CHERI_BLK_REP_LEN(block_len, slab_align), WB_UP(slab_align));
	size_t expected_slab_len = CHERI_SLAB_REP_LEN(num_blocks, block_len, slab_align);

	TC_PRINT("%s\n", label);
	zassert_equal(actual_slab_len, expected_slab_len, "Failed assert_slab_macro_cheri");
	zassert_true(IS_ALIGNED(actual_slab_addr, expected_slab_align),
		     "Failed assert_slab_macro_cheri - buffer %p not %u-byte aligned", buffer,
		     expected_slab_align);
}

static inline void assert_block_macro_cheri(const char *label, size_t slab_align, size_t block_len,
					    void *blk_ptr)
{
	uintptr_t actual_block_addr = (uintptr_t)blk_ptr;
	uintptr_t actual_block_base = __builtin_cheri_base_get(blk_ptr);
	size_t actual_block_len = __builtin_cheri_length_get(blk_ptr);

	size_t expected_block_align = CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(slab_align));
	size_t expected_block_len = CHERI_ROUND_UP(
		WB_UP(block_len), CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(slab_align)));

	TC_PRINT("%s", label);

	zassert_true(
		IS_ALIGNED((unsigned long)actual_block_base, (unsigned long)expected_block_align),
		"\nFailed assert_block_macro_cheri - blk_ptr %p not %u-byte aligned", blk_ptr,
		expected_block_align);

	zassert_equal(
		(unsigned long)actual_block_base, (unsigned long)actual_block_addr,
		"\nFailed assert_block_macro_cheri - blk_ptr base %lx and addr %lx not the same",
		(unsigned long)actual_block_base, (unsigned long)actual_block_addr);
	TC_PRINT("alignment: good, ");

	zassert_equal(actual_block_len, expected_block_len,
		      "\nFailed assert_block_macro_cheri - blk_ptr not correct length");
	TC_PRINT("block length: good\n");
}

/* For init slab */

static inline void print_slab_init_cheri(const char *label, size_t slab_align, size_t block_len,
					 size_t num_blocks, void *buffer, size_t block_len_rep)
{
	uintptr_t base = __builtin_cheri_base_get(buffer);
	size_t length = __builtin_cheri_length_get(buffer);
	size_t perms = __builtin_cheri_perms_get(buffer);
	size_t tag = __builtin_cheri_tag_get(buffer);

	size_t wb_block_len = WB_UP(block_len);
	size_t wb_slab_align = WB_UP(slab_align);

	size_t expected_slab_rep_len =
		__builtin_cheri_round_representable_length(wb_block_len * num_blocks);
	size_t builtins_slab_alignment =
		~__builtin_cheri_representable_alignment_mask(wb_block_len * num_blocks) + 1;
	size_t builtins_block_alignment =
		~__builtin_cheri_representable_alignment_mask(wb_block_len) + 1;

	size_t needed_slab_rep_len_blk =
		__builtin_cheri_round_representable_length(block_len_rep * num_blocks);
	size_t needed_slab_alignment_blk =
		~__builtin_cheri_representable_alignment_mask(block_len_rep * num_blocks) + 1;

	TC_PRINT("%s\n", label);

	TC_PRINT("  Requested block alignment: %zu, after WB_UP: %zu, CHERI builtins: %zu\n",
		 slab_align, wb_slab_align, builtins_block_alignment);
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
	TC_PRINT("  Requested block size: %zu, after WB_UP: %zu, after no CHERI rounding during "
		 "init: %zu\n",
		 block_len, wb_block_len, block_len_rep);
#else
	TC_PRINT("  Requested block size: %zu, after WB_UP: %zu, after CHERI builtins rounding "
		 "during init: %zu\n",
		 block_len, wb_block_len, block_len_rep);
#endif /* CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS */
	TC_PRINT("  Requested slab alignment: %zu, after WB_UP: %zu, expected CHERI builtins: %zu, "
		 "needed from (block_len_rep *num_blks): %zu\n",
		 slab_align, wb_slab_align, builtins_slab_alignment, needed_slab_alignment_blk);

	TC_PRINT("  Requested slab size: %zu, after WB_UP: %zu, block_len_rep*num_blks: %zu, "
		 "expected CHERI builtins: %zu, needed rounded(block_len_rep*num_blks): %zu\n",
		 (num_blocks * block_len), (num_blocks * wb_block_len),
		 (num_blocks * block_len_rep), expected_slab_rep_len, needed_slab_rep_len_blk);

	TC_PRINT("    Results in buffer: %p with bounds 0x%lx to 0x%lx, and size %zu bytes, perms: "
		 "0x%lx, tag: 0x%lx\n",
		 buffer, (unsigned long)base, (unsigned long)(base + length), (size_t)length,
		 (unsigned long)perms, (unsigned long)tag);
}

static inline void assert_block_init_cheri(const char *label, size_t slab_align, size_t block_len,
					   void *blk_ptr, size_t block_len_rep)
{
	uintptr_t actual_block_addr = (uintptr_t)blk_ptr;
	uintptr_t actual_block_base = __builtin_cheri_base_get(blk_ptr);
	size_t actual_block_len = __builtin_cheri_length_get(blk_ptr);

	TC_PRINT("%s", label);

	zassert_equal(
		(unsigned long)actual_block_base, (unsigned long)actual_block_addr,
		"\nFailed assert_block_init_cheri - blk_ptr base %lx and addr %lx not the same",
		(unsigned long)actual_block_base, (unsigned long)actual_block_addr);
	TC_PRINT("alignment: good, ");

	zassert_equal(actual_block_len, block_len_rep,
		      "\nFailed assert_block_init_cheri - blk_ptr not correct length");
	TC_PRINT("block length: good\n");
}

/* For init slab with reduced number of blocks */
#ifdef CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE

static inline void assert_slab_init_cheri_reduced(const char *label, size_t slab_align,
						  size_t block_len, size_t num_blocks, void *buffer,
						  size_t block_len_rep)
{
	TC_PRINT("%s\n", label);

	size_t actual_slab_len = __builtin_cheri_length_get(buffer);
	size_t needed_slab_rep_len_blk = block_len_rep * NUMBLOCKS_l1;
	size_t needed_slab_rep_len_blk_rnd =
		__builtin_cheri_round_representable_length(block_len_rep * NUMBLOCKS_l1);

	TC_PRINT("actual slab length: %zu, needed to fit blk rep lengths: %zu, needed slab rep "
		 "length: %zu\n",
		 actual_slab_len, needed_slab_rep_len_blk, needed_slab_rep_len_blk_rnd);

	/* Expected not to be equal in this test since the number of blocks should not fit */
	zassert_not_equal(actual_slab_len, needed_slab_rep_len_blk,
			  "Failed assert_slab_init_cheri_reduced");
	TC_PRINT("The number of blocks requested will not fit!....\n");
}
#endif /* CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE */
#endif /* __CHERI_PURE_CAPABILITY__ */
