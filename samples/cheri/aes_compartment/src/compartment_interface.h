/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "aeslib.h"

struct entry_caps {

	/* Designed for AES ECB mode */
	uint8_t *in;
	int8_t insize;
	const uint8_t *key;
	int8_t keysize;
	uint8_t *out;
	int8_t outsize;
};
