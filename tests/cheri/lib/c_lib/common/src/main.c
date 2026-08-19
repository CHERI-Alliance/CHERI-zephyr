/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2026 University of Birmingham, modified to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test aspects of the minimal C libraries with CHERI
 *
 * @defgroup cheri_libc_api_tests CHERI minimal libc tests
 *
 * @ingroup cheri_tests
 *
 * This is a modified variation of some of the tests in
 * tests/lib/c_lib/common associated with a subset of
 * functions modified to be CHERI compatible.
 * Additional CHERI tests have been added.
 */


#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/test_toolchain.h>

#include <limits.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <zephyr/ztest_error_hook.h>


ZTEST_SUITE(libc_common_cheri, NULL, NULL, NULL, NULL, NULL);


/**
 * @brief Test memcpy operation
 *
 * @see memcpy().
 */
ZTEST(libc_common_cheri, test_memcpy)
{
	/* make sure the buffer is word aligned */
	uintptr_t mem_dest[4] = {0};
	uintptr_t mem_src[4] = {0};
	unsigned char *mem_dest_tmp = NULL;
	unsigned char *mem_src_tmp = NULL;

	unsigned char *mem_dest_byte = (unsigned char *)mem_dest;
	unsigned char *mem_src_byte = (unsigned char *)mem_src;

	/* initialize source buffer in bytes */
	for (int i = 0; i < sizeof(mem_src); i++) {
		mem_src_byte[i] = i;
	}

	/* verify when dest in not word aligned */
	mem_dest_tmp = mem_dest_byte + 1;
	mem_src_tmp = mem_src_byte;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 10),
		mem_dest_tmp, "memcpy error");
	zassert_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");

	/* restore the environment */
	memset(mem_dest_byte, '\0', sizeof(mem_dest));
	/* verify when dest and src are all in not word aligned */
	mem_dest_tmp = mem_dest_byte + sizeof(uintptr_t) - 1;
	mem_src_tmp = mem_src_byte + sizeof(uintptr_t) - 1;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 10),
		mem_dest_tmp, "memcpy error");
	zassert_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");

	/* restore the environment */
	memset(mem_dest_byte, '\0', sizeof(mem_dest));
	/* verify when the copy count is zero, the copy will directly return */
	mem_dest_tmp = mem_dest_byte + sizeof(uintptr_t) - 1;
	mem_src_tmp = mem_src_byte + sizeof(uintptr_t) - 1;
	zassert_equal(memcpy(mem_dest_tmp, mem_src_tmp, 0),
		mem_dest_tmp, "memcpy error");
	zassert_not_equal(memcmp(mem_dest_tmp, mem_src_tmp, 10),
		0, "memcpy failed");
}

/**
 * @brief Test memmove operation
 *
 * @see memmove().
 */
ZTEST(libc_common_cheri, test_memmove)
{
	char move_buffer[6] = "12123";
	char move_new[6] = {0};
	static const char move_overlap[6] = "12121";

	/* verify <src> buffer overlaps with the start of the <dest> buffer */
	zassert_equal(memmove(move_buffer + 2, move_buffer, 3), move_buffer + 2,
		     "memmove error");
	zassert_equal(memcmp(move_overlap, move_buffer, sizeof(move_buffer)), 0,
		     "memmove failed");

	/* verify the buffer is not overlap, then forward-copy */
	zassert_equal(memmove(move_new, move_buffer, sizeof(move_buffer)), move_new,
		     "memmove error");
	zassert_equal(memcmp(move_new, move_buffer, sizeof(move_buffer)), 0,
		     "memmove failed");
}

/**
 * @}
 */
