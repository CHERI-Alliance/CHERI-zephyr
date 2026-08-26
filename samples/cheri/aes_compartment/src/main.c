/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "inline_funcs.h"
#include "compartment_interface.h"	/* table for compartment interfacing */
#include "compartment_entry.h"		/* def of compartment funcs - only 1 */

/* Attacker compartment details */
#include "attack_interface.h"
#include "attack_entry.h"

/* enable ECB mode */
#define ECB 1
#define ENCRYPT 1
#define DECRYPT 2

#define USER_STACKSIZE 2048

/* kernel thread PCC bounds defined in linker and set at boot */
extern uintptr_t __start;
extern uintptr_t _end;

/* Create the thread / stack for a compartment
 *
 * At the moment, these must be defined globally; we could create one for each
 * test / compartment, but as they only ever run singly, it's okay to use one
 * for every test.
 */
struct k_thread compartment_thread;
K_THREAD_STACK_DEFINE(compartment_stack, USER_STACKSIZE);

/* compartment bounds extracted from linker script */
extern char __compartment1_start[];
extern char __compartment1_end[];

/* Attacker bounds extract from linker script */
extern char __compartment2_start[];
extern char __compartment2_end[];

static struct entry_caps aes_interface_table;
static struct attack_caps attack_interface_table;

/* Location of compartment information (in this case, the AES sbox) which we
 * 'leak' here to demonstrate how accessing this data is impossible from
 * another container.
 */
extern char __sbox_loc;

/* Borrowed from tiny-AES's test_encrypt_ecb for 128 bit */
const uint8_t in[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
			  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
const uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			   0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
const uint8_t out[] = {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
			   0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
const size_t msg_size = sizeof(in);
const size_t key_size = sizeof(key);
const size_t out_size = sizeof(out);

/*** standard zephyr return code for exception ***/

/* This is called after the kernel’s internal fault processing.
 * This allows you to print and complete the sample
 * after a CHERI hardware exception.
 */
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	printk(">>> %s: reason=%u esf=%p\n", __func__, reason, esf);

	if (reason == 0) {
		printk("CPU CHERI hardware exception!\n");

/* Test 3 and 6 are supposed to emit a memory protection error */
#if TEST_NO == 3
		printk("Test Complete\n");
#elif TEST_NO == 6
		printk("Test Complete\n");
#endif

	} else {
		printk("Sample finished\n");
	}

	/* halt forever, DO NOT RETURN */
	for (;;) {
		k_busy_wait(1000000);
	}
}
/*** end standard zephyr return code for exception ***/

/* simple wrapper around k_thread_create.
 *  arg1: interface_ptr (struct with objects /functions for the compartment
 *  arg2: integer (used to indicate encryption/decryption; used for tests 1-3
 *  arg3: pointer to a leaked address in one of the compartment; used for
 *		  tests 5/6
 */
void start_compartment(void *compartment_entry, void *interface_ptr, int arg1, uintptr_t *arg2)
{

	printk(">> Leaving main...\n");
	/* Create a (userspace) thread for the compartment.
	 *
	 * We use zephyr's userspace threads feature for our compartments; these
	 * will natively take advantage of CHERI's hardware to provide isolation
	 * on cheri zephyr systems.
	 *
	 */
	k_tid_t tid = k_thread_create(&compartment_thread, compartment_stack, USER_STACKSIZE,
					  compartment_entry, interface_ptr, &arg1, arg2, -1, K_USER,
					  K_MSEC(0));

	k_thread_join(tid, K_FOREVER);
	printk("<< back in main...\n");
}

/* helper method to shrink a capability to new bounds */
void *shrink_fptr(void *func, uintptr_t bound_low, uintptr_t bound_high)
{
	print_cap("bounds before reduction", func);

	/* Use the pcc to derive a new cap for the compartment */
	void *pcc = __builtin_cheri_program_counter_get();
	void *compartment_cap = __builtin_cheri_address_set(pcc, bound_low);

	/* Create a new cap which stores the proper bounds */
	size_t compartment_len = bound_high - bound_low;

	compartment_cap = __builtin_cheri_bounds_set(compartment_cap, compartment_len);

	/* Apply these bounds to the func that was passed in */
	void *bounded_func = __builtin_cheri_address_set(compartment_cap, (uintptr_t)func);

	print_cap("bounds after reduction", bounded_func);
	return bounded_func;
}

/* helper function to print array as hex; assumes AES128 */
void phex(uint8_t *str)
{
	uint8_t len = 16;

	unsigned char i;

	for (i = 0; i < len; ++i) {
		printk("%.2x", str[i]);
	}
	printk("\n");
}

/* summarize AES test */
bool test_results(struct entry_caps *aes_interface_table, const uint8_t *in, const uint8_t *out)
{

	printk("\n");
	printk("key:               ");
	for (int i = 0; i < aes_interface_table->keysize; ++i) {
		printk("%.2x", aes_interface_table->key[i]);
	}
	printk("\n");
	printk("in:                ");
	for (int i = 0; i < aes_interface_table->insize; ++i) {
		printk("%.2x", in[i]);
	}

	printk("\nEncryption result: ");
	for (int i = 0; i < aes_interface_table->outsize; ++i) {
		printk("%.2x", aes_interface_table->out[i]);
	}
	printk("\n");
	printk("Ground truth:      ");
	for (int i = 0; i < aes_interface_table->outsize; ++i) {
		printk("%.2x", out[i]);
	}
	printk("\n");

	/* test buffout matches gt */
	return (0 == memcmp((char *)aes_interface_table->out, (char *)out,
				aes_interface_table->outsize));
}

/* 1. call compartment WITHOUT reduced bounds - not a proper compartment */
void test1(struct entry_caps aes_interface_table)
{
	printk("Test 1/6: Testing encryption: full bounds - no restrictions!\n");

	/* print out current bounds info of our compartment */
	print_cap("AES library compartment bounds *before* reduction", entry);

	/* run compartment without restricting bounds */
	start_compartment(entry, &aes_interface_table, ENCRYPT, 0);

	/* Test if the AES encryption was done properly */
	bool outcome = test_results(&aes_interface_table, in, out);

	if (outcome) {
		printk("Test Complete\n");
	} else {
		printk("Test FAILED\n");
	}
}

/* 2. call compartment WITH reduced bounds */
void test2(struct entry_caps aes_interface_table)
{
	printk("Test 2/6: Testing encryption with restricted bounds - full isolation!\n");

	/* Now, reduce the bounds to the size of the compartment
	 *
	 * For experiments 2, 3, and 4, we use entry_bounded instead of
	 * entry for isolation.
	 */
	printk("Reducing library compartment bounds...\n");
	void *entry_bounded =
		shrink_fptr(entry, (uintptr_t)__compartment1_start, (uintptr_t)__compartment1_end);

	/* start the compartment */
	start_compartment(entry_bounded, &aes_interface_table, ENCRYPT, 0);

	/* Test if the AES encryption was done properly */
	bool outcome = test_results(&aes_interface_table, in, out);

	if (outcome) {
		printk("Test Complete\n");
	} else {
		printk("Test FAILED\n");
	}
}

/* 3. Demonstrate that compartments, once properly bounded, can't 'reach
 *	outside' their bounds, eg, to access a global variable. This function is
 *	the same as the last one but a conditional in the compartment code will
 *	attempt to access a variable outside the bounds of the compartment.
 */
void test3(struct entry_caps aes_interface_table)
{
	printk("Test 3/6: Testing trying to access outside a compartment!\n");

	/* Again, reduce the bounds to the size of the compartment */
	printk("Reducing library compartment bounds...\n");
	void *entry_bounded =
		shrink_fptr(entry, (uintptr_t)__compartment1_start, (uintptr_t)__compartment1_end);

	/* start the compartment */
	start_compartment(entry_bounded, &aes_interface_table, ENCRYPT, 0);

	/* Test if the AES encryption was done properly */
	bool outcome = test_results(&aes_interface_table, in, out);

	if (outcome) {
		printk("Test Complete\n");
	} else {
		printk("Test FAILED\n");
	}
}

/* 4: Test decryption with reduced bounds */
void test4(struct entry_caps aes_interface_table)
{
	printk("Test 4/6: Testing decryption with restricted bounds - full isolation!\n");

	/* Again, reduce the bounds to the size of the compartment */
	printk("Reducing library compartment bounds...\n");
	void *entry_bounded =
		shrink_fptr(entry, (uintptr_t)__compartment1_start, (uintptr_t)__compartment1_end);

	/* RESET the aes_interface_table; now 'out' is swapped with in
	 * because we're decrypting.
	 */
	uint8_t in_dyn[16] = {0};
	uint8_t out_dyn[16] = {0};

	memcpy(in_dyn, out, msg_size);
	memset(out_dyn, 0, out_size);
	aes_interface_table.in = in_dyn;
	aes_interface_table.out = out_dyn;

	/* start the compartment */
	start_compartment(entry_bounded, &aes_interface_table, DECRYPT, 0);

	/* Test if the AES decryption was done properly
	 *
	 * Here, out and in are flipped to test decryption
	 */
	bool outcome = test_results(&aes_interface_table, out, in);

	if (outcome) {
		printk("Test Complete\n");
	} else {
		printk("Test FAILED\n");
	}
}

/* 5. Now, simulate a malicious 'attack compartment' that tries to 'reach in'
 * to another compartment's memory.  This will succeced because the bounds for
 * this second compartment haven't been shrunk.
 */
void test5(struct attack_caps attack_interface_table)
{
	printk("Test 5/6: Testing attacker compartment: full bounds - no restrictions!\n");

	uintptr_t sbox_loc = (uintptr_t)&__sbox_loc;

	/* start the compartment */
	start_compartment(attack_entry, &attack_interface_table, 0, &sbox_loc);

	/* Demonstrate we were able to extract the secret data */
	printk("First 4 byte of AES compartment private data (sbox): ");
	printk("%.2x %.2x %.2x %.2x...\n", attack_interface_table.out[0],
		   attack_interface_table.out[1], attack_interface_table.out[2],
		   attack_interface_table.out[3]);
	printk("Test Complete\n");
}

/* 6. Finally, simulate how the malicious compartment is blocked from 'reaching
 *	in' to the memory of the other compartment when its bounds are properly
 *	reduced. This compartment will encounter a memory protection error as it
 *	starts to exfiltrate and end up in k_sys_fatal_error_handler().
 */
void test6(struct attack_caps attack_interface_table)
{
	printk("Test 6/6: Testing attacker compartment: restricted bounds - full isolation!\n");

	uintptr_t sbox_loc = (uintptr_t)&__sbox_loc;

	printk("Reducing attack compartment bounds...\n");
	void *attack_bounded = shrink_fptr(attack_entry, (uintptr_t)__compartment2_start,
					   (uintptr_t)__compartment2_end);

	/* start the compartment */
	start_compartment(attack_bounded, &attack_interface_table, 0, &sbox_loc);

	/* Demonstrate we were able to extract the secret data */
	printk("First 4 byte of AES compartment private data (sbox): ");
	printk("%.2x %.2x %.2x %.2x...\n", attack_interface_table.out[0],
		   attack_interface_table.out[1], attack_interface_table.out[2],
		   attack_interface_table.out[3]);

	/* This test SHOULD cause a memory protection error */
	printk("Test FAILED\n");
}

int main(void)
{
	printk("\nCHERI AES (128) compartment test on %s\n", CONFIG_BOARD_TARGET);

	/* Sanity check current pcc / bounds */
	printk("%s kernel pcc bounds __start: 0x%lx\n", __func__, (unsigned long)&__start);
	printk("%s kernel pcc bounds _end: 0x%lx\n", __func__, (unsigned long)&_end);
	print_cap(__func__, main); /* after CHERI rounding top may be > _end */

	/* populate aes_interface_table for passing caps to compartment;
	 * This is necessary in this manual PoC.
	 */
	printk("Set up interface tables for interfacing with the compartment...");

	/* pass a copy of 'in' so we preserve the original value */
	uint8_t in_dyn[16] = {0};
	uint8_t out_dyn[16] = {0};

	memcpy(in_dyn, in, msg_size);
	memset(out_dyn, 0, out_size);

	/* Setup the table */
	aes_interface_table.in = in_dyn;
	aes_interface_table.insize = msg_size;
	aes_interface_table.key = key;
	aes_interface_table.keysize = key_size;
	aes_interface_table.out = out_dyn;
	aes_interface_table.outsize = out_size;

	/* setup the attack interface table */
	attack_interface_table.memcpy_cap = memcpy;

	/* SETUP DONE */
	printk("done\n\n");

	/* execute the appropriate test */
#if TEST_NO == 1
	test1(aes_interface_table);
#elif TEST_NO == 2
	test2(aes_interface_table);
#elif TEST_NO == 3
	test3(aes_interface_table);
#elif TEST_NO == 4
	test4(aes_interface_table);
#elif TEST_NO == 5
	test5(attack_interface_table);
#elif TEST_NO == 6
	test6(attack_interface_table);
#else
	/* default to test2 to show shrinking bounds */
	test2(aes_interface_table);
#endif

	return 0;
}
