/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test memory slab APIs with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_mem_slab_tests CHERI Memory Slab Tests
 *
 * @ingroup cheri_tests
 *
 * This module tests the following memory slab routines with CHERI
 *
 *     k_mem_slab_init
 *     k_mem_slab_alloc
 *     k_mem_slab_free
 *     k_mem_slab_num_used_get
 *
 * for when memory slab is defined in the form:
 *  static char __aligned(align) init_slab_buffer[length * num_blocks];
 *  static struct k_mem_slab init_slab;
 * and when memory slab is defined in the form:
 * K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align);
 */

/* CHERI memory allocation alignment:
 * See CHERI C++ programming guide.
 * Due to CHERI bounds compression, not all memory allocation requests can be exactly
 * bound to the requested alignment and length. To obtain exact bounds requires
 * increasing alignment and rounding up the length.
 *
 * For CHERI, the memory slab api has been modified for run-time alignment and length
 * rounding using the CHERI recommended builtins. However the creation of the
 * memory slab backing buffer is done at compile time, and once created the bounds of
 * the slab can not be extended to accommodate block length rounding.
 *
 * slab_macro.c
 * For the macro instantiated backing buffer created by K_MEM_SLAB_DEFINE
 * CHERI macros have been created to align and round up the size of the slab
 * to accommodate rounding and alignment requirements of a block size.
 * This ensures the slab size and its bounds will always be large enough to
 * hold the requested number of blocks with exact bounds.
 * It follows the equation:
 *  x = cheri_round(cheri_round(len) * num_blks)
 * where the macros round up the size of the blocks so they can be
 * exactly bound at run-time.
 * These are tested with:
 *   cheri_macro_slab_bounds()
 *   cheri_macro_block_bounds()
 *
 *slab_init.c
 * For the compile time static backing buffer of the form:
 *  static char __aligned(align) init_slab_buffer[length * num_blocks];
 *  static struct k_mem_slab init_slab;
 * which takes the form of the following equation:
 *  y = cheri_round(len * num_blks)
 * Here the compiler automatically rounds up and aligns the whole slab
 * according to CHERI requirements, however it does not
 * take into account rounding needed of individual blocks.
 * For many combination of block lengths / number of blocks this does
 * not cause a problem. For 64 bit CHERI exact representability is
 * guaranteed up to 4 KiB lengths. For 32 bit CHERI problems arise
 * for much smaller block sizes.
 * Tested with: cheri_init_slab_bounds();
 *              cheri_init_block_bounds();
 * several options have been implemented to deal with this:
 * a) return an error if the number of cheri-length-rounded blocks
 *    do not fit in the slab at run time. This is the default option.
 *    Tested with: cheri_init_slab_bounds_exact_fail();
 * b) reduce the number of blocks available in the slab at run-time.
 *    Tested with: cheri_init_slab_num_blks_reduced();
 *                 cheri_init_block_num_blks_reduced();
 * c) implement relaxed bounds of each block whilst maintaining
 *    overall slab bounds/requested blocks, but not recommened
 *    due to potential of allowing overflows into adjacent blocks.
 *    Tested with: cheri_init_slab_num_blks_relaxed();
 *                 cheri_init_block_num_blks_relaxed();
 * d) use macros defined for K_MEM_SLAB_DEFINE to round up the
 *    block length so it can always be fitted into the slab.
 *    Tested with: cheri_init_slab_bounds_exact_macro_pass();
 *                 cheri_init_block_bounds_exact_macro_pass();
 *
 * Further tests are included here to:
 * a) scan for alignment thresholds on different CHERI architectures.
 * to obtain precise thresholds. (approximated in powers of 2).
 * This was used to create the precise rounding macros.
 * Thresholds obtained with: cheri_scan_cheri_alignment_thresholds();
 * b) The macros are tested with: cheri_test_cheri_macros();
 * c) Extract edge case values to test with options a,b,c,d above
 * Tested with: cheri_get_slab_length_edge_cases():
 */

#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_inputs.h"
#include "inline_funcs.h"
#include "func_defs.h"

#if !CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS && !CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE
/**
 * @brief Test scanning alignment thresholds
 *
 * @details Scan CHERI alignment thresholds from the
 * builtins for a specific CHERI architecture that
 * this test is currently running on.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_a_scan_cheri_alignment_thresholds)
{
	cheri_scan_cheri_alignment_thresholds();
}

/**
 * @brief Test cheri macros
 *
 * @details Verify the cheri macros for length and
 * alignment against the builtins for each running
 * architecture
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_b_cheri_macros)
{
	cheri_test_cheri_macros();
}

/**
 * @brief test init-based slab allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the init-based slab. It runs a test within 9
 * different alignment boundaries, prints the slab length
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_c_init_slab_bounds)
{
	cheri_init_slab_bounds();
}

/**
 * @brief test init-based block allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for run-time blocks. It runs a test within 9
 * different alignment boundaries, prints the block bounds
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_d_init_block_bounds)
{
	cheri_init_block_bounds();
}

/**
 * @brief scans for edge cases for testing
 *
 * @details Scans for edge cases to test the memory slabs
 * for correct slab and bounds set up when running in CHERI mode.
 * The outputs are used by the test_inputs.h file for tests.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_e_get_slab_length_edge_cases)
{
	cheri_get_slab_length_edge_cases();
}

/**
 * @brief Verify edge case with init-based slab
 *
 * @details Verify an error is returned if the number of
 * cheri-length-rounded blocks do not fit in the slab at
 * run time. This is the default option.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_f_init_slab_edge_cases_exact_fail)
{
	cheri_init_slab_bounds_exact_fail();
}

/**
 * @brief solves edge case with macro for init-based slab
 *
 * @details Verify the use of macros defined for K_MEM_SLAB_DEFINE
 * to round up the block length so it can be fitted into the slab.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_g_init_slab_edge_cases_exact_macro_pass)
{
	cheri_init_slab_bounds_exact_macro_pass();
}
/**
 * @brief solves edge case with macro for init-based slab
 *
 * @details Verify the use of macros defined for K_MEM_SLAB_DEFINE
 * to round up the block length. Verify cheri exact bounds
 * for run-time blocks. prints the block bounds
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_h_init_block_edge_cases_exact_macro_pass)
{
	cheri_init_block_bounds_exact_macro_pass();
}

/**
 * @brief test macro-based slab allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the macro-based slab. It runs a test within 9
 * different alignment boundaries, plus an edge case,
 * prints the slab bounds and alignment details,
 * and asserts these details against what is expected.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_i_macro_slab_bounds)
{
	cheri_macro_slab_bounds();
}
/**
 * @brief test macro-based block allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for run-time blocks. It runs a test within 9
 * different alignment boundaries, plus an edge case,
 * prints the block bounds and alignment details,
 * and asserts these details against what is expected.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_j_macro_block_bounds)
{
	cheri_macro_block_bounds();
}
#endif
#ifdef CONFIG_CHERI_MEM_SLAB_RELAX_BLK_BOUNDS
/**
 * @brief solves edge case with relaxed block bounds for init-based slab
 *
 * @details Verify that the system implements relaxed
 * bounds of each block whilst maintaining overall slab
 * bounds/requested blocks, but not recommened due to
 * potential of allowing overflows into adjacent blocks.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_a_init_slab_edge_cases_bounds_relaxed)
{
	cheri_init_slab_bounds_relaxed();
}
/**
 * @brief solves edge case with relaxed block bounds for init-based slab
 *
 * @details Verify that the system implements relaxed
 * bounds of each block. Verify the requested
 * block size.
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_b_init_block_edge_cases_bounds_relaxed)
{
	cheri_init_block_bounds_relaxed();
}
#endif
#ifdef CONFIG_CHERI_MEM_SLAB_NUM_BLKS_REDUCE

/**
 * @brief solves edge case with reduced number of blocks for init-based slab
 *
 * @details Verify that the system returns a reduced number
 * of blocks when they can not all be fitted into the slab
 * due to CHERI rounding of representable lengths
 * for maintaining exact bounds
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_a_init_slab_edge_cases_num_blks_reduced)
{
	cheri_init_slab_num_blks_reduced();
}
/**
 * @brief solves edge case with reduced number of blocks for init-based slab
 *
 * @details Verify that the system returns a reduced number
 * of blocks with exact bounds
 *
 * @ingroup cheri_mem_slab_tests
 */
ZTEST(test_mem_slab_cheri, test_b_init_block_edge_cases_num_blks_reduced)
{
	cheri_init_block_num_blks_reduced();
}
#endif

/*test case main entry*/
ZTEST_SUITE(test_mem_slab_cheri, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after,
	    NULL);
