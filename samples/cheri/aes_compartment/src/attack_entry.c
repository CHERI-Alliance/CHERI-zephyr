/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "attack_entry.h"
#include "attack_interface.h"

/* here we manually define the section the compartment will reside */
[[gnu::section(".compartment2.entry"), gnu::aligned(512)]]
void attack_entry(void *p1, void *p2, void *p3)
{
	struct attack_caps *caps = p1;

	/* re-derive a capability based on the location of the
	 * sbox (in the other compartment)
	 */
	void *pcc = __builtin_cheri_program_counter_get();
	void *sbox_cap = __builtin_cheri_address_set(pcc, *(uintptr_t *)p3);
	uint8_t *sbox = (uint8_t *)sbox_cap;

	/* Note that if the location of the sbox is outside pcc,
	 * as it is in test #5, we'll see a capability like, eg:

	   (gdb) print sbox
	   $7 = (volatile uint8_t *) 0x800258c0 [V:1111:C:r.xCal..:0:.:0x80026800-0x8002681c] <snip>

	 * Note that the value of the captability is outside the bounds
	 * Trying to exfiltrate the sbox will fail, in this case with a memory
	 * protection error.
	 */

	/* (Attempt to) exfiltrate the sbox */
	caps->memcpy_cap(caps->out, sbox, 256);
}
