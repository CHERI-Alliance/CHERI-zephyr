/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "compartment_interface.h"
#include "compartment_entry.h"
#include "aeslib.h"

volatile int OOB_TEST = 64;

/* here we manually define the section the compartment will reside */
[[gnu::section(".compartment1.entry"), gnu::aligned(512)]]
void entry(void *p1, void *p2, void *p3)
{
	struct entry_caps *caps = p1;
	int mode = *(int *)p2;

	/* Demonstrate thread-local stack works */
	volatile int omg = 6;

#if TEST_NO == 3
	/* any accesses OUTSIDE the compartment, e.g. to global variables like
	 * OOB_TEST will cause a memory protection error.
	 */
	omg += OOB_TEST;
#endif

	/* perform the appropriate AES operation */
	struct AES_ctx ctx;

	AES_init_ctx(&ctx, caps->key);

	if (mode == 1) {
		AES_ECB_encrypt(&ctx, caps->in);
	} else if (mode == 2) {
		AES_ECB_decrypt(&ctx, caps->in);
	}

	/* Move the data out */
	for (int i = 0; i < caps->insize; ++i) {
		caps->out[i] = caps->in[i];
	}
}
