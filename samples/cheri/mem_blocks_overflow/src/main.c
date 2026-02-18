/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/mem_blocks.h>
#include "inline_funcs.h"

#define NUMBLOCKS1   4
#define CHERIALIGN1   16
#define BLOCKLEN1    CHERIALIGN1 * 1 /* must be a power of 2 */

#define INDEX_SZ BLOCKLEN1 + 4 /* index should be greater than BLOCKLEN1 to overflow */

/* define macro-based mem_block */
SYS_MEM_BLOCKS_DEFINE(macro_mem_block1, BLOCKLEN1, NUMBLOCKS1, CHERIALIGN1);

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

/* Allocate a single block from mem_blocks and overflow it */
int main(void)
{
	printk("\nRunning CHERI mem_blocks block overflow sample on %s\n", CONFIG_BOARD_TARGET);

	printk("\nmem_block buffer address: %p\n", &macro_mem_block1);
	printk("\nblock size: %zu\n", (size_t)BLOCKLEN1);

	/* Pointer to memory block - api must be void * */
	void *ptr1;

	/* Allocate one block */
	int rc = sys_mem_blocks_alloc(&macro_mem_block1, 1, &ptr1);

	if (rc != 0) {
		printk("sys_mem_blocks_alloc failed: %d\n", rc);
		return rc;
	}

	printk("block address: %p\n", ptr1);
	print_cheri("", ptr1);
	printk("\nmax index: %zu\n", (size_t)INDEX_SZ);

	/* cast block to buffer array */
	char *ptr1_char = ptr1;

	printk("\nAttempting to overflow memory block...\n");

	/* This will trigger a hardware exception i > BUF_SZ */
	for (int i = 0; i < INDEX_SZ; i++) {
		ptr1_char[i] = 0xab;
		printk("written contents[%d]: 0x%02x\n", i, ptr1_char[i]);
	}

	printk("mem_blocks block Overflowed!\n");
	printk("Sample finished\n");

	return 0;
}
