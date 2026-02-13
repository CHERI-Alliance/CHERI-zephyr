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


/*
 * USER/KERNEL stack tests
 *
 * These tests validate that CHERI capability bounds are applied correctly
 * to stacks defined using K_THREAD_STACK_DEFINE,
 * K_KERNEL_STACK_DEFINE, and their array variants in
 * include/zephyr/kernel/thread_stack.h. It also checks array bounds
 * are tightened on thread initialisation in arch/riscv/core/thread.c
 */

/* User thread stacks */
K_THREAD_STACK_DEFINE(user_stack, STEST_STACKSIZE);
K_THREAD_STACK_ARRAY_DEFINE(user_stack_array, NUM_STACKS, STEST_STACKSIZE);

/* Kernel thread stacks */
K_KERNEL_STACK_DEFINE(kern_stack, STEST_STACKSIZE);
K_KERNEL_STACK_ARRAY_DEFINE(kern_stack_array, NUM_STACKS, STEST_STACKSIZE);

struct k_thread test_thread;

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

	/* Capture CSP capability on CHERI-RISCV only architecture */

	void *csp_cap;

	__asm__ volatile(STRINGIFY(M_CMOVE)" %0, csp" : "=C"(csp_cap) :: "memory");

	print_cheri("Thread capability stack pointer (CSP):\n", csp_cap);

	assert_thread_stack(csp_cap, stack_size);
#endif
	/* Exit the thread */
	k_thread_abort(k_current_get());
}

/*
 * Inspect the stack bounds outside a thread
 * print the raw base pointer
 */
static void print_stack(const char *name,
			char *cap,
			size_t size_bytes)
{
	TC_PRINT("=== %s ===\n", name);
	TC_PRINT("Raw base pointer: %p\n", cap);

#ifdef __CHERI_PURE_CAPABILITY__
	print_cheri("", cap);
#endif
	TC_PRINT("Stack allocated size (bytes): %zu\n", size_bytes);
}


/* functions used by z tests */


/*
 * print and assert the bounds of both single stacks and an array
 * of stacks. For the stack arrays the bounds will cover
 * the whole array and is created using K_KERNEL_STACK_DEFINE
 * and K_KERNEL_STACK_ARRAY_DEFINE
 */
void stack_bounds_cheri(void)
{

#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("Testing CHERI modified stack macros in thread_stack.h\n");
	TC_PRINT("Checking alignment and array sizes\n");
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif

	/* Single stacks */

	/*expect bounds to be the same as the allocated stack size */

	print_stack("user_stack",
		K_THREAD_STACK_BUFFER(user_stack),
		K_THREAD_STACK_SIZEOF(user_stack));
#ifdef __CHERI_PURE_CAPABILITY__
	assert_single_stack(
		K_THREAD_STACK_BUFFER(user_stack),
		K_THREAD_STACK_SIZEOF(user_stack));
#endif
	print_stack("kernel_stack",
		K_KERNEL_STACK_BUFFER(kern_stack),
		K_KERNEL_STACK_SIZEOF(kern_stack));
#ifdef __CHERI_PURE_CAPABILITY__
	assert_single_stack(
		K_KERNEL_STACK_BUFFER(kern_stack),
		K_KERNEL_STACK_SIZEOF(kern_stack));
#endif

	/* Arrayed stacks */

	/* expect bounds to be the fully allocated array CHERI_round(stack size * NUM_STACKS) */

	/* user stack array */
	for (int i = 0; i < NUM_STACKS; i++) {
		char name_user[32];

		snprintf(name_user, sizeof(name_user),
			"user_stack_array[%d]", i);
		/* returns with bounds of whole buffer */
		/* can't shrink bounds in macro because used in kernel setup */
		print_stack(name_user,
			K_THREAD_STACK_BUFFER(user_stack_array[i]),
			K_THREAD_STACK_SIZEOF(user_stack_array[i]));
#ifdef __CHERI_PURE_CAPABILITY__
		assert_array_stack(i,
			K_THREAD_STACK_BUFFER(user_stack_array[i]),
			K_THREAD_STACK_SIZEOF(user_stack_array[i]));
#endif
	}

	/* kernel stack array */
	for (int i = 0; i < NUM_STACKS; i++) {
		char name_kern[34];

		snprintf(name_kern, sizeof(name_kern),
			"kernel_stack_array[%d]", i);
		/* returns with bounds of whole buffer */
		/* can't shrink bounds in macro because used in kernel setup */
		print_stack(name_kern,
			K_KERNEL_STACK_BUFFER(kern_stack_array[i]),
			K_KERNEL_STACK_SIZEOF(kern_stack_array[i]));
#ifdef __CHERI_PURE_CAPABILITY__
		assert_array_stack(i,
			K_THREAD_STACK_BUFFER(user_stack_array[i]),
			K_THREAD_STACK_SIZEOF(user_stack_array[i]));
#endif
	}
}

/*
 * check the bounds of the current stack csp thread.
 * here the bounds should be reduced to a single element
 * if running with SMP. This is done in riscv/core/thread.c
 *
 * This test uses the inspect_thread_stack_cap
 * entry function to enter the thread and observe the
 * capability stack pointer (csp). The bounds
 * are checked against the expected size.
 */
void stack_threads_bounds_cheri(void)
{

#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("Testing tight bounds of individual array stacks in riscv/core/thread.c\n");
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif

	/* user thread - check bounds are tightened per element of array */

	TC_PRINT("=== Creating threads on user_stack_array ===\n");

	for (int i = 0; i < NUM_STACKS; i++) {
		static size_t stack_size = K_THREAD_STACK_SIZEOF(user_stack_array[i]);
		/* In cheri we can't turn integers into void* and back */
	       global_params.stack_size = stack_size;

		TC_PRINT("stack array [%d]\n", i);
		k_tid_t tid = k_thread_create(
		&test_thread,
		user_stack_array[i],
		stack_size,
		inspect_thread_stack_cap,      /* our inspector */
		&global_params, NULL, NULL,
		0,
		0,
		K_NO_WAIT
		);
		k_thread_join(tid, K_FOREVER);
	}

	/* kernel thread - check bounds are tightened per element of array */

	TC_PRINT("=== Creating threads on kernel_stack_array ===\n");

	for (int i = 0; i < NUM_STACKS; i++) {
		static size_t stack_size = K_KERNEL_STACK_SIZEOF(kern_stack_array[i]);
		/* In cheri we can't turn integers into void* and back */
	       global_params.stack_size = stack_size;

		TC_PRINT("stack array [%d]\n", i);
		k_tid_t tid = k_thread_create(
		&test_thread,
		kern_stack_array[i],
		stack_size,
		inspect_thread_stack_cap,      /* our inspector */
		&global_params, NULL, NULL,
		0,
		0,
		K_NO_WAIT
		);
		k_thread_join(tid, K_FOREVER);
	}
}
