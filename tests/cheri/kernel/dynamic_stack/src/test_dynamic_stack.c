/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_internal.h>

#include "func_defs.h"

#ifdef __CHERI_PURE_CAPABILITY__
#include "inline_funcs.h"
#endif

#define POOL_SIZE 28672

#ifdef __CHERI_PURE_CAPABILITY__

#ifdef CONFIG_USERSPACE
#define STACK_OBJ_SIZE Z_THREAD_CHERI_REP_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#else
#define STACK_OBJ_SIZE Z_KERNEL_CHERI_REP_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#endif

#else

#ifdef CONFIG_USERSPACE
#define STACK_OBJ_SIZE K_THREAD_STACK_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#else
#define STACK_OBJ_SIZE K_KERNEL_STACK_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#endif

#endif /* __CHERI_PURE_CAPABILITY__ */

#define MAX_HEAP_STACKS (POOL_SIZE / STACK_OBJ_SIZE)

K_HEAP_DEFINE(stack_heap, POOL_SIZE);

/*
 * dynamic stack tests
 *
 * These tests validate that CHERI capability bounds are applied
 * correctly to dynamic stacks defined using either a stack heap via
 * K_HEAP_DEFINE, (CONFIG_DYNAMIC_THREAD_PREFER_POOL)
 * or from the system heap (CONFIG_DYNAMIC_THREAD_PREFER_ALLOC).
 * Tests are based on some tests from
 * tests/kernel/threads/dynamic_thread_stack with CHERI specific additions.
 * It also tests different stack sizes for CHERI rounding.
 */

/* helper functions for inspecting the stack csp thread */

/*
 * Arguments passed into the Zephyr thread entry functions require
 * void* parameters. We want to pass the stack size. In CHERI it is not
 * correct to turn integers into void* and back so create a structure
 * to be passed
 */
struct stest_params {
	size_t stack_size;
};
static struct stest_params global_params;


/*
 * Zephyr thread entry functions must have three void* args
 * even if not used. This executes on the thread’s context.
 * Here we can get the csp from the register and check its bounds
 * The asm instruction is only compatible with CHERI-RISCV
 */
void inspect_thread_stack_cap(void *a, void *b, void *c)
{

	/* get the stack size from the passed structure */
	struct stest_params *params = a;
	size_t stack_size = params->stack_size;

#ifdef __CHERI_PURE_CAPABILITY__

	TC_PRINT("Inside thread inspecting running stack capability\n");
	TC_PRINT("requested stack size: %zu\n", stack_size);

	size_t expected_stack_bounds = __builtin_cheri_round_representable_length(
					K_THREAD_STACK_LEN(stack_size));

	TC_PRINT("expected bounds size: %zu\n", expected_stack_bounds);

	/* Capture CSP capability on CHERI-RISCV only architecture */

	void *csp_cap;

	__asm__ volatile(STRINGIFY(M_CMOVE)" %0, csp" : "=C"(csp_cap) :: "memory");

	print_cheri("Thread capability stack pointer (CSP):\n", csp_cap);

	assert_thread_stack(csp_cap, expected_stack_bounds);
#endif
	/* Exit the thread */
	k_thread_abort(k_current_get());
}


/* functions used by z tests */


/*
 * Print and assert the bounds of stacks allocated from
 * an array pool in dynamic.c (CONFIG_DYNAMIC_THREAD_PREFER_POOL).
 */
void dynamic_stack_bounds_cheri_pool(void)
{

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_POOL)) {
		ztest_test_skip();
	}

#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("CONFIG_DYNAMIC_THREAD_PREFER_POOL=y\n");
	TC_PRINT("CONFIG_DYNAMIC_THREAD_STACK_SIZE:%i\n", (int)CONFIG_DYNAMIC_THREAD_STACK_SIZE);
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif

	/* Single stack allocation from pool for all threads */

	static k_tid_t tid[CONFIG_DYNAMIC_THREAD_POOL_SIZE];
	static struct k_thread th[CONFIG_DYNAMIC_THREAD_POOL_SIZE];
	static k_thread_stack_t *stack[CONFIG_DYNAMIC_THREAD_POOL_SIZE];

	/* allocate all thread stacks from the pool */

	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		stack[i] = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
						IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0);

		zassert_not_null(stack[i]);

#ifdef __CHERI_PURE_CAPABILITY__
		TC_PRINT("Stack: %i from pool\n", (int)i);

		assert_single_stack((char *)stack[i],
			CONFIG_DYNAMIC_THREAD_STACK_SIZE);

		print_cheri("", stack[i]);
#else
		TC_PRINT("stack_ptr=%p\n", stack[i]);
#endif
	}


	/* spawn our threads */
	TC_PRINT("........spawn threads........\n");
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {

		global_params.stack_size = CONFIG_DYNAMIC_THREAD_STACK_SIZE;

		tid[i] = k_thread_create(
				&th[i],
				stack[i],
				CONFIG_DYNAMIC_THREAD_STACK_SIZE,
				inspect_thread_stack_cap,      /* our inspector */
				&global_params, NULL, NULL,
				0,
				0,
				K_NO_WAIT
				);
	}

	/* join all threads */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		zassert_ok(k_thread_join(tid[i], K_FOREVER));
	}

	/* clean up stacks allocated from the pool */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		zassert_ok(k_thread_stack_free(stack[i]));
	}

}

/*
 * Print and assert the bounds of stacks allocated from
 * sys_heap in dynamic.c (CONFIG_DYNAMIC_THREAD_PREFER_ALLOC).
 */

void dynamic_stack_bounds_cheri_sysheap(void)
{

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		ztest_test_skip();
	}

#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("CONFIG_DYNAMIC_THREAD_PREFER_ALLOC=y\n");
	TC_PRINT("CONFIG_DYNAMIC_THREAD_STACK_SIZE:%i\n", (int)CONFIG_DYNAMIC_THREAD_STACK_SIZE);
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif

	size_t N;
	static k_tid_t tid[MAX_HEAP_STACKS];
	static struct k_thread th[MAX_HEAP_STACKS];
	static k_thread_stack_t *stack[MAX_HEAP_STACKS];

	/* allocate all thread stacks from the heap */

	for (N = 0; N < MAX_HEAP_STACKS; ++N) {
		stack[N] = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
						IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0);
		if (stack[N] == NULL) {

			break;
		}
#ifdef __CHERI_PURE_CAPABILITY__
		TC_PRINT("Stack: %i from heap\n", (int)N);

		assert_single_stack_sysheap((char *)stack[N],
			CONFIG_DYNAMIC_THREAD_STACK_SIZE);

		print_cheri("", stack[N]);
#endif
	}
	/* spawn our threads */
	TC_PRINT("........spawn threads........\n");
	for (N = 0; N < MAX_HEAP_STACKS; ++N) {
		if (stack[N] == NULL) {

			break;
		}

		global_params.stack_size = CONFIG_DYNAMIC_THREAD_STACK_SIZE;

		tid[N] = k_thread_create(
				&th[N],
				stack[N],
				CONFIG_DYNAMIC_THREAD_STACK_SIZE,
				inspect_thread_stack_cap,      /* our inspector */
				&global_params, NULL, NULL,
				0,
				0,
				K_NO_WAIT
				);
	}

	/* join all threads */
	for (N = 0; N < MAX_HEAP_STACKS; ++N) {
		if (stack[N] == NULL) {
			break;
		}
		zassert_ok(k_thread_join(tid[N], K_FOREVER));

	}

	/* clean up stacks allocated from the pool */
	for (N = 0; N < MAX_HEAP_STACKS; ++N) {
		if (stack[N] == NULL) {
			break;
		}
		zassert_ok(k_thread_stack_free(stack[N]));
	}

}
