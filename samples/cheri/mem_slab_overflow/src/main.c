/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "inline_funcs.h"

#define NUMBLOCKS1   4
#define CHERIALIGN1   16
#define BLOCKLEN1    CHERIALIGN1 * 1

#define INDEX_SZ BLOCKLEN1 + 4 /* index should be greater than BLOCKLEN1 to overflow */

static char __aligned(CHERIALIGN1) init_slab_buffer1[BLOCKLEN1 * NUMBLOCKS1];
static struct k_mem_slab init_slab_struct1;

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

/* Allocate a single block from mem_slab and overflow it */
int main(void)
{
	printk("\nRunning CHERI mem_slab block overflow sample on %s\n", CONFIG_BOARD_TARGET);

	/* Initialise slab */
	k_mem_slab_init(&init_slab_struct1, init_slab_buffer1, BLOCKLEN1, NUMBLOCKS1);

	printk("\nslab buffer address: %p\n", init_slab_struct1.buffer);
	printk("\nblock size: %zu\n", init_slab_struct1.info.block_size);

	/* Pointer to memory block - api must be void * */
	void *ptr1;

	/* Allocate one block */
	int rc = k_mem_slab_alloc(&init_slab_struct1, &ptr1, K_NO_WAIT);

	if (rc != 0) {
		printk("k_mem_slab_alloc failed: %d\n", rc);
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

	printk("mem_slab block Overflowed!\n");
	printk("Sample finished\n");

	return 0;
}
