/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

#ifdef __CHERI_PURE_CAPABILITY__
/* CHERI helper functions for printing and asserting bounds information */

static inline void print_mem_block_macro_cheri(const char *label, size_t balign, size_t block_len,
					  size_t num_blocks, sys_mem_blocks_t *mem_block)
{
	uintptr_t base = __builtin_cheri_base_get(mem_block->buffer);
	size_t length = __builtin_cheri_length_get(mem_block->buffer);
	size_t perms = __builtin_cheri_perms_get(mem_block->buffer);
	size_t tag = __builtin_cheri_tag_get(mem_block->buffer);

	size_t expected_block_rep_len =
		__builtin_cheri_round_representable_length((size_t)WB_UP(block_len));

	size_t expected_buffer_rep_len =
		__builtin_cheri_round_representable_length((size_t)num_blocks * WB_UP(block_len));

	TC_PRINT("%s\n", label);

	TC_PRINT("  Requested block alignment: %zu, after WB_UP: %zu, and after CHERI aligned: "
		 "%zu\n",
		 balign, (size_t)WB_UP(balign),
		 (size_t)CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(balign)));

	TC_PRINT("  Requested block size: %zu, after WB_UP: %zu CHERI macro rounding: %zu, "
		"CHERI builtins rounding: %zu\n",
		 block_len, (size_t)WB_UP(block_len),
		 (size_t)CHERI_ROUND_UP_TO_REP_LEN(WB_UP(block_len), WB_UP(balign)),
		 expected_block_rep_len);

	TC_PRINT("  Requested buffer size: %zu, after WB_UP: %zu CHERI macro rounding: %zu, "
		 "CHERI builtins rounding: %zu\n",
		 (num_blocks * block_len), (num_blocks * (size_t)WB_UP(block_len)),
		 (size_t)CHERI_ROUND_UP_TO_REP_LEN(num_blocks * WB_UP(block_len), WB_UP(balign)),
		 expected_buffer_rep_len);

	TC_PRINT(
		"  Requested buffer alignment: %zu, after WB_UP: %zu, and after CHERI aligned: %zu\n",
		balign, (size_t)WB_UP(balign),
		(size_t)CHERI_ALIGN_FOR_LEN(num_blocks * WB_UP(block_len), WB_UP(balign)));

	TC_PRINT("    Results in buffer: %p with bounds 0x%lx to 0x%lx, and size %zu bytes, perms: "
		 "0x%lx, tag: 0x%lx\n",
		 mem_block->buffer, (unsigned long)base,
		 (unsigned long)(base + length), (size_t)length,
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

static inline void assert_mem_block_macro_cheri(const char *label, size_t balign, size_t block_len,
					   size_t num_blocks, void *buffer)
{
	uintptr_t actual_buffer_addr = (uintptr_t)buffer;
	size_t actual_buffer_len = __builtin_cheri_length_get(buffer);
	uintptr_t actual_buffer_base = __builtin_cheri_base_get(buffer);

	/* no additional CHERI rounding of block_len required because always a power of two */
	size_t expected_buffer_align = CHERI_ALIGN_FOR_LEN(
		num_blocks * WB_UP(block_len), WB_UP(balign));
	size_t expected_buffer_len = CHERI_ROUND_UP_TO_REP_LEN(
		num_blocks * WB_UP(block_len), WB_UP(balign));

	TC_PRINT("%s\n", label);
	TC_PRINT("actual_buffer_len: %zu, expected_buffer_len: %zu\n",
		actual_buffer_len, expected_buffer_len);

	zassert_equal(actual_buffer_len, expected_buffer_len,
		"Failed assert_mem_block_macro_cheri");

	zassert_true(IS_ALIGNED(actual_buffer_addr, expected_buffer_align),
		"Failed assert_mem_block_macro_cheri - buffer %p not %u-byte aligned", buffer,
		expected_buffer_align);

	zassert_equal(
		(unsigned long)actual_buffer_base, (unsigned long)actual_buffer_addr,
		"\nFailed assert_mem_block_macro_cheri - buffer_ptr base %lx and addr %lx not the same",
		(unsigned long)actual_buffer_base, (unsigned long)actual_buffer_addr);
}

static inline void assert_block_macro_cheri(const char *label, size_t balign, size_t block_len,
					    void *blk_ptr)
{
	uintptr_t actual_block_addr = (uintptr_t)blk_ptr;
	uintptr_t actual_block_base = __builtin_cheri_base_get(blk_ptr);
	size_t actual_block_len = __builtin_cheri_length_get(blk_ptr);

	size_t expected_block_align = CHERI_ALIGN_FOR_LEN(WB_UP(block_len), WB_UP(balign));
	size_t expected_block_len = CHERI_ROUND_UP_TO_REP_LEN(WB_UP(block_len), WB_UP(balign));

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
#endif /* __CHERI_PURE_CAPABILITY__ */
