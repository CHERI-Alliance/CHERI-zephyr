/*
 * Copyright (c) 2026 Added for Codasip CLIC & CHERI by University of Birmingham
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CODASIP_CLIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CODASIP_CLIC_H_

/* CLIC CSR number */
#define CSR_MTVT       (0x307)
#define CSR_MNXTI      (0x345)
#define CSR_MINTTHRESH (0x347)
#define CSR_MINTSTATUS (0xfb1)


#ifdef CONFIG_64BIT

   #ifdef __CHERI_PURE_CAPABILITY__
	/* integer load/store at cap addr based on ld/sd (XLEN = 64) */
	/* (for cap load/store at cap addr use clc and csc instructions) */

	#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
	.macro clr, rd, mem
	ld \rd, \mem
	.endm

	.macro csr, rs, mem
	sd \rs, \mem
	.endm
	#else
	.macro clr, rd, mem
	ld.cap \rd, \mem
	.endm

	.macro csr, rs, mem
	sd.cap \rs, \mem
	.endm
	#endif
   #else
	/* register-wide load/store based on ld/sd (XLEN = 64) */

	.macro lr, rd, mem
	ld \rd, \mem
	.endm

	.macro sr, rs, mem
	sd \rs, \mem
	.endm
   #endif
#else
   #ifdef __CHERI_PURE_CAPABILITY__
     /* integer load/store at cap addr based on lw/sw (XLEN = 32) */
     #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
	.macro clr, rd, mem
	lw \rd, \mem
	.endm

	.macro csr, rs, mem
	sw \rs, \mem
	.endm
     #else
	.macro clr, rd, mem
	lw.cap \rd, \mem
	.endm

	.macro csr, rs, mem
	sw.cap \rs, \mem
	.endm
     #endif
   #else
	/* register-wide load/store based on lw/sw (XLEN = 32) */

	.macro lr, rd, mem
	lw \rd, \mem
	.endm

	.macro sr, rs, mem
	sw \rs, \mem
	.endm
   #endif
#endif
#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CODASIP_CLIC_H_ */
