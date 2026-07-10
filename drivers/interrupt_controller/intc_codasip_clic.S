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
#include "intc_clic.h"

#ifdef CONFIG_64BIT
	/* register-wide load/store based on ld/sd (XLEN = 64) */

	.macro lr, rd, mem
	ld \rd, \mem
	.endm

	.macro sr, rs, mem
	sd \rs, \mem
	.endm

#else
	/* register-wide load/store based on lw/sw (XLEN = 32) */

	.macro lr, rd, mem
	lw \rd, \mem
	.endm

	.macro sr, rs, mem
	sw \rs, \mem
	.endm

#endif

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
	addi sp, sp, -16
	sr ra, 0(sp)

	/* Read and clear mnxti to get highest current interrupt and enable interrupts. Will return
	 * original interrupt if no others appear. */
	csrrci a0, CSR_MNXTI, MSTATUS_IEN
	beqz a0, irq_done_clic /* Check if original interrupt vanished. */

irq_loop:

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Call corresponding registered function in _sw_isr_table. a0 is offset in words, table is
	 * 2-word wide -> shift by one */
	la t0, _sw_isr_table
	slli a0, a0, (1)
	add t0, t0, a0

	/* Load argument in a0 register */
	lr a0, 0(t0)

	/* Load ISR function address in register t1 */
	lr t1, RV_REGSIZE(t0)

	/* Call ISR function */
	jalr ra, t1, 0

	/* Read and clear mnxti to get highest current interrupt and enable interrupts. */
	csrrci a0, CSR_MNXTI, MSTATUS_IEN

	/* if get max number returned assume no pending interrupt */
	/* CLIC implementation for Codasip qemu */
maxcheck:
	li t1, 0x3FFC
	beq a0, t1, irq_done_clic

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_exit
#endif

	bnez a0, irq_loop

irq_done_clic:
	lr ra, 0(sp)
	addi sp, sp, 16
	ret
#endif
