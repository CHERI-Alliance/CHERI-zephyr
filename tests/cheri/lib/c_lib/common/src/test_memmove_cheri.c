/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/ztest.h>

#ifdef __CHERI_PURE_CAPABILITY__

#include "inline_funcs.h"

struct cap_s {
	int value1;
	int *ptr1;
	int value2;
	int *ptr2;
};

static int backing[20];

/**
 *
 * @brief Test memmove function with CHERI backward overlap
 *
 * @details Test moving memory starting backwards and
 * verifying that all capability metadata survives.
 *
 * @ingroup cheri_libc_api_tests
 */
ZTEST(libc_common_cheri, test_memmove_cheri_overlap_backward)
{
	struct cap_s buf[10];
	struct cap_s expected[10];

	/* Initialize array */
	for (int i = 0; i < 10; i++) {
		buf[i].ptr1 =
			__builtin_cheri_bounds_set(&backing[i],
						   sizeof(int));
		buf[i].value1 = i;
	}

	/* save a copy to check output */
	memcpy(&expected, &buf, sizeof(buf));

	/*
	 * Overlapping move:
	 *
	 * before:
	 *   [0][1][2][3][4][5][6][7][8][9]
	 *
	 * after:
	 *   [0][0][1][2][3][4][5][6][7][8]
	 *
	 * This must use the backwards-copy path.
	 */

	memmove(&buf[1], &buf[0],
			      9 * sizeof(buf[0]));

	for (int i = 0; i < 9; i++) {

		/* Ordinary fields */
		zassert_equal(
			buf[i + 1].value1,
			i,
			"value1 mismatch");

		/* Address preserved */
		zassert_equal(
			__builtin_cheri_address_get(buf[i + 1].ptr1),
			__builtin_cheri_address_get(expected[i].ptr1),
			"address mismatch");

		/* Tag preserved */
		zassert_true(
			__builtin_cheri_tag_get(buf[i + 1].ptr1),
			"tag lost");

		/* Bounds preserved */
		zassert_equal(
			__builtin_cheri_length_get(buf[i + 1].ptr1),
			sizeof(int),
			"bounds mismatch");

		/* Permissions preserved */
		zassert_equal(
			__builtin_cheri_perms_get(buf[i + 1].ptr1),
			__builtin_cheri_perms_get(expected[i].ptr1),
			"permissions mismatch");

		/* Base preserved */
		zassert_equal(
			__builtin_cheri_base_get(buf[i + 1].ptr1),
			__builtin_cheri_base_get(expected[i].ptr1),
			"base mismatch");
	}

}

/**
 *
 * @brief Test memmove function with CHERI backward offset
 *
 * @details Test moving memory with the same offset alignment
 * to exercise backward byte copy, capability copy, tail copy
 * and verifying that all capability metadata survives.
 *
 * @ingroup cheri_libc_api_tests
 */
ZTEST(libc_common_cheri, test_memmove_cheri_same_offset_alignment_backward)
{
	uint8_t buf[512] = {0};

	uintptr_t addr =
		((uintptr_t)(buf + 64) + sizeof(void *) - 1) &
		~(sizeof(void *) - 1);

	struct cap_s *src = (struct cap_s *)addr;
	struct cap_s expected;

	src->value1 = 123;
	src->value2 = 456;

	src->ptr1 =
		__builtin_cheri_bounds_set(&backing[0],
					   sizeof(int));

	src->ptr2 =
		__builtin_cheri_bounds_set(&backing[1],
					   sizeof(int));

	/*
	 * Place destination one capability-width into the
	 * source object. This guarantees overlap while
	 * keeping both src and dst capability-aligned.
	 */
	struct cap_s *dst =
		(struct cap_s *)((uint8_t *)src +
				 sizeof(void *));

	print_cheri("src\n", src);
	print_cheri("dst\n", dst);

	TC_PRINT("src_end=%p\n",
		 (uint8_t *)src + sizeof(*src));

	TC_PRINT("overlap=%d\n",
		 (uint8_t *)dst <
		 ((uint8_t *)src + sizeof(*src)));

	zassert_true(
		(uint8_t *)dst <
		((uint8_t *)src + sizeof(*src)),
		"test does not overlap");

	/*
	 * Copy starting one byte before each structure.
	 *
	 * This should exercise:
	 *
	 *   byte-copy
	 *     - align
	 *     - capability-copy
	 *     - tail-byte-copy
	 *
	 * while also taking the backwards memmove path.
	 */
	void *src_copy = ((uint8_t *)src) - 1;
	void *dst_copy = ((uint8_t *)dst) - 1;

	print_cheri("src_copy\n", src_copy);
	print_cheri("dst_copy\n", dst_copy);

	/* save a copy to check output */
	memcpy(&expected, src, sizeof(expected));

	/* move memory */
	memmove(dst_copy,
		      src_copy,
		      sizeof(*src) + 5);

	/* Ordinary fields */
	zassert_equal(
		dst->value1,
		expected.value1,
		"value1 mismatch");

	zassert_equal(
		dst->value2,
		expected.value2,
		"value2 mismatch");

	/* First capability address */
	zassert_equal(
		__builtin_cheri_address_get(dst->ptr1),
		__builtin_cheri_address_get(expected.ptr1),
		"ptr1 address mismatch");

	/* First capability tag */
	zassert_equal(
		__builtin_cheri_tag_get(dst->ptr1),
		__builtin_cheri_tag_get(expected.ptr1),
		"ptr1 tag mismatch");

	/* First capability bounds */
	zassert_equal(
		__builtin_cheri_length_get(dst->ptr1),
		__builtin_cheri_length_get(expected.ptr1),
		"ptr1 bounds mismatch");

	/* First capability permissions */
	zassert_equal(
		__builtin_cheri_perms_get(dst->ptr1),
		__builtin_cheri_perms_get(expected.ptr1),
		"ptr1 permissions mismatch");

	/* First capability base */
	zassert_equal(
		__builtin_cheri_base_get(dst->ptr1),
		__builtin_cheri_base_get(expected.ptr1),
		"ptr1 base mismatch");

	/* Second capability address */
	zassert_equal(
		__builtin_cheri_address_get(dst->ptr2),
		__builtin_cheri_address_get(expected.ptr2),
		"ptr1 address mismatch");

	/* Second capability tag */
	zassert_equal(
		__builtin_cheri_tag_get(dst->ptr2),
		__builtin_cheri_tag_get(expected.ptr2),
		"ptr1 tag mismatch");

	/* Second capability bounds */
	zassert_equal(
		__builtin_cheri_length_get(dst->ptr2),
		__builtin_cheri_length_get(expected.ptr2),
		"ptr1 bounds mismatch");

	/* Second capability permissions */
	zassert_equal(
		__builtin_cheri_perms_get(dst->ptr2),
		__builtin_cheri_perms_get(expected.ptr2),
		"ptr1 permissions mismatch");

	/* Second capability base */
	zassert_equal(
		__builtin_cheri_base_get(dst->ptr2),
		__builtin_cheri_base_get(expected.ptr2),
		"ptr1 base mismatch");
}

#endif
