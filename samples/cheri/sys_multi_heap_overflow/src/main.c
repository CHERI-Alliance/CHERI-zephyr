/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/multi_heap.h>
#include "inline_funcs.h"

/* multi heaps */
/* number of heaps */
#define N_MULTI_HEAPS 4
/* size of each heap */
#define MHEAP_BYTES 256
/* multiheap structure */
static struct sys_multi_heap multi_heap;
/* backing buffer - align to 'at least' pointer size */
__aligned(sizeof(void *)) static char heap_mem[N_MULTI_HEAPS][MHEAP_BYTES];
/* array of ptrs to each heap structure */
static struct sys_heap mheaps[N_MULTI_HEAPS];



/* size of allocation from heap */
#define ALLOC1 128

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

void *multi_heap_choice(struct sys_multi_heap *mheap, void *cfg,
			size_t align, size_t size)
{
	struct sys_heap *h = cfg;

	return sys_heap_aligned_alloc(h, align, size);
}

/* Allocate some memory from one of the multi-heaps and overflow it */
int main(void)
{
	printk("\nRunning CHERI multi-heap overflow sample on %s\n", CONFIG_BOARD_TARGET);

	/* set allocation function to use for multi_heap */
	sys_multi_heap_init(&multi_heap, multi_heap_choice);

	/* initialise each heap structure in mheaps from backing buffer heap_mem */
	for (int i = 0; i < N_MULTI_HEAPS; i++) {
		/* individual heap access: mheaps[i].heap */
		sys_heap_init(&mheaps[i], &heap_mem[i][0], MHEAP_BYTES);
		/* register and sort heaps */
		sys_multi_heap_add_heap(&multi_heap, &mheaps[i], NULL);
	}

	/* Pointer to memory allocated */
	void *ptr1;

	/* Allocate memory from heap 3 */
	ptr1 = sys_multi_heap_alloc(&multi_heap, &mheaps[3],
						 ALLOC1);
	if (ptr1 == 0) {
		printk("sys_multi_heap_alloc failed!\n");
		return -1;
	}

	printk("multi_heap allocation address: %p\n", ptr1);
	print_cheri("", ptr1);
	printk("\nmax index: %zu\n", (size_t)INDEX_SZ);
	printk("Requested memory length: %zu\n", (size_t)ALLOC1);

	/* cast block to buffer array */
	char *ptr1_char = ptr1;

	printk("\nAttempting to overflow multi_heap allocation...\n");

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

	printk("multi_heap allocation overflowed!\n");
	/* free memory from heap 3 */
	sys_multi_heap_free(&multi_heap, ptr1);
	printk("Sample finished\n");

	return 0;
}
