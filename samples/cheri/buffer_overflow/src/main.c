/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "inline_funcs.h"

#define BUF_SZ 4
#define INDEX_SZ 8 /* index should be greater than buffer size to overflow */

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

int main(void)
{
	printk("\nRunning CHERI buffer overflow sample on %s\n", CONFIG_BOARD_TARGET);

	char buffer[BUF_SZ];

	printk("\nbuffer address: %p\n", buffer);
	printk("buffer size: %zu\n", sizeof(buffer));
	print_cheri("", buffer);
	printk("\nmax index: %zu\n", (size_t)INDEX_SZ);

	printk("\nAttempting to overflow buffer...\n");

	/* CHERI hardware exception i > BUF_SZ */
	for (int i = 0; i < INDEX_SZ; i++) {
		buffer[i] = 0xab;
		printk("written contents[%d]: 0x%02x\n", i, buffer[i]);
	}

	printk("Buffer Overflowed!\n");
	printk("Sample finished\n");

	return 0;
}
