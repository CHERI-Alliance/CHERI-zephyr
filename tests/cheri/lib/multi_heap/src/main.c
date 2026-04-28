/*
 * Copyright (c) 2026 University of Birmingham, added to support CHERI tests
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/**
 * @brief Heap tests with CHERI
 *
 * @defgroup cheri_k_heap_api_tests CHERI Heap Memory Tests
 *
 * @ingroup cheri_tests
 *
 * This complements the tests in
 * tests/lib/multi-heap but are more CHERI-specific.
 */

/*test case main entry*/
ZTEST_SUITE(cheri_mheap_api, NULL, NULL, NULL, NULL, NULL);
