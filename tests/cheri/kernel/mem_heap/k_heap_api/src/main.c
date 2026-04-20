/*
 *  Copyright (c) 2020 Intel Corporation.
 *  Copyright (c) 2026 University of Birmingham, Added support for CHERI
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/**
 * @brief Kernel Heap API Tests with CHERI memory alignment
 * and size rounding requirements to enforce CHERI bounds
 *
 * @defgroup cheri_k_heap_api_tests Kernel Heap Tests with CHERI
 *
 * @ingroup cheri_tests
 *
 * This module performs the kheap_api tests with additional
 * CHERI tests for memory alignment and size rounding
 */
/*test case main entry*/
ZTEST_SUITE(k_heap_api_cheri, NULL, NULL, NULL, NULL, NULL);
