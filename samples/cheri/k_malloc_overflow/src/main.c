/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "inline_funcs.h"

#define ALLOC1		128

#define INDEX_SZ ALLOC1 + 15 /* index should be greater than ALLOC1 + CHERI rounded to overflow */

/* This is called after the kernel’s internal fault processing.
 * This allows you to print and complete the twister test
 * after a CHERI hardware exception.
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

/* Allocate some memory from the global MEM_POOL and overflow it */
int main(void)
{
	printk("\nRunning CHERI k_malloc overflow sample on %s\n", CONFIG_BOARD_TARGET);

	/* Pointer to memory allocated */
	void *ptr1;

	/* Allocate memory from global MEM_POOL*/
	/* Note that for CHERI the bounds include the k_malloc header
	 * and any CHERI rounding / padding
	 */
	ptr1 = k_malloc(ALLOC1);

	if (ptr1 == 0) {
		printk("k_malloc failed!\n");
		return -1;
	}

	printk("k_malloc allocation address: %p\n", ptr1);
	print_cheri("", ptr1);
	printk("\nmax index: %zu\n", (size_t)INDEX_SZ);
	printk("Requested memory length: %zu\n", (size_t)ALLOC1);

	/* cast block to buffer array */
	char *ptr1_char = ptr1;

	printk("\nAttempting to overflow k_malloc allocation...\n");

	/* This will trigger a hardware exception i > BUF_SZ */
	for (int i = 0; i < INDEX_SZ; i++) {
		ptr1_char[i] = 0xab;
		/* only print subset */
		if (i == 1) {
			printk("....\n");
		}
		if (i == 0 || i > ALLOC1-3) {
			printk("written contents[%d]: 0x%02x\n", i, ptr1_char[i]);
		}
	}

	printk("k_malloc allocation overflowed!\n");
	k_free(ptr1);
	printk("Sample finished\n");

	return 0;
}
