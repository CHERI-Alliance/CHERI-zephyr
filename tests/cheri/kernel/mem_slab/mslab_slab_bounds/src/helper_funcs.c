/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "func_defs.h"

/*
 * Helper function to get alignment thresholds for precise macros
 *
 * Due to CHERI bounds compression, not all memory allocation requests can be exactly
 * bounded to the requested alignment and length. To obtain exact bounds requires
 * increasing alignment and rounding up the length.
 * Approximate alignment boundaries goes up in powers of two (see CHERI C++ guide)
 * and differs between architectures. Additionally the powers of two guidance is not
 * precise. For run-time alignment and length rounding the use of the CHERI
 * builtins is recommended. However if precise alignment and lengths are
 * needed at compile-time, e.g for macros, the precise boundaries
 * need to be known.
 *
 * This helper function scans and extracts the precise alignment boundaries
 * from the CHERI builtin functions on the architecture it is currently
 * running. It builds a table that can be used in a specific CHERI
 * architecture macro.
 * The tables are included and defined in:
 *    include/zephyr/arch/riscv/cheri/cheri_macros.h
 *    and are used for the mem_slab CHERI macro defined in kernel.h
 */
#ifdef __CHERI_PURE_CAPABILITY__
#define MAX_LEN CHERI_MACRO_MEM_MAX_LEN /* scan up to lengths of 16 MiB */
#endif /* __CHERI_PURE_CAPABILITY__ */

void cheri_scan_cheri_alignment_thresholds(void)
{
#ifdef __CHERI_PURE_CAPABILITY__
#ifdef CONFIG_64BIT
	TC_PRINT("scanning 64 bit alignment thresholds\n");
#else
	TC_PRINT("scanning 32 bit alignment thresholds\n");
#endif /* CONFIG_64BIT */
	size_t prev_align = 0;
	/* first threshold (len=0) is the initial align */
	/* align = ~mask + 1 */
	size_t first_align = ~(__builtin_cheri_representable_alignment_mask((size_t)0)) + 1;

	TC_PRINT("CHERI_ALIGN_TABLE(X) \\\n");
	TC_PRINT("  X(%zu, %zu) /* initial */ \\\n", (size_t)0, first_align);

	/* Scan for boundaries; emit threshold when alignment changes */
	for (size_t length = 1; length <= (size_t)MAX_LEN; ++length) {
		size_t alignment = ~(__builtin_cheri_representable_alignment_mask(length)) + 1;

		if (alignment != prev_align && length != 1) {
			/* Don’t print the initial if prev_align==0, print transitions only */
			/* print the first length at which alignment becomes 'alignment' */
			TC_PRINT("  X(%zu, %zu) \\\n", length, alignment);
		}
		prev_align = alignment;
	}
	TC_PRINT("\n");
#else
	TC_PRINT("Non-CHERI mode - nothing to scan\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/*
 * Helper functions to test the CHERI macros against the CHERI builtins
 *
 * The cheri_test_cheri_macros function scans around five alignment
 * boundary conditions to check the macros match the builtins
 * for each architecture that this test runs on.
 *
 * The test_macros_print_assert function does the checking and
 * printing.
 */
#ifdef __CHERI_PURE_CAPABILITY__
#define PRINT_MACROS 0

static inline void test_macros_print_assert(
	size_t length, size_t num_blks, size_t minalign, size_t minmask)
{
	/*
	 * eq1: When memory slab defined in the form:
	 *  static char __aligned(align) init_slab_buffer[length * num_blocks];
	 *  static struct k_mem_slab init_slab;
	 * CHERI representable rounding and alignment will be:
	 *  cheri_rep_len = round(length*num_blocks)
	 *  cheri_rep_align = align(length*num_blocks)
	 */

	/* builtins */
	size_t rep_len1_builtins = __builtin_cheri_round_representable_length(length*num_blks);
	size_t align_mask1 = __builtin_cheri_representable_alignment_mask(length*num_blks);
	/* align up to minimum alignment if required e.g size of a pointer */
	size_t rep_len1_builtins_round = (rep_len1_builtins + (minalign - 1)) & minmask;
	size_t align1_builtins = ~(align_mask1 & minmask) + 1;

	/* macros - direct from cheri_macros.h */
	/* precise macro */
	size_t align1_macro_precise =
		CHERI_ALIGN_FOR_LEN_PRECISE(length*num_blks, minalign);
	size_t rep_len1_macro_precise =
		CHERI_ROUND_UP_TO_REP_LEN_PRECISE(length*num_blks, minalign);

	/* bit-shift macro */
	size_t align1_macro_bit_shift =
		CHERI_ALIGN_FOR_LEN_BIT_SHIFT(length*num_blks, minalign);
	size_t rep_len1_macro_bit_shift =
		CHERI_ROUND_UP_TO_REP_LEN_BIT_SHIFT(length*num_blks, minalign);

	/* single bit-shift macro */
	size_t align1_macro_single =
		CHERI_ALIGN_FOR_LEN_SINGLE(length*num_blks, minalign);
	size_t rep_len1_macro_single =
		CHERI_ROUND_UP_TO_REP_LEN_SINGLE(length*num_blks, minalign);

	/* default macro */
	size_t align1_macro =
		CHERI_ALIGN_FOR_LEN(length*num_blks, minalign);
	size_t rep_len1_macro =
		CHERI_ROUND_UP_TO_REP_LEN(length*num_blks, minalign);

	/* mem_slab macro from kernel.h (uses cheri_macros.h) */
	size_t rep_len1_macro_blk =
		CHERI_BLK_REP_LEN(length*num_blks, minalign);
	size_t align1_macro_blk =
		CHERI_BLK_REP_ALIGN(length*num_blks, minalign);

	/*
	 * eq2: When memory slab defined in the form:
	 * K_MEM_SLAB_DEFINE(slab_name, length, num_blocks, align);
	 * CHERI representable rounding and alignment will be:
	 * cheri_rep_len = rounded(round(length) * num_blocks)
	 * cheri_rep_align = align(round(length) * num_blocks)
	 */

	/* builtins */
	/* do block first */
	size_t align_mask2_blk  =
		__builtin_cheri_representable_alignment_mask(length);
	/* align up to minimum alignment if required */
	size_t align_mask2_blk_rndup =
		(~(align_mask2_blk & minmask)) + 1;
	/* Round one block length to that alignment */
	/* can't use __builtin_cheri_round_representable_length because of minimum requirement */
	size_t rep_len2_builtins_blk_round =
		(length + (align_mask2_blk_rndup - 1)) & ~(align_mask2_blk_rndup - 1);
	/* then do slab */
	size_t slab_total =
		rep_len2_builtins_blk_round * num_blks;
	size_t align_mask2_slab =
		__builtin_cheri_representable_alignment_mask(slab_total);
	size_t align2_builtins =
		(~(align_mask2_slab & minmask)) + 1;
	size_t rep_len2_builtins_slab_round =
		(slab_total + (align2_builtins - 1)) & ~(align2_builtins - 1);

	/* macro - blk - direct from cheri_macros.h */
	size_t align2_macro_blk =
		CHERI_ALIGN_FOR_LEN(length, minalign);
	size_t rep_len2_macro_blk =
		CHERI_ROUND_UP_TO_REP_LEN(length, minalign);

	/* macro - slab - direct from cheri_macros.h */
	size_t align2_macro =
		CHERI_ALIGN_FOR_LEN(rep_len2_macro_blk*num_blks, minalign);
	size_t rep_len2_macro =
		CHERI_ROUND_UP_TO_REP_LEN(rep_len2_macro_blk*num_blks, minalign);

	/* mem_slab macro from kernel.h (uses cheri_macros.h) */
	size_t align2_macro_slab =
		CHERI_SLAB_REP_ALIGN(num_blks, length, minalign);
	size_t rep_len2_macro_slab =
		CHERI_SLAB_REP_LEN(num_blks, length, minalign);

	if (PRINT_MACROS > 0) {
		TC_PRINT("len:%zu, num_blks:%zu, eq.1 len_r:%zu, len_r_rd:%zu, aln:%zu, "
			"aln_map:%zu, len_r_map:%zu, aln_m:%zu, len_r_m:%zu, aln_m_a:%zu, "
			"len_r_m_a:%zu, eq.2 len_r:%zu, aln:%zu, len_r_m:%zu, aln_m:%zu\n",
		length,
		num_blks,
		rep_len1_builtins,
		rep_len1_builtins_round,
		align1_builtins,
		align1_macro_bit_shift,
		rep_len1_macro_bit_shift,
		align1_macro_precise,
		rep_len1_macro_precise,
		align1_macro_blk,
		rep_len1_macro_blk,
		rep_len2_builtins_slab_round,
		align2_builtins,
		rep_len2_macro_slab,
		align2_macro_slab);
	}

	/* check macros match builtins */

	/*eq.1 builtins vs precise macro */
	zassert_equal(rep_len1_builtins_round, rep_len1_macro_precise,
		"Failed cheri_test_cheri_macros, rep_len1_builtins_round: %zu, "
		"rep_len1_macro_precise: %zu",
		rep_len1_builtins_round, rep_len1_macro_precise);

	zassert_equal(align1_builtins, align1_macro_precise,
		"Failed cheri_test_cheri_macros, align1_builtins: %zu, "
		"align1_macro_precise: %zu,",
		align1_builtins, align1_macro_precise);

	/*eq 1. builtins vs bit-shift macro */
	zassert_equal(rep_len1_builtins_round, rep_len1_macro_bit_shift,
		"Failed cheri_test_cheri_macros, rep_len1_builtins_round: %zu, "
		"rep_len1_macro_bit_shift: %zu",
		rep_len1_builtins_round, rep_len1_macro_bit_shift);

	zassert_equal(align1_builtins, align1_macro_bit_shift,
		"Failed cheri_test_cheri_macros, align1_builtins: %zu, "
		"align1_macro_bit_shift: %zu",
		align1_builtins, align1_macro_bit_shift);

	/*eq 1. builtins vs single bit-shift macro */
	zassert_equal(rep_len1_builtins_round, rep_len1_macro_single,
		"Failed cheri_test_cheri_macros, rep_len1_builtins_round: %zu, "
		"rep_len1_macro_single: %zu",
		rep_len1_builtins_round, rep_len1_macro_single);

	zassert_equal(align1_builtins, align1_macro_single,
		"Failed cheri_test_cheri_macros, align1_builtins: %zu, "
		"align1_macro_single: %zu",
		align1_builtins, align1_macro_single);

	/*eq 1. builtins vs default macro */
	zassert_equal(rep_len1_builtins_round, rep_len1_macro,
		"Failed cheri_test_cheri_macros, rep_len1_builtins_round: %zu, "
		"rep_len1_macro: %zu",
		rep_len1_builtins_round, rep_len1_macro);

	zassert_equal(align1_builtins, align1_macro,
		"Failed cheri_test_cheri_macros, align1_builtins: %zu, "
		"align1_macro: %zu",
		align1_builtins, align1_macro);

	/*eq 1. builtins vs mem_slab macro */
	zassert_equal(rep_len1_builtins_round, rep_len1_macro_blk,
		"Failed cheri_test_cheri_macros, rep_len1_builtins_round: %zu, "
		"rep_len1_macro_blk: %zu",
		rep_len1_builtins_round, rep_len1_macro_blk);

	zassert_equal(align1_builtins, align1_macro_blk,
	"Failed cheri_test_cheri_macros, align1_builtins: %zu, "
	"align1_macro_blk: %zu",
	align1_builtins, align1_macro_blk);

	/*eq 2. builtins vs default macro */
	zassert_equal(rep_len2_builtins_blk_round, rep_len2_macro_blk,
		"Failed cheri_test_cheri_macros, rep_len2_builtins_blk_round: %zu, "
		"rep_len2_macro_blk: %zu",
		rep_len2_builtins_blk_round, rep_len2_macro_blk);

	zassert_equal(rep_len2_builtins_slab_round, rep_len2_macro,
		"Failed cheri_test_cheri_macros, rep_len2_builtins_slab_round: %zu, "
		"rep_len2_macro: %zu",
		rep_len2_builtins_slab_round, rep_len2_macro);

	zassert_equal(align2_builtins, align2_macro,
		"Failed cheri_test_cheri_macros, align2_builtins: %zu, align2_macro: %zu",
		align2_builtins, align2_macro);

	/*eq 2. builtins vs mem_slab macro */
	zassert_equal(rep_len2_builtins_slab_round, rep_len2_macro_slab,
		"Failed cheri_test_cheri_macros, rep_len2_builtins_slab_round: %zu, "
		"rep_len2_macro_slab: %zu",
		rep_len2_builtins_slab_round, rep_len2_macro_slab);

	zassert_equal(align2_builtins, align2_macro_slab,
		"Failed cheri_test_cheri_macros, align2_builtins: %zu, "
		"align2_macro_slab: %zu",
		align2_builtins, align2_macro_slab);

} /* test_macros_print_assert */
#endif /* __CHERI_PURE_CAPABILITY__ */

void cheri_test_cheri_macros(void)
{
#ifdef __CHERI_PURE_CAPABILITY__

	/* align to at least size of pointer - as WB_UP() in mem_slab */
	size_t minalign = sizeof(uintptr_t);
	size_t minmask = ~(minalign-1);

	size_t min_num_blocks = 3; size_t max_num_blocks = 5;

	/* boundary conditions */
	const size_t margin = 128;
	size_t min_length1 = 128-margin;  size_t max_length1 = 128+margin;
	size_t min_length2 = 4096-margin;  size_t max_length2 = 4096+margin;
	size_t min_length3 = 8192-margin;  size_t max_length3 = 8192+margin;
	size_t min_length4 = 16384-margin; size_t max_length4 = 16384+margin;
	size_t min_length5 = 32768-margin; size_t max_length5 = 32768+margin;

	TC_PRINT("testing boundary 1\n");
	for (size_t i = min_num_blocks; i <= max_num_blocks; i++) {
		for (size_t j = min_length1; j < max_length1; j++) {
			test_macros_print_assert(j, i, minalign, minmask);
		}
	}
	TC_PRINT("testing boundary 2\n");
	for (size_t i = min_num_blocks; i <= max_num_blocks; i++) {
		for (size_t j = min_length2; j < max_length2; j++) {
			test_macros_print_assert(j, i, minalign, minmask);
		}
	}
	TC_PRINT("testing boundary 3\n");
	for (size_t i = min_num_blocks; i <= max_num_blocks; i++) {
		for (size_t j = min_length3; j < max_length3; j++) {
			test_macros_print_assert(j, i, minalign, minmask);
		}
	}
	TC_PRINT("testing boundary 4\n");
	for (size_t i = min_num_blocks; i <= max_num_blocks; i++) {
		for (size_t j = min_length4; j < max_length4; j++) {
			test_macros_print_assert(j, i, minalign, minmask);
		}
	}
	TC_PRINT("testing boundary 5\n");
	for (size_t i = min_num_blocks; i <= max_num_blocks; i++) {
		for (size_t j = min_length5; j < max_length5; j++) {
			test_macros_print_assert(j, i, minalign, minmask);
		}
	}
#else
	TC_PRINT("Non-CHERI mode - nothing to test\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
} /* cheri_test_cheri_macros */

/*
 * Helper function to detect edge cases for testing
 *
 * The cheri_get_slab_length_edge_cases function scans for
 * edge cases to test the memory slabs for correct
 * slab and bounds set up when running in CHERI mode.
 * The outputs are used by the test_inputs.h file for tests.
 *
 * It detects edge cases for testing.
 * Prints a line when x != y, where:
 *   x = cheri_round( cheri_round(len) * num_blks )
 *   y = cheri_round( len * num_blks )
 * where x - is the CHERI rep length needed to fit exact CHERI bounded blocks
 *           without bounds overlap or alignment issues to both blocks and slab.
 * where y - is the CHERI rep length of a slab when the static buffer is created
 * this function finds conditions where these equations are not the same
 * and are used to test the CHERI modified version of mem-slab
 *
 * In many cases these two equations result in the same length, so
 * we need to find and include test cases where this is not true
 * to ensure test coverage.
 */
void cheri_get_slab_length_edge_cases(void)
{
	size_t min_num_blocks = 1; size_t max_num_blocks = 7;
	size_t num_to_print = 10; size_t print_num = 0;
#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("x = cheri_round( cheri_round(len) * num_blks )\n");
	TC_PRINT("y = cheri_round( len * num_blks )\n");
#if CONFIG_64BIT
	size_t min_length = 16000;  size_t max_length = 17000;

	TC_PRINT("Extracting 64 bit CHERI representable buffer lengths that differ........\n");
#else
	size_t min_length = 1;  size_t max_length = 1096;

	TC_PRINT("Extracting 32 bit CHERI representable buffer lengths that differ........\n");
#endif

	for (size_t num_blks = min_num_blocks; num_blks <= max_num_blocks; num_blks++) {
		for (size_t len = min_length; len < max_length; len++) {
			size_t len_rounded =
				__builtin_cheri_round_representable_length((size_t)WB_UP(len));
			size_t x =
				__builtin_cheri_round_representable_length(len_rounded*num_blks);

			size_t y =
				__builtin_cheri_round_representable_length(
					(size_t)WB_UP(len)*num_blks);

			if (x != y) {
				if (print_num <= num_to_print) {
					TC_PRINT("len: %zu, WB_UP(len): %zu, "
					"num_blks: %zu, x: %zu, y: %zu\n",
					len, (size_t)WB_UP(len), num_blks, x, y);
					print_num++;
				}
			}
		}
	}
#else
	TC_PRINT("Running in non-CHERI mode, no edge cases to detect........\n");
#endif /* __CHERI_PURE_CAPABILITY__ */
} /* cheri_get_slab_length_edge_cases */
