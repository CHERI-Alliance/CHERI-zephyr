/*
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * functions to support CHERI
 */

#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>


#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#include <zephyr/arch/riscv/cheri/cheri_funcs.h>
 #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
 /* LLVM header */
 #include <cheri_init_globals_bw.h>
 #else
 /* LLVM header */
 #include <cheri_init_globals.h>
 #endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#endif /* __CHERI_PURE_CAPABILITY__ */


#ifdef __CHERI_PURE_CAPABILITY__

/* Init cap table  - put this here for now*/
/* cheri_init_globals_3 is part of cheri llvm cheri_init_globals.h */
void init_cap_relocs(void *data_cap, void *code_cap)
{
	cheri_init_globals_3(data_cap, code_cap, data_cap);
}

/* Init global capabilities */
/* Root memory mapped device capability */
void *mmdev_root_cap = (void *)(intptr_t)-1; /* -1 puts it in the data region and not the bss. */

/* function */
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
	temp = __builtin_cheri_perms_and(temp, CHERI_PERM_DEVICE_MEMORY);
	mmdev_root_cap = temp;
	/* clear temp */
	temp = (void *) 0;
}

/*
 * Build a capability for a memory mapped device.
 * The address and size information is obtained from the initialisation
 * file for the device which uses the device tree information
 */
inline void *cheri_build_device_cap(size_t address, size_t size)
{
	void *device_cap = mmdev_root_cap;

	device_cap = __builtin_cheri_address_set(device_cap, address);
	device_cap = __builtin_cheri_bounds_set(device_cap, size);
	return (void *) device_cap;
}

#endif /* __CHERI_PURE_CAPABILITY__ */
/* ------------------------------------------------------------ */


