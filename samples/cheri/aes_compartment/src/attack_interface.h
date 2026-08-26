/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "aeslib.h"

struct attack_caps {

	/* Give the attacker access to memcpy */
	void *(*memcpy_cap)(void *dst, const void *src, size_t size);

	/* Give the attacker an output sink */
	uint8_t out[256];
};
