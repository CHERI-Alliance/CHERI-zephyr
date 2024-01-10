/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri_init_globals.h>
#define RISCV_DEVICE_MMAP_SIZE 0x80000000 /* 0x0 to 0x80000000 */
#define	CHERI_PERM_LOAD				(1 << 2)	/* 0x00000004 */
#define	CHERI_PERM_STORE			(1 << 3)	/* 0x00000008 */

/* Init cap table  - put this here for now*/
/* cheri_init_globals_3 is part of cheri llvm cheri_init_globals.h */
extern void init_cap_relocs(void *data_cap, void *code_cap);

void init_cap_relocs(void *data_cap, void *code_cap)
{
	cheri_init_globals_3(data_cap, code_cap, data_cap);
}

/* Init global capabilities */
/* extern here capabilities to be used elsewhere */

/* Root memory mapped device capability */
extern void *mmdev_root_cap;
void *mmdev_root_cap = (void *)(intptr_t)-1; /* -1 puts it in the data region and not the bss. */

/* function */
extern void init_cap_roots(void *root);
void init_cap_roots(void *root)
{
	/*
	 * root is the root capability i.e ddc which can be used to
	 * create some global capabilities before the root is cleared.
	 * Create global capabilities here.
	 */

	/*
	 * Root memory mapped device capability.
	 * This capability is further reduced in size for individual
	 * devices during their initialisation.
	 */
	void *temp;

	temp = __builtin_cheri_bounds_set(root, RISCV_DEVICE_MMAP_SIZE);
	/* Permit normal load and stores only in the device memory region*/
	temp = __builtin_cheri_perms_and(temp, CHERI_PERM_LOAD | CHERI_PERM_STORE);
	mmdev_root_cap = temp;
	/* clear temp */
	temp = (void *) 0;
}

#endif /* __CHERI_PURE_CAPABILITY__ */

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

void _PrepC(void)
{
	#ifndef __CHERI_PURE_CAPABILITY__
		/* in purecap we clear the bss early on so we can initialise any global capabilities in bss */
		z_bss_zero();
	#endif
	z_data_copy();
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
	z_cstart();

	CODE_UNREACHABLE;
}
