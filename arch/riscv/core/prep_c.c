/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
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
 */

#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>



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
