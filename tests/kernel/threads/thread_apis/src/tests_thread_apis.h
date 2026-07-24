/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2026 University of Birmingham, Modified to support CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_
#define ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#else
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#endif
K_THREAD_STACK_DECLARE(tstack, STACK_SIZE);
extern size_t tstack_size;
extern struct k_thread tdata;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_KERNEL_THREADS_THREAD_APIS_SRC_TEST_THREAD_APIS_H_ */
