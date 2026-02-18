/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "inline_funcs.h"

/* Stack to provoke overflow */
#define OVERFLOW_STACK_SIZE 2048
/* Size of stack memory to inspect (must be > OVERFLOW_STACK_SIZE) */
#define OVERFLOW_BYTES OVERFLOW_STACK_SIZE + 4096

K_THREAD_STACK_DEFINE(overflow_stack, OVERFLOW_STACK_SIZE);
static struct k_thread overflow_thread;

/* This is called after the kernel’s internal fault processing.
 * This allows you to print and return to main afterwards
 * Also use this because the 64-bit CHERI overloads the logging message on exception
 */
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	printk(">>> %s: reason=%u esf=%p\n", __func__, reason, esf);

	if (reason == 0) {
		printk("CPU CHERI hardware exception!\n");
	}
	printk("Sample finished\n");

	/* halt forever, DO NOT RETURN */
	for (;;) {
		k_busy_wait(1000000);
	}
}

/* thread entry function */
void overflow_entry(void *a, void *b, void *c)
{
	printk("Inside thread entry function\n");

	/* get stack pointer */
	void *sp;

#ifdef __CHERI_PURE_CAPABILITY__
	__asm__ volatile(STRINGIFY(M_CMOVE)" %0, csp" : "=C"(sp) :: "memory");
	print_cheri("Thread capability stack pointer (CSP):\n", sp);
#else
	__asm__ volatile("mv %0, sp" : "=r"(sp));
	printk("Thread stack pointer (SP) : %p\n", sp);
#endif
	/* Create a derived pointer and walk outside bounds */
	char *p = (char *)sp;

	/* CHERI will trap when i exceeds CSP bounds */
	for (int i = 0; i < OVERFLOW_BYTES; i++) {
		char val = p[i];

		printk("reading stack[%d] @ %p -> 0x%02x\n", i, (void *)&p[i], val);
	}
	printk("Stack Overflow - read past the end of the stack!\n");
	/* End the thread to avoid looping */
	k_thread_abort(k_current_get());
}

int main(void)
{
	printk("\nRunning CHERI stack overflow sample on %s\n", CONFIG_BOARD_TARGET);

	size_t stack_size = K_THREAD_STACK_SIZEOF(overflow_stack);

	printk("stack size= %zu bytes, overflow bytes= %zu\n",
		stack_size, (size_t)OVERFLOW_BYTES);

	/* Create the thread stack. */
	k_tid_t tid = k_thread_create(
	&overflow_thread,
	overflow_stack,
	stack_size,
	overflow_entry,
	NULL, NULL, NULL,
	0,
	0,
	K_NO_WAIT
	);

	/*
	 * Wait for it to finish/fault. If a bounds fault occurs, the kernel will
	 * log a trap and terminate the thread.
	 */
	k_thread_join(tid, K_FOREVER);

	printk("returned to %s\n", __func__);
	printk("Sample finished\n");
	return 0;
}
