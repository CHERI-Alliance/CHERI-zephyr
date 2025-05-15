/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/*
 * common interrupt management code for riscv SOCs supporting the riscv
 * privileged architecture specification
 */
#include <zephyr/kernel_structs.h>
#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/riscv/irq.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

/*
 * __soc_handle_irq is defined as .weak to allow re-implementation by
 * SOCs that do not truly follow the riscv privilege specification.
 */
WTEXT(__soc_handle_irq)

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 * Exception number is given as parameter via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	/* Clear exception number from CSR mip register */
	li t1, 1
	sll t0, t1, a0
	csrrc t1, mip, t0

	/* Return */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CRET
	#else
	ret
	#endif
