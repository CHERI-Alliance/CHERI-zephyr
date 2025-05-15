/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
#if defined(CONFIG_RISCV_GP)
	/* Initialize global pointer */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE	cgp, cnull
	#else
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
	#endif
#endif


#ifndef __CHERI_PURE_CAPABILITY__
	.option norvc;
#endif


#ifdef __CHERI_PURE_CAPABILITY__

#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
/* for Zcheripurecap only, RISC-V 9.5 spec core starts in purecap mode */
/* we cannot switch or check modes for purecap only */
/* do nothing here */
#else
/* V8.0 cambridge spec - put into purecap mode */
/* cambridge cores may start in non-cap mode and so we need to switch */
/* here the auipcc instruction (see M_CMOVE_PCC) which works for cap and non-cap modes with cap registers */
/* put into purecap mode */
	.option push
	.option nocapmode

	/*
	 * May enter in non-capmode even though
	 * ELF header requests capmode.
	 * These instructions work in both modes.
	 */
	lla	t0, 1f
	/*
	* M_CMOVE_PCC uses cspecialr ct1, pcc, which uses auipcc which works for cap and non-cap modes
	* in the cambridge cores
	*/
	M_CMOVE_PCC ct1, pcc
	M_CSETADDR ct1, ct1, t0
	/*
	* If you don't set flag, cllc and auipcc instructions loose the
	* capability tag when using pcc.
	* You have to jump to the capability with the flag set so pcc
	* has the flag set for purecap
	*/
	li	t0, 1
	csetflags ct1, ct1, t0
	#ifdef __riscv_xcheri_mode_dependent_jumps
		jr.cap	ct1
	#else
		cjr	ct1
	#endif
	1:
	.option pop
#endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#endif /* __CHERI_PURE_CAPABILITY__ */

/* Bound PCC */
#ifdef __CHERI_PURE_CAPABILITY__
boundpcc:
	/* Create a reduced PCC. */
	/* this needs doing before set trap vector because cllc uses the bounds of pcc */
	M_CLLC	ct0, __start /* __start start of function/base kernel - set base addr to __start*/
	M_CLLC	ct1, _end /* _end in linker */
	sub	t2, t1, t0 /* get length */
	M_CSETADDR ct0, ct0, t0 /*set base address */
	M_CSETBOUNDS ct0, ct0, t2 /* set upper bound */
	/* set permissions - allow PCC write perms until created stacks and cleared bss in reset.S*/
	li	t2, CHERI_PERM_EXECUTABLE_AND_LOADSTORE
	M_CANDPERM ct0, ct0, t2
	M_CLLC	ct1, 1f /* where to go now */
	M_CSETADDR ct0, ct0, t1 /* set new starting address for PCC */
	M_CJR	ct0 /* jump to new PCC with reduced bounds */
/*align jump to 16 bytes for 64 bit arch */
.balign 16
1:
#endif

/* set trap vector */
#if defined(CONFIG_RISCV_VECTORED_MODE)
#if defined(CONFIG_RISCV_HAS_CLIC)

	/*
	 * CLIC vectored mode
	 *
	 * CLIC vectored mode uses mtvec exclusively for exception handling and
	 * mtvec.base must be aligned to 64 bytes (this is done using
	 * CONFIG_ARCH_SW_ISR_TABLE_ALIGN)
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
		M_CLLC	ct0, _isr_wrapper /*Load Linked Capability via Capability isr.S file */

		li t1, 0x03 /* Enable CLIC vectored mode by setting LSB */
		M_CINCOFFSET ct0, ct0, t1

		M_CSPECIALW mtcc, ct0 /*Machine trap code capability*/
		M_CMOVE	ct1, cnull
		M_CSPECIALW mtdc, ct1 /*Machine trap data capability */
	#else
		la t0, _isr_wrapper
		addi t0, t0, 0x03 /* Enable CLIC vectored mode by setting LSB */
		csrw mtvec, t0
	#endif

	/*
	 * CLIC vectored mode has a similar concept to CLINT vectored mode,
	 * where an interrupt vector table is used for specific interrupts.
	 * However, in CLIC vectored mode, the handler table contains the
	 * address of the interrupt handler instead of an opcode containing a
	 * jump instruction, this is done by leveraging
	 * CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_ADDRESS.
	 * When an interrupt occurs in CLIC vectored mode, the address of the
	 * handler entry from the vector table is loaded and then jumped to in
	 * hardware. This time mtvt is used as the base address for the
	 * interrupt table.
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
	 	M_CLLC	ct0, _irq_vector_table /* Load Linked Capability via Capability */

	 	/*
	 	 * Not sure if this will work in purcap mode as memory mapped register
	 	 * would need to contain a capability, so twice as big needed to
	 	 * store ct0  at 0x307.
	 	 * ToDo - finish this bit
	 	 */

	 #else
		la t0, _irq_vector_table
		csrw 0x307, t0 /* mtvt */
	#endif

#else /* !CONFIG_RISCV_HAS_CLIC */

	/*
	 * CLINT vectored mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _irq_vector_table (interrupt vector table). Add 1 to base
	 * address of _irq_vector_table to indicate that vectored mode
	 * is used (LSB = 0x1). CPU will mask the LSB out of
	 * the address so that base address of _irq_vector_table is used.
	 *
	 * NOTE: _irq_vector_table is 256-byte aligned. Incorrect alignment
	 *        of _irq_vector_table breaks this code.
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
	 	M_CLLC	ct0, _irq_vector_table /*Load Linked Capability via Capability isr.S file */

		li t1, 0x01 /* Enable CLIC vectored mode by setting LSB */
		M_CINCOFFSET ct0, ct0, t1

		M_CSPECIALW mtcc, ct0 /*Machine trap code capability*/
		M_CMOVE	ct1, cnull
		M_CSPECIALW mtdc, ct1 /*Machine trap data capability */
	 #else
		la t0, _irq_vector_table /* Load address of interrupt vector table */
		addi t0, t0, 0x01 /* Enable vectored mode by setting LSB */
		csrw mtvec, t0
	#endif
#endif /* CONFIG_RISCV_HAS_CLIC */

#else /* !CONFIG_RISCV_VECTORED_MODE */

	/*
	 * CLINT direct mode
	 *
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
		M_CLLC	ct0, _isr_wrapper /*NOT VECTORED_MODE Load Linked Capability via Capability isr.S file */
		M_CSPECIALW M_MTCC, ct0 /*Machine trap code capability*/
		M_CMOVE	ct1, cnull
		M_CSPECIALW M_MTDC, ct1 /*Machine trap data capability */
	#else
		la t0, _isr_wrapper
		csrw mtvec, t0
	#endif

#endif /* CONFIG_RISCV_VECTORED_MODE */

	/* Jump to __reset */
	#ifdef __CHERI_PURE_CAPABILITY__
		M_CLLC	ct0, __reset /* Load PCC-relative capability */
		M_CJR ct0

	#else
		tail __reset
	#endif

