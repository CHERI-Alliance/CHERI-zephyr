/*
 * Assembly macros and helpers
 *
 * Copyright (c) 2022 BayLibre, SAS
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

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

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
   #ifdef __CHERI_PURE_CAPABILITY__
     #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
     	.macro cflr, rd, mem
	fld \rd, \mem
	.endm

	.macro cfsr, rs, mem
	fsd \rs, \mem
	.endm
     #else
    	.macro cflr, rd, mem
	cfld \rd, \mem
	.endm

	.macro cfsr, rs, mem
	cfsd \rs, \mem
	.endm
     #endif
  #else
	.macro flr, rd, mem
	fld \rd, \mem
	.endm

	.macro fsr, rs, mem
	fsd \rs, \mem
	.endm
   #endif
#else

	.macro flr, rd, mem
	flw \rd, \mem
	.endm

	.macro fsr, rs, mem
	fsw \rs, \mem
	.endm

#endif

	/*
	 * Perform rd += rs * mult using only shifts and adds.
	 * Useful when the mul instruction isn't available.
	 * mult must be a constant. rs will be clobbered.
	 */
	.macro shiftmul_add rd, rs, mult

	beqz \rs, 999f

	.set _bitpos, 0
	.set _lastbitpos, 0

	.rept 32
	.if ((\mult) & (1 << _bitpos))
	.if (_bitpos - _lastbitpos) != 0
	slli \rs, \rs, (_bitpos - _lastbitpos)
	.set _lastbitpos, _bitpos
	.endif
	add \rd, \rd, \rs
	.endif
	.set _bitpos, _bitpos + 1
	.endr
999:
	.endm

/* lowest common denominator for register availability */
#if defined(CONFIG_RISCV_ISA_RV32E)
#define RV_E(op...) op
#define RV_I(op...) /* unavailable */
#else
#define RV_E(op...) op
#define RV_I(op...) op
#endif
