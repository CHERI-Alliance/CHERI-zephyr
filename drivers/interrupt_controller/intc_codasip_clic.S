/*
 * Copyright (c) 2024 Baumer Electric AG
 * Copyright (c) 2026 Modified for Codasip CLIC & CHERI by University of Birmingham
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Assembler-hooks for the Codasip Interrupt Controller for 32-bit riscv
 */

#include <zephyr/arch/cpu.h>
#include "intc_codasip_clic.h" /* includes RISCV non-CHERI and CHERI macros */

GTEXT(__soc_handle_irq)
/*
 * In an ECLIC, pending interrupts don't have to be cleared by hand.
 * In vectored mode, interrupts are cleared automatically.
 * In non-vectored mode, interrupts are cleared when writing the mnxti register (done in
 * __soc_handle_all_irqs).
 * Thus this function can directly return.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret

#if !defined(CONFIG_RISCV_VECTORED_MODE)

GTEXT(__soc_handle_all_irqs)

#ifdef CONFIG_TRACING
/* imports */
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

/*
 * This function services and clears all pending interrupts in non-vectored mode.
 */
SECTION_FUNC(exception.other, __soc_handle_all_irqs)
#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -32 /* twice as big for CHERI cap */
	M_CSC cra, 0(csp) /* save cap at cap */
#else
	addi sp, sp, -16
	sr ra, 0(sp) /* macro */
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Read and clear mnxti to get highest current interrupt and enable interrupts. Will return
	 * original interrupt if no others appear. */
	csrrci a0, CSR_MNXTI, MSTATUS_IEN
	beqz a0, irq_done_clic /* Check if original interrupt vanished. */

irq_loop:

#ifdef CONFIG_TRACING_ISR
#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL sys_trace_isr_enter
#else
	call sys_trace_isr_enter
#endif /* __CHERI_PURE_CAPABILITY__ */
#endif

	/* Call corresponding registered function in _sw_isr_table. a0 is offset in words, table is
	 * 2-word wide -> shift by one */
#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC ct0, _sw_isr_table
	/* shift more as we are now using caps */
	slli a0, a0, (2)
	M_CINCOFFSET ct0, ct0, a0
#else
	la t0, _sw_isr_table
	slli a0, a0, (1)
	add t0, t0, a0
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Load argument in a0 register */
#ifdef __CHERI_PURE_CAPABILITY__
	M_CLC ca0, 0(ct0) /* load cap at cap */
#else
	lr a0, 0(t0)
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Load ISR function address in register t1 */
#ifdef __CHERI_PURE_CAPABILITY__
	M_CLC ct1, RV_REGSIZE*2(ct0) /* load cap at cap - register is twice the size*/
#else
	lr t1, RV_REGSIZE(t0)
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Call ISR function */
#ifdef __CHERI_PURE_CAPABILITY__
	M_CJALR cra, ct1, 0
#else
	jalr ra, t1, 0
#endif /* __CHERI_PURE_CAPABILITY__ */

	/* Read and clear mnxti to get highest current interrupt and enable interrupts. */
	csrrci a0, CSR_MNXTI, MSTATUS_IEN

	/* if get max number returned assume no pending interrupt */
	/* CLIC implementation for Codasip qemu */
maxcheck:
	li t1, 0x3FFC
	beq a0, t1, irq_done_clic

#ifdef CONFIG_TRACING_ISR
#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL sys_trace_isr_exit
#else
	call sys_trace_isr_exit
#endif /* __CHERI_PURE_CAPABILITY__ */
#endif

	bnez a0, irq_loop

irq_done_clic:
#ifdef __CHERI_PURE_CAPABILITY__
	M_CLC cra, 0(csp) /* load cap at cap */
	M_CINCOFFSETIMM csp, csp, 32 /* twice as big for CHERI cap */
	M_CRET

#else
	lr ra, 0(sp) /* macro */
	addi sp, sp, 16
	ret
#endif /* __CHERI_PURE_CAPABILITY__ */
#endif
