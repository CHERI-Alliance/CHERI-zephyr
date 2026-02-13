/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_internal.h>
#include <zephyr/tracing/tracing.h>   /* declares sys_trace_idle() hook */

#include "func_defs.h"

#ifdef __CHERI_PURE_CAPABILITY__
#include "inline_funcs.h"
#endif

/*
 * IDLE stack tests
 *
 * These tests validate that CHERI capability bounds are applied correctly
 * to idle stacks defined by the kernel using K_THREAD_STACK_DEFINE,
 * K_KERNEL_STACK_DEFINE, and their array variants in
 * include/zephyr/kernel/thread_stack.h. It also checks array bounds
 * are tightened on thread initialisation in arch/riscv/core/thread.c
 */

/* helper functions for inspecting the idle stack csp thread */

#ifdef __CHERI_PURE_CAPABILITY__
/* check the csp is bounded with the expected size */
static int check_idle_thread_stack(const char *name, void *cap, size_t size_bytes)
{
	size_t cap_len  = __builtin_cheri_length_get(cap);

	/*
	 * Since this runs in the idle thread, we must not assert here.
	 * Instead return an error to be asserted later.
	 * we only check the size, we do not check the address is aligned to
	 * base since the thread is running and using the stack
	 * Also, since this runs in the idle thread, we must minimise printing.
	 */
	if (!(cap_len == size_bytes)) {
		printk("idle %s: cap_len=%zu expected=%zu\n",
			name, cap_len, size_bytes);
		return 1;
	}
	return 0;
}
#endif /* __CHERI_PURE_CAPABILITY__ */

/* Per-CPU observation flags, one flag: */
static atomic_t idle_seen_ok = ATOMIC_INIT(0);

/*
 * Zephyrs `tracing hook` called by the idle loop when CPU enters idle state
 * This executes on the idle thread’s context, so CSP is the idle CSP.
 * Here we can get the csp from the register and check its bounds
 * The asm instruction is only compatible with CHERI-RISCV
 */
void sys_trace_idle(void)
{
#ifdef __CHERI_PURE_CAPABILITY__

	/* Capture CSP capability on CHERI RISCV only architecture */

	void *csp_cap;

	__asm__ volatile(STRINGIFY(M_CMOVE) " %0, csp" : "=C"(csp_cap) :: "memory");

	/* Get the idle thread and id for the current CPU */
#ifdef CONFIG_SMP
	struct k_thread *idle = arch_curr_cpu()->idle_thread;
	unsigned int cpu_id = arch_curr_cpu()->id;
#else
	struct k_thread *idle = _current_cpu->idle_thread;
	unsigned int cpu_id = 0;
#endif /* CONFIG_SMP */

	/* return if already seen this cpu */
	if (atomic_get(&idle_seen_ok) & BIT(cpu_id)) {
		return;
	}

	if (idle != NULL) {
		size_t stack_size = idle->stack_info.size;
		/*
		 * Since this runs in the idle thread, we must minimise printing.
		 * printing here can cause issues.
		 * also we must not assert here.
		 */
		if (check_idle_thread_stack("csp", csp_cap, stack_size) < 1) {
			atomic_set(&idle_seen_ok, 1);
		}
	}
	return;
#else
	atomic_set(&idle_seen_ok, 1);
#endif /* __CHERI_PURE_CAPABILITY__ */
}


/* functions used by z tests */

/*
 * check the bounds of the current idle stack. If running with SMP
 * the bounds will cover the whole array and is created
 * using K_KERNEL_STACK_DEFINE & K_KERNEL_STACK_ARRAY_DEFINE
 */
void idle_stack_bounds_cheri(void)
{

TC_PRINT("Inspecting the idle stacks\n");

#ifdef __CHERI_PURE_CAPABILITY__
	TC_PRINT("Checking alignment and array sizes\n");
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Get the idle thread for the current CPU */
#ifdef CONFIG_SMP
	/* multicore */
	struct k_thread *idle = arch_curr_cpu()->idle_thread;
	unsigned int cpu_id = arch_curr_cpu()->id;
#else
	/* single core */
	struct k_thread *idle = _current_cpu->idle_thread;
#endif /* CONFIG_SMP */

	/* ensure stack is available */
	zassert_not_null(idle, "idle thread is NULL");

	/* get the idle thread’s stack info */
	char *start_cap = (char *)idle->stack_info.start;
	size_t stack_size = idle->stack_info.size;

#ifdef __CHERI_PURE_CAPABILITY__
	/* print and assert bounds */

	print_cheri("idle stack\n", start_cap);
	TC_PRINT("logical stack size %zu\n", stack_size);

#ifdef CONFIG_SMP
	assert_array_stack_idle(cpu_id,
			start_cap,
			stack_size);
#else
	assert_single_stack(start_cap,
			stack_size);
#endif /* CONFIG_SMP */
#endif /* __CHERI_PURE_CAPABILITY__ */
}

/*
 * check the bounds of the current idle stack csp thread.
 * here the bounds should be reduced to a single element
 * if running with SMP. This is done in riscv/core/thread.c
 *
 * This test simply waits a bit for idle to run and then
 * verifies bounds by hooking into the zephyr
 * sys_trace_idle function system hook to observe the
 * capability stack pointer (csp)
 */
void idle_csp_bounds_cheri(void)
{

	TC_PRINT("Inspecting the idle stacks stack pointer in thread\n");

#ifdef __CHERI_PURE_CAPABILITY__
#else
	TC_PRINT("Non-CHERI mode, no bounds to test\n");
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Give the system a moment to enter idle at least once */
	k_sleep(K_MSEC(5));
	zassert_true(atomic_get(&idle_seen_ok),
		"Idle CSP bounds hook did not observe expected bounds");
}
