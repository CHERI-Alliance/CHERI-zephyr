/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test memory block APIs with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_mem_block_tests CHERI Memory Block Tests
 *
 * @ingroup cheri_tests
 *
 * This module tests the following memory slab routines with CHERI
 *
 *     sys_mem_blocks_alloc
 *     sys_bitarray_test_bit
 *     sys_mem_blocks_free
 *
 * for when memory blocks is defined in the form:
 * SYS_MEM_BLOCKS_DEFINE(name, blk_sz, num_blks, buf_align);
 * and
 * static uint8_t __aligned(WB_UP(balign)) buf[blk_sz * num_blks];
 * SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf);
 */

/* CHERI memory allocation alignment:
 * See CHERI C++ programming guide.
 * Due to CHERI bounds compression, not all memory allocation requests can be exactly
 * bound to the requested alignment and length. To obtain exact bounds requires
 * increasing alignment and rounding up the length.
 *
 * For CHERI, the memory blocks api has been modified within the alloc_blocks
 * function to tightly bound each block when it is allocated at run-time.
 * The bounds of a block can be represented exactly since the API design
 * stipulates the size must be a power of two.
 *
 * However the creation of the memory backing buffer is done at compile time,
 * and must be aligned and rounded correctly at compile time.
 *
 * For the macro instantiated backing buffer created by SYS_MEM_BLOCKS_DEFINE
 * CHERI macros are used to CHERI align the buffer to ensure the bounds
 * can be represented exactly.
 * These are tested with:
 *   cheri_mem_block_bounds()
 *   cheri_mem_block_block_bounds()
 *
 * For the compile time static backing buffer of the form:
 * static uint8_t __aligned(WB_UP(balign)) buf[blk_sz * num_blks];
 * SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, blk_sz, num_blks, buf);
 * Here the compiler automatically rounds up and aligns the whole buffer
 * according to CHERI requirements, with the minimum alignment
 * defined by WB_UP(balign).
 * Since the block size is defined as a power of two, this requires no
 * additional CHERI rounding.
 * These are tested with:
 * cheri_mem_block_ext_bounds();
 * cheri_mem_block_ext_block_bounds();
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>

#include "test_inputs.h"
#include "inline_funcs.h"
#include "func_defs.h"

/**
 * @brief test macro-based buffer allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the macro-based buffer. It runs a test within 9
 * different alignment boundaries, prints the buffer length
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_block_tests
 */
ZTEST(test_mem_block_cheri, test_a_mem_block_bounds)
{
	cheri_mem_block_bounds();
}

/**
 * @brief test macro-based block allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for run-time blocks. It runs a test within 9
 * different alignment boundaries, prints the block bounds
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_block_tests
 */
ZTEST(test_mem_block_cheri, test_b_mem_block_block_bounds)
{
	cheri_mem_block_block_bounds();
}

/**
 * @brief test static backing buffer allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the static backing buffer. It runs a test within 9
 * different alignment boundaries, prints the memory length
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_block_tests
 */
ZTEST(test_mem_block_cheri, test_c_mem_block_ext_bounds)
{
	cheri_mem_block_ext_bounds();
}

/**
 * @brief test block allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for run-time blocks. It runs a test within 9
 * different alignment boundaries, prints the block bounds
 * and alignment details, and asserts these details against
 * what is expected.
 *
 * @ingroup cheri_mem_block_tests
 */
ZTEST(test_mem_block_cheri, test_d_mem_block_ext_block_bounds)
{
	cheri_mem_block_ext_block_bounds();
}

/**
 * @brief test original buffer allocation CHERI bounds
 *
 * @details Verify that the system sets cheri exact bounds
 * for the original test inputs of mem_block tests. Print
 * and assert against what is expected.
 * See tests/lib/mem_blocks
 *
 * @ingroup cheri_mem_block_tests
 */
ZTEST(test_mem_block_cheri, test_e_mem_block_bounds_orig)
{
	cheri_mem_block_bounds_orig_test();
}

/*test case main entry*/
ZTEST_SUITE(test_mem_block_cheri, NULL, NULL, NULL, NULL, NULL);
