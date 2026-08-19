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
 * @brief Test memcpy function with CHERI basic
 *
 * @details Test copying a single capability and verifying
 * that all capability metadata survives.
 *
 * @ingroup cheri_libc_api_tests
 */
ZTEST(libc_common_cheri, test_memcpy_cheri_basic)
{

	int a = 7;
	int b = 8;

	void *src = &a;
	void *dst = &b;

	print_cheri("src\n", src);

	/* perform memory copy */
	memcpy(&dst, &src, sizeof(void *));

	print_cheri("dst\n", dst);

	/* Address preserved */
	zassert_equal(
		__builtin_cheri_address_get(src),
		__builtin_cheri_address_get(dst),
		"address mismatch");

	/* Tag preserved */
	zassert_equal(
		__builtin_cheri_tag_get(src),
		__builtin_cheri_tag_get(dst),
		"tag mismatch");

	/* Bounds preserved */
	zassert_equal(
		__builtin_cheri_length_get(src),
		__builtin_cheri_length_get(dst),
		"bounds mismatch");

	/* Permissions preserved */
	zassert_equal(
		__builtin_cheri_perms_get(src),
		__builtin_cheri_perms_get(dst),
		"permissions mismatch");

	/* Base preserved */
	zassert_equal(
		__builtin_cheri_base_get(src),
		__builtin_cheri_base_get(dst),
		"base mismatch");
}

/**
 *
 * @brief Test memcpy function with CHERI struct
 *
 * @details Test copying a structure with mixed
 * capabilities and data and verifying
 * that all capability metadata survives.
 *
 * @ingroup cheri_libc_api_tests
 */

ZTEST(libc_common_cheri, test_memcpy_cheri_struct)
{

	static struct cap_s src[10];
	static struct cap_s dst[10];

	/* Initialize arrays */
	for (int i = 0; i < 10; i++) {
		src[i].value1 = i;

		src[i].ptr1 =
		__builtin_cheri_bounds_set(&backing[i], sizeof(int));

		src[i].value2 = i + 100;

		src[i].ptr2 =
		__builtin_cheri_bounds_set(&backing[i + 10], sizeof(int));

		dst[i].value1 = -1;
		dst[i].ptr1 = NULL;
		dst[i].value2 = -1;
		dst[i].ptr2 = NULL;
	}

	print_cheri("src[0].ptr1\n", src[0].ptr1);

	/* perform memory copy */
	memcpy(dst, src, sizeof(src));

	print_cheri("dst[0].ptr1\n", dst[0].ptr1);

	for (int i = 0; i < 10; i++) {

		/* Ordinary fields */
		zassert_equal(
			dst[i].value1,
			src[i].value1,
			"value1 mismatch");

		zassert_equal(
			dst[i].value2,
			src[i].value2,
			"value2 mismatch");

		/* First capability address */
		zassert_equal(
			__builtin_cheri_address_get(src[i].ptr1),
			__builtin_cheri_address_get(dst[i].ptr1),
			"ptr1 address mismatch");

		/* First capability tag */
		zassert_equal(
			__builtin_cheri_tag_get(src[i].ptr1),
			__builtin_cheri_tag_get(dst[i].ptr1),
			"ptr1 tag mismatch");

		/* First capability bounds */
		zassert_equal(
			__builtin_cheri_length_get(src[i].ptr1),
			__builtin_cheri_length_get(dst[i].ptr1),
			"ptr1 bounds mismatch");

		/* First capability permissions */
		zassert_equal(
			__builtin_cheri_perms_get(src[i].ptr1),
			__builtin_cheri_perms_get(dst[i].ptr1),
			"ptr1 permissions mismatch");

		/* First capability base */
		zassert_equal(
			__builtin_cheri_base_get(src[i].ptr1),
			__builtin_cheri_base_get(dst[i].ptr1),
			"ptr1 base mismatch");

		/* Second capability address */
		zassert_equal(
			__builtin_cheri_address_get(src[i].ptr2),
			__builtin_cheri_address_get(dst[i].ptr2),
			"ptr2 address mismatch");

		/* Second capability tag */
		zassert_equal(
			__builtin_cheri_tag_get(src[i].ptr2),
			__builtin_cheri_tag_get(dst[i].ptr2),
			"ptr2 tag mismatch");

		/* Second capability bounds */
		zassert_equal(
			__builtin_cheri_length_get(src[i].ptr2),
			__builtin_cheri_length_get(dst[i].ptr2),
			"ptr2 bounds mismatch");

		/* Second capability permissions */
		zassert_equal(
			__builtin_cheri_perms_get(src[i].ptr2),
			__builtin_cheri_perms_get(dst[i].ptr2),
			"ptr2 permissions mismatch");

		/* Second capability base */
		zassert_equal(
			__builtin_cheri_base_get(src[i].ptr2),
			__builtin_cheri_base_get(dst[i].ptr2),
			"ptr2 base mismatch");
	}
}

/**
 *
 * @brief Test memcpy function with CHERI partial cap
 *
 * @details Test copying a partial capability
 * the tag bit gets cleared.
 *
 * @ingroup cheri_libc_api_tests
 */
ZTEST(libc_common_cheri, test_memcpy_cheri_partial_capability_copy)
{
	struct cap_s src;
	struct cap_s dst = {0};

	src.ptr1 = __builtin_cheri_bounds_set(&backing[0], sizeof(int));
	src.value1 = 123;

	TC_PRINT("src.value1: %d\n", src.value1);
	print_cheri("src.ptr\n", src.ptr1);

	/* Copy less than a capability */
	memcpy(&dst, &src, (sizeof(int) + (sizeof(void *) / 2)));

	TC_PRINT("dst.value1: %d\n", dst.value1);
	print_cheri("dst.ptr\n", dst.ptr1);

	/* check capability tag */
	zassert_false(
	__builtin_cheri_tag_get(dst.ptr1),
	"Partial capability copy unexpectedly retained tag");
}

/**
 *
 * @brief Test memcpy function with CHERI offset alignment
 *
 * @details Test copying with the same offset alignment
 * to exercise byte copy, capability copy, tail copy
 *
 * @ingroup cheri_libc_api_tests
 */
ZTEST(libc_common_cheri, test_memcpy_cheri_same_offset_alignment)
{
	uint8_t src_buf[128] = {0};
	uint8_t dst_buf[128] = {0};

	/*
	 * Place the struct at the next capability-aligned address
	 * inside each buffer.
	 */
	uintptr_t src_addr =
		((uintptr_t)(src_buf + 16) + sizeof(void *) - 1) &
		~(sizeof(void *) - 1);

	uintptr_t dst_addr =
		((uintptr_t)(dst_buf + 16) + sizeof(void *) - 1) &
		~(sizeof(void *) - 1);

	struct cap_s *src = (struct cap_s *)src_addr;
	struct cap_s *dst = (struct cap_s *)dst_addr;

	/* Initialise source */
	src->value1 = 123;
	src->ptr1 =
		__builtin_cheri_bounds_set(&backing[0], sizeof(int));
	src->value2 = 456;
	src->ptr2 =
		__builtin_cheri_bounds_set(&backing[1], sizeof(int));

	/*
	 * Now copy starting one byte before the struct.
	 *
	 * src_copy and dst_copy have the same non-zero alignment
	 * so memcpy should exercise:
	 *
	 *   byte-copy
	 *   - align
	 *   - capability-copy
	 *   - tail byte-copy
	 */
	void *src_copy = ((uint8_t *)src) - 1;
	void *dst_copy = ((uint8_t *)dst) - 1;

	/* set last tail bytes */
	uint8_t *src_tail = (uint8_t *)src + sizeof(*src);
	uint8_t *dst_tail = (uint8_t *)dst + sizeof(*dst);

	src_tail[3] = 36;

	TC_PRINT("src_tail[3] : %zu\n", (size_t)src_tail[3]);
	print_cheri("src->ptr1\n", src->ptr1);
	print_cheri("src->ptr2\n", src->ptr2);

	/* Copy */
	memcpy(dst_copy, src_copy, sizeof(*src) + 5);

	TC_PRINT("dst_tail[3] : %zu\n", (size_t)dst_tail[3]);
	print_cheri("dst->ptr1\n", dst->ptr1);
	print_cheri("dst->ptr2\n", dst->ptr2);

	/* Ordinary fields */
	zassert_equal(
		dst->value1,
		src->value1,
		"value1 mismatch");

	zassert_equal(
		dst->value2,
		src->value2,
		"value2 mismatch");

	zassert_equal(
		dst_tail[3],
		36,
		"tail byte 4 mismatch");

	/* First capability address */
	zassert_equal(
		__builtin_cheri_address_get(src->ptr1),
		__builtin_cheri_address_get(dst->ptr1),
		"ptr1 address mismatch");

	/* First capability tag */
	zassert_equal(
		__builtin_cheri_tag_get(src->ptr1),
		__builtin_cheri_tag_get(dst->ptr1),
		"ptr1 tag mismatch");

	/* First capability bounds */
	zassert_equal(
		__builtin_cheri_length_get(src->ptr1),
		__builtin_cheri_length_get(dst->ptr1),
		"ptr1 bounds mismatch");

	/* First capability permissions */
	zassert_equal(
		__builtin_cheri_perms_get(src->ptr1),
		__builtin_cheri_perms_get(dst->ptr1),
		"ptr1 permissions mismatch");

	/* First capability base */
	zassert_equal(
		__builtin_cheri_base_get(src->ptr1),
		__builtin_cheri_base_get(dst->ptr1),
		"ptr1 base mismatch");

	/* Second capability address */
	zassert_equal(
		__builtin_cheri_address_get(src->ptr2),
		__builtin_cheri_address_get(dst->ptr2),
		"ptr2 address mismatch");

	/* Second capability tag */
	zassert_equal(
		__builtin_cheri_tag_get(src->ptr2),
		__builtin_cheri_tag_get(dst->ptr2),
		"ptr2 tag mismatch");

	/* Second capability bounds */
	zassert_equal(
		__builtin_cheri_length_get(src->ptr2),
		__builtin_cheri_length_get(dst->ptr2),
		"ptr2 bounds mismatch");

	/* Second capability permissions */
	zassert_equal(
		__builtin_cheri_perms_get(src->ptr2),
		__builtin_cheri_perms_get(dst->ptr2),
		"ptr2 permissions mismatch");

	/* Second capability base */
	zassert_equal(
		__builtin_cheri_base_get(src->ptr2),
		__builtin_cheri_base_get(dst->ptr2),
		"ptr2 base mismatch");
}

#endif
