/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

 #include <zephyr/toolchain.h>

 #ifdef __CHERI_PURE_CAPABILITY__
/* CHERI ISA RISCV PERMISSIONS */
#define	CHERI_GLOBAL				(1 << 0)	/* 0x00000001 */
#define	CHERI_PERMIT_EXECUTE			(1 << 1)	/* 0x00000002 */
#define	CHERI_PERMIT_LOAD			(1 << 2)	/* 0x00000004 */
#define	CHERI_PERMIT_STORE			(1 << 3)	/* 0x00000008 */
#define	CHERI_PERMIT_LOAD_CAPABILITY		(1 << 4)	/* 0x00000010 */
#define	CHERI_PERMIT_STORE_CAPABILITY		(1 << 5)	/* 0x00000020 */
#define	CHERI_PERMIT_STORE_LOCAL_CAPABILITY	(1 << 6)	/* 0x00000040 */
#define	CHERI_PERMIT_SEAL			(1 << 7)	/* 0x00000080 */
#define	CHERI_PERMIT_CINVOKE			(1 << 8)	/* 0x00000100 */
#define	CHERI_PERMIT_UNSEAL			(1 << 9)	/* 0x00000200 */
#define	CHERI_PERMIT_ACCESS_SYSTEM_REGISTERS	(1 << 10)	/* 0x00000400 */
#define	CHERI_PERMIT_SET_CID			(1 << 11)	/* 0x00000800 */
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
	cmove	cgp, cnull
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

/* put into purecap mode */
#ifdef __CHERI_PURE_CAPABILITY__
	.option push
	.option nocapmode

	/*
	 * May enter in non-capmode even though
	 * ELF header requests capmode.
	 * These instructions work in both modes.
	 */
	lla	t0, 1f
	cspecialr ct1, pcc
	csetaddr ct1, ct1, t0
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
#endif

boundpcc:
/* Bound PCC */
#ifdef __CHERI_PURE_CAPABILITY__
	/* Create a reduced PCC. */
	/* this needs doing before set trap vector because cllc uses the bounds of pcc */
	cllc	ct0, __start /* __start start of function/base kernel - set base addr to __start*/
	cllc	ct1, _end /* _end in linker */
	sub	t2, t1, t0 /* get length */
	csetaddr ct0, ct0, t0 /*set base address */
	csetbounds ct0, ct0, t2 /* set upper bound */
	/* set permissions */
	li	t2, CHERI_GLOBAL | \
		CHERI_PERMIT_LOAD | \
		CHERI_PERMIT_LOAD_CAPABILITY | \
		CHERI_PERMIT_STORE | \
		CHERI_PERMIT_STORE_CAPABILITY | \
		CHERI_PERMIT_STORE_LOCAL_CAPABILITY | \
 		CHERI_PERMIT_EXECUTE | \
 		CHERI_PERMIT_ACCESS_SYSTEM_REGISTERS
	candperm ct0, ct0, t2
	cllc	ct1, 1f /* where to go now */
	csetaddr ct0, ct0, t1 /* set new starting address for PCC */
	cjr	ct0 /* jump to new PCC with reduced bounds */
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
		cllc	ct0, _isr_wrapper /*Load Linked Capability via Capability isr.S file */

		li t1, 0x03 /* Enable CLIC vectored mode by setting LSB */
		cincoffset ct0, ct0, t1

		cspecialw mtcc, ct0 /*Machine trap code capability*/
		cmove	ct1, cnull
		cspecialw mtdc, ct1 /*Machine trap data capability */
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
	 	cllc	ct0, _irq_vector_table /* Load Linked Capability via Capability */

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
	 	cllc	ct0, _irq_vector_table /*Load Linked Capability via Capability isr.S file */

		li t1, 0x01 /* Enable CLIC vectored mode by setting LSB */
		cincoffset ct0, ct0, t1

		cspecialw mtcc, ct0 /*Machine trap code capability*/
		cmove	ct1, cnull
		cspecialw mtdc, ct1 /*Machine trap data capability */
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
		cllc	ct0, _isr_wrapper /*NOT VECTORED_MODE Load Linked Capability via Capability isr.S file */
		cspecialw mtcc, ct0 /*Machine trap code capability*/
		cmove	ct1, cnull
		cspecialw mtdc, ct1 /*Machine trap data capability */
	#else
		la t0, _isr_wrapper
		csrw mtvec, t0
	#endif

#endif /* CONFIG_RISCV_VECTORED_MODE */

	/* Jump to __reset */
	#ifdef __CHERI_PURE_CAPABILITY__
		cllc	ct0, __reset /* Load PCC-relative capability */
		cjr ct0

	#else
		tail __reset
	#endif




