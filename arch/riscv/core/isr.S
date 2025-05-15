/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec

 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/syscall.h>
#include "asm_macros.inc"

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
#include <soc_isr_stacking.h>
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

/* Convenience macro for loading/storing register states. */
#ifdef __CHERI_PURE_CAPABILITY__
#define DO_CALLER_SAVED(op) \
	RV_E(	op ct0, __z_arch_esf_t_ct0_OFFSET(csp)	);\
	RV_E(	op ct1, __z_arch_esf_t_ct1_OFFSET(csp)	);\
	RV_E(	op ct2, __z_arch_esf_t_ct2_OFFSET(csp)	);\
	RV_I(	op ct3, __z_arch_esf_t_ct3_OFFSET(csp)	);\
	RV_I(	op ct4, __z_arch_esf_t_ct4_OFFSET(csp)	);\
	RV_I(	op ct5, __z_arch_esf_t_ct5_OFFSET(csp)	);\
	RV_I(	op ct6, __z_arch_esf_t_ct6_OFFSET(csp)	);\
	RV_E(	op ca0, __z_arch_esf_t_ca0_OFFSET(csp)	);\
	RV_E(	op ca1, __z_arch_esf_t_ca1_OFFSET(csp)	);\
	RV_E(	op ca2, __z_arch_esf_t_ca2_OFFSET(csp)	);\
	RV_E(	op ca3, __z_arch_esf_t_ca3_OFFSET(csp)	);\
	RV_E(	op ca4, __z_arch_esf_t_ca4_OFFSET(csp)	);\
	RV_E(	op ca5, __z_arch_esf_t_ca5_OFFSET(csp)	);\
	RV_I(	op ca6, __z_arch_esf_t_ca6_OFFSET(csp)	);\
	RV_I(	op ca7, __z_arch_esf_t_ca7_OFFSET(csp)	);\
	RV_E(	op cra, __z_arch_esf_t_cra_OFFSET(csp)	)
#else
#define DO_CALLER_SAVED(op) \
	RV_E(	op t0, __z_arch_esf_t_t0_OFFSET(sp)	);\
	RV_E(	op t1, __z_arch_esf_t_t1_OFFSET(sp)	);\
	RV_E(	op t2, __z_arch_esf_t_t2_OFFSET(sp)	);\
	RV_I(	op t3, __z_arch_esf_t_t3_OFFSET(sp)	);\
	RV_I(	op t4, __z_arch_esf_t_t4_OFFSET(sp)	);\
	RV_I(	op t5, __z_arch_esf_t_t5_OFFSET(sp)	);\
	RV_I(	op t6, __z_arch_esf_t_t6_OFFSET(sp)	);\
	RV_E(	op a0, __z_arch_esf_t_a0_OFFSET(sp)	);\
	RV_E(	op a1, __z_arch_esf_t_a1_OFFSET(sp)	);\
	RV_E(	op a2, __z_arch_esf_t_a2_OFFSET(sp)	);\
	RV_E(	op a3, __z_arch_esf_t_a3_OFFSET(sp)	);\
	RV_E(	op a4, __z_arch_esf_t_a4_OFFSET(sp)	);\
	RV_E(	op a5, __z_arch_esf_t_a5_OFFSET(sp)	);\
	RV_I(	op a6, __z_arch_esf_t_a6_OFFSET(sp)	);\
	RV_I(	op a7, __z_arch_esf_t_a7_OFFSET(sp)	);\
	RV_E(	op ra, __z_arch_esf_t_ra_OFFSET(sp)	)
#endif

	.macro get_current_cpu dst
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CSPECIALR \dst, mscratchc
	#else
	csrr \dst, mscratch
	#endif
#else
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC \dst, _kernel
	M_CINCOFFSETIMM \dst, \dst,  ___kernel_t_cpus_OFFSET
	#else
	la \dst, _kernel + ___kernel_t_cpus_OFFSET
	#endif
#endif
	.endm

/* imports */
GDATA(_sw_isr_table)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(_Fault)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

GTEXT(z_riscv_fatal_error)
GTEXT(z_get_next_switch_handle)
GTEXT(z_riscv_switch)
GTEXT(z_riscv_thread_start)

#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

#ifdef CONFIG_USERSPACE
GDATA(_k_syscall_table)
#endif

/* exports */
GTEXT(_isr_wrapper)
/* use ABI name of registers for the sake of simplicity */

/*
 * Generic architecture-level IRQ handling, along with callouts to
 * SoC-specific routines.
 *
 * Architecture level IRQ handling includes basic context save/restore
 * of standard registers and calling ISRs registered at Zephyr's driver
 * level.
 *
 * Since RISC-V does not completely prescribe IRQ handling behavior,
 * implementations vary (some implementations also deviate from
 * what standard behavior is defined). Hence, the arch level code expects
 * the following functions to be provided at the SOC level:
 *
 *     - __soc_is_irq: decide if we're handling an interrupt or an exception
 *     - __soc_handle_irq: handle SoC-specific details for a pending IRQ
 *       (e.g. clear a pending bit in a SoC-specific register)
 *
 * If CONFIG_RISCV_SOC_CONTEXT_SAVE=y, calls to SoC-level context save/restore
 * routines are also made here. For details, see the Kconfig help text.
 */

/*
 * Handler called upon each exception/interrupt/fault
 */
SECTION_FUNC(exception.entry, _isr_wrapper)

/* Provide requested alignment, which depends e.g. on MTVEC format */
.balign CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT

#ifdef CONFIG_USERSPACE
	/* retrieve address of _current_cpu preserving s0 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ct2, cs0 /* temp save */
	M_CSPECIALR cs0, mscratchc /* read */
	M_CSPECIALW mscratchc, ct2 /* write */
	#else
	csrrw s0, mscratch, s0 /* read and write to CSR */
	#endif

	/* preserve t0 and t1 temporarily */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc ct0, _curr_cpu_arch_user_exc_ctmp0(cs0) /* store cap at cap */
	csc ct1, _curr_cpu_arch_user_exc_ctmp1(cs0) /* store cap at cap */
	#else
	sr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	sr t1, _curr_cpu_arch_user_exc_tmp1(s0)
	#endif

	/* determine if we come from user space */
	csrr t0, mstatus
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f


	/* in user space we were: switch to our privileged stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ct0, csp
	clc csp, _curr_cpu_arch_user_exc_csp(cs0) /* load cap at cap */
	#else
	mv t0, sp
	lr sp, _curr_cpu_arch_user_exc_sp(s0)
	#endif

	/* Save user stack value. Coming from user space, we know this
	 * can't overflow the privileged stack. The esf will be allocated
	 * later but it is safe to store our saved user sp here. */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc ct0, (-__z_arch_esf_t_SIZEOF + __z_arch_esf_t_csp_OFFSET)(csp) /* store cap at cap */
	#else
	sr t0, (-__z_arch_esf_t_SIZEOF + __z_arch_esf_t_sp_OFFSET)(sp)
	#endif

	/* Make sure tls pointer is sane */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	clc ctp, _thread_offset_to_tls(ct0) /* load cap at cap */
	#else
	lr t0, ___cpu_t_current_OFFSET(s0)
	lr tp, _thread_offset_to_tls(t0)
	#endif

	/* Clear our per-thread usermode flag */
	#ifdef __CHERI_PURE_CAPABILITY__
	/* ToDo - build with a userspace app and check this is converted correctly */
	lui t0, %tprel_hi(is_user_mode) /* load upper immediate */
	M_CINCOFFSET ct0, ctp, t0, %tprel_cincoffset(is_user_mode)
	csb zero, %tprel_lo(is_user_mode)(ct0)
	#else
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb zero, %tprel_lo(is_user_mode)(t0)
	#endif
1:
	/* retrieve original t0/t1 values */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, _curr_cpu_arch_user_exc_ctmp0(cs0) /* load cap at cap */
	clc ct1, _curr_cpu_arch_user_exc_ctmp1(cs0) /* load cap at cap */
	#else
	lr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	lr t1, _curr_cpu_arch_user_exc_tmp1(s0)
	#endif

	/* retrieve original s0 and restore _current_cpu in mscratch */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ct2, cs0 /* temp save */
	M_CSPECIALR cs0, mscratchc /* read */
	M_CSPECIALW mscratchc, ct2 /* write */
	#else
	csrrw s0, mscratch, s0 /* read and write to CSR */
	#endif
#endif

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_STACKING
#else

	/* Save caller-saved registers on current thread stack. */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -__z_arch_esf_t_SIZEOF
	/* If we keep returning here after the first DO_CALLER_SAVED, it is likely due to an out of bounds stack exception */
	/* If we read the value of mcause and mtval first we need some register to store the values but don't want to loose the values of those about to be saved*/
	DO_CALLER_SAVED(csc)		; /* store cap reg at cap, use csc*/
	#else
	addi sp, sp, -__z_arch_esf_t_SIZEOF
	DO_CALLER_SAVED(sr)		;
	#endif
#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	/* Save s0 in the esf and load it with &_current_cpu. */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc cs0, __z_arch_esf_t_cs0_OFFSET(csp) /* store cap reg at cap */
	get_current_cpu cs0
	#else
	sr s0, __z_arch_esf_t_s0_OFFSET(sp)
	get_current_cpu s0
	#endif

	/* Save MEPC register */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CSPECIALR ct0, mepcc
	csc ct0, __z_arch_esf_t_mepcc_OFFSET(csp) /* store cap reg at cap */
	#else
	csrr t0, mepc
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	#endif

	/* Save MSTATUS register */
	#ifdef __CHERI_PURE_CAPABILITY__
	csrr t2, mstatus /* mstatus integer reg */
	M_CINCOFFSETIMM ct3, csp, __z_arch_esf_t_mstatus_OFFSET /*0 offset with macro so inc first*/
	csr t2, 0(ct3) /*store integer at cap (macro) */
	#else
	csrr t2, mstatus
	sr t2, __z_arch_esf_t_mstatus_OFFSET(sp)
	#endif

#if defined(CONFIG_FPU_SHARING)
	/* determine if FPU access was disabled */
	li t1, MSTATUS_FS
	and t1, t1, t2
	bnez t1, no_fp
	/* determine if this is an Illegal Instruction exception */
	csrr t2, mcause
	li t1, 2		/* 2 = illegal instruction */
	bne t1, t2, no_fp
	/* determine if we trapped on an FP instruction. */
	csrr t2, mtval		/* get faulting instruction */
#ifdef CONFIG_QEMU_TARGET
	/*
	 * Some implementations may not support MTVAL in this capacity.
	 * Notably QEMU when a CSR instruction is involved.
	 */
	bnez t2, 1f
	#ifdef __CHERI_PURE_CAPABILITY__
	clw t2, 0(ct0)		/* ct0 = mepcc - load integer word */
	#else
	lw t2, 0(t0)		/* t0 = mepc */
	#endif
1:
#endif
	andi t0, t2, 0x7f	/* keep only the opcode bits */
	xori t1, t0, 0b1010011	/* OP-FP */
	beqz t1, is_fp
	ori  t0, t0, 0b0100000
	xori t1, t0, 0b0100111	/* LOAD-FP / STORE-FP */
	beqz t1, is_fp
	/*
	 * The FRCSR, FSCSR, FRRM, FSRM, FSRMI, FRFLAGS, FSFLAGS and FSFLAGSI
	 * are in fact CSR instructions targeting the fcsr, frm and fflags
	 * registers. They should be caught as FPU instructions as well.
	 *
	 * CSR format: csr#[31-20] src[19-15] op[14-12] dst[11-7] SYSTEM[6-0]
	 * SYSTEM = 0b1110011, op = 0b.xx where xx is never 0
	 * The csr# of interest are: 1=fflags, 2=frm, 3=fcsr
	 */
	xori t1, t0, 0b1110011	/* SYSTEM opcode */
	bnez t1, 2f		/* not a CSR insn */
	srli t0, t2, 12
	andi t0, t0, 0x3
	beqz t0, 2f		/* not a CSR insn */
	srli t0, t2, 20		/* isolate the csr register number */
	beqz t0, 2f		/* 0=ustatus */
	andi t0, t0, ~0x3	/* 1=fflags, 2=frm, 3=fcsr */
#if !defined(CONFIG_RISCV_ISA_EXT_C)
	bnez t0, no_fp
#else
	beqz t0, is_fp
2:	/* remaining non RVC (0b11) and RVC with 0b01 are not FP instructions */
	andi t1, t2, 1
	bnez t1, no_fp
	/*
	 * 001...........00 = C.FLD    RV32/64  (RV128 = C.LQ)
	 * 001...........10 = C.FLDSP  RV32/64  (RV128 = C.LQSP)
	 * 011...........00 = C.FLW    RV32     (RV64/128 = C.LD)
	 * 011...........10 = C.FLWSPP RV32     (RV64/128 = C.LDSP)
	 * 101...........00 = C.FSD    RV32/64  (RV128 = C.SQ)
	 * 101...........10 = C.FSDSP  RV32/64  (RV128 = C.SQSP)
	 * 111...........00 = C.FSW    RV32     (RV64/128 = C.SD)
	 * 111...........10 = C.FSWSP  RV32     (RV64/128 = C.SDSP)
	 *
	 * so must be .01............. on RV64 and ..1............. on RV32.
	 */
	srli t0, t2, 8
#if defined(CONFIG_64BIT)
	andi t1, t0, 0b01100000
	xori t1, t1, 0b00100000
	bnez t1, no_fp
#else
	andi t1, t0, 0b00100000
	beqz t1, no_fp
#endif
#endif /* CONFIG_RISCV_ISA_EXT_C */

is_fp:	/* Process the FP trap and quickly return from exception */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC cra, fp_trap_exit
	M_CMOVE ca0, csp
	M_CTAIL z_riscv_fpu_trap
	#else
	la ra, fp_trap_exit
	mv a0, sp
	tail z_riscv_fpu_trap
	#endif
2:
no_fp:	/* increment _current->arch.exception_depth */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	clb t1, _thread_offset_to_exception_depth(ct0) /* load byte at cap */
	add t1, t1, 1
	csb t1, _thread_offset_to_exception_depth(ct0) /* store byte at cap */
	#else
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, 1
	sb t1, _thread_offset_to_exception_depth(t0)
	#endif

	/* configure the FPU for exception mode */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL z_riscv_fpu_enter_exc
	#else
	call z_riscv_fpu_enter_exc
	#endif
#endif /* CONFIG_FPU_SHARING */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca0, csp
	M_CINCOFFSETIMM ca0, csp, __z_arch_esf_t_soc_context_OFFSET
	M_CJAL cra, __soc_save_context
	#else
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_save_context
	#endif
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/*
	 * Check if exception is the result of an interrupt or not.
	 * (SOC dependent). Following the RISC-V architecture spec, the MSB
	 * of the mcause register is used to indicate whether an exception
	 * is the result of an interrupt or an exception/fault. But for some
	 * SOCs (like pulpino or riscv-qemu), the MSB is never set to indicate
	 * interrupt. Hence, check for interrupt/exception via the __soc_is_irq
	 * function (that needs to be implemented by each SOC). The result is
	 * returned via register a0 (1: interrupt, 0 exception)
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CJAL cra, __soc_is_irq
	#else
	jal ra, __soc_is_irq
	#endif

	/* If a0 != 0, jump to is_interrupt */
	bnez a0, is_interrupt

	/*
	 * If the exception is the result of an ECALL, check whether to
	 * perform a context-switch or an IRQ offload. Otherwise call _Fault
	 * to report the exception.
	 */
	csrr t0, mcause
	li t2, SOC_MCAUSE_EXP_MASK
	and t0, t0, t2

	/*
	 * If mcause == SOC_MCAUSE_ECALL_EXP, handle system call from
	 * kernel thread.
	 */
	li t1, SOC_MCAUSE_ECALL_EXP
	beq t0, t1, is_kernel_syscall

#ifdef CONFIG_USERSPACE
	/*
	 * If mcause == SOC_MCAUSE_USER_ECALL_EXP, handle system call
	 * for user mode thread.
	 */
	li t1, SOC_MCAUSE_USER_ECALL_EXP
	beq t0, t1, is_user_syscall
#endif /* CONFIG_USERSPACE */

	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a z_arch_esf_t structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to
	 * no_reschedule to restore stack.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca0, csp
	M_CLLC cra, no_reschedule
	M_CTAIL _Fault
	#else
	mv a0, sp
	la ra, no_reschedule
	tail _Fault
	#endif

is_kernel_syscall:
	/*
	 * A syscall is the result of an ecall instruction, in which case the
	 * MEPC will contain the address of the ecall instruction.
	 * Increment saved MEPC by 4 to prevent triggering the same ecall
	 * again upon exiting the ISR.
	 *
	 * It's safe to always increment by 4, even with compressed
	 * instructions, because the ecall instruction is always 4 bytes.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, __z_arch_esf_t_mepcc_OFFSET(csp) /* load cap at cap */
	M_CINCOFFSETIMM ct0, ct0, 4
	csc ct0, __z_arch_esf_t_mepcc_OFFSET(csp) /* save cap at cap */
	#else
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t0, t0, 4
	sr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	#endif

#ifdef CONFIG_PMP_STACK_GUARD
	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
#endif

	/* Determine what to do. Operation code is in t0. */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, __z_arch_esf_t_ct0_OFFSET(csp)
	#else
	lr t0, __z_arch_esf_t_t0_OFFSET(sp)
	#endif

	.if RV_ECALL_RUNTIME_EXCEPT != 0; .err; .endif
	beqz t0, do_fault

#if defined(CONFIG_IRQ_OFFLOAD)
	li t1, RV_ECALL_IRQ_OFFLOAD
	beq t0, t1, do_irq_offload
#endif

#ifdef CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL
	li t1, RV_ECALL_SCHEDULE
	bne t0, t1, skip_schedule
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, __z_arch_esf_t_ca0_OFFSET(csp) /* load cap at cap */
	clc ca1, __z_arch_esf_t_ca1_OFFSET(csp) /* load cap at cap */
	j reschedule
	#else
	lr a0, __z_arch_esf_t_a0_OFFSET(sp)
	lr a1, __z_arch_esf_t_a1_OFFSET(sp)
	j reschedule
	#endif
skip_schedule:
#endif

	/* default fault code is K_ERR_KERNEL_OOPS */
	li a0, 3
	j 1f

do_fault:
	/* Handle RV_ECALL_RUNTIME_EXCEPT. Retrieve reason in a0, esf in A1. */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, __z_arch_esf_t_ca0_OFFSET(csp) /* load cap at cap */
1:	M_CMOVE ca1, csp
	M_CTAIL z_riscv_fatal_error
	#else
	lr a0, __z_arch_esf_t_a0_OFFSET(sp)
1:	mv a1, sp
	tail z_riscv_fatal_error
	#endif

#if defined(CONFIG_IRQ_OFFLOAD)
do_irq_offload:
	/*
	 * Retrieve provided routine and argument from the stack.
	 * Routine pointer is in saved a0, argument in saved a1
	 * so we load them with a1/a0 (reversed).
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca1, __z_arch_esf_t_ca0_OFFSET(csp) /* load cap at cap */
	clc ca0, __z_arch_esf_t_ca1_OFFSET(csp) /* load cap at cap */
	#else
	lr a1, __z_arch_esf_t_a0_OFFSET(sp)
	lr a0, __z_arch_esf_t_a1_OFFSET(sp)
	#endif

	/* Increment _current_cpu->nested */
	#ifdef __CHERI_PURE_CAPABILITY__
	clw t1, ___cpu_t_nested_OFFSET(cs0) /* load word at cap */
	addi t2, t1, 1
	csw t2, ___cpu_t_nested_OFFSET(cs0) /* store word at cap */
	bnez t1, 1f
	#else
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, 1f
	#endif

	/* Switch to interrupt stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ct0, csp
	clc csp, ___cpu_t_irq_stack_OFFSET(cs0) /* load cap at cap */
	#else
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)
	#endif

	/* Save thread stack pointer on interrupt stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -16
	csc ct0, 0(csp) /* save cap at cap */
	#else
	addi sp, sp, -16
	sr t0, 0(sp)
	#endif
1:
	/* Execute provided routine (argument is in a0 already). */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CJALR cra, ca1, 0
	#else
	jalr ra, a1, 0
	#endif

	/* Leave through the regular IRQ exit path */
	j irq_done
#endif /* CONFIG_IRQ_OFFLOAD */

#ifdef CONFIG_USERSPACE
is_user_syscall:

#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * We came from userspace and need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	M_CCALL z_riscv_pmp_stackguard_enable
	#else
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
	#endif
#endif

	/* It is safe to re-enable IRQs now */
	csrs mstatus, MSTATUS_IEN

	/*
	 * Same as for is_kernel_syscall: increment saved MEPC by 4 to
	 * prevent triggering the same ecall again upon exiting the ISR.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct1, __z_arch_esf_t_mepcc_OFFSET(csp) /* load cap at cap */
	M_CINCOFFSETIMM ct1, ct1, 4
	csc ct1, __z_arch_esf_t_mepcc_OFFSET(csp) /* save cap at cap */
	#else
	lr t1, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t1, t1, 4
	sr t1, __z_arch_esf_t_mepc_OFFSET(sp)
	#endif

	/* Restore argument registers from user stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, __z_arch_esf_t_ca0_OFFSET(csp) /* load cap at cap */
	clc ca1, __z_arch_esf_t_ca1_OFFSET(csp)
	clc ca2, __z_arch_esf_t_ca2_OFFSET(csp)
	clc ca3, __z_arch_esf_t_ca3_OFFSET(csp)
	clc ca4, __z_arch_esf_t_ca4_OFFSET(csp)
	clc ca5, __z_arch_esf_t_ca5_OFFSET(csp)
	clc ct0, __z_arch_esf_t_ct0_OFFSET(csp)
	#else
	lr a0, __z_arch_esf_t_a0_OFFSET(sp)
	lr a1, __z_arch_esf_t_a1_OFFSET(sp)
	lr a2, __z_arch_esf_t_a2_OFFSET(sp)
	lr a3, __z_arch_esf_t_a3_OFFSET(sp)
	lr a4, __z_arch_esf_t_a4_OFFSET(sp)
	lr a5, __z_arch_esf_t_a5_OFFSET(sp)
	lr t0, __z_arch_esf_t_t0_OFFSET(sp)
	#endif
#if defined(CONFIG_RISCV_ISA_RV32E)
	/* Stack alignment for RV32E is 4 bytes */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -8 /* twice as big for CHERI cap */
	M_CMOVE ct1, csp
	csc ct1, 0(csp)
	#else
	addi sp, sp, -4
	mv t1, sp
	sw t1, 0(sp)
	#endif
#else
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca6, csp
	#else
	mv a6, sp
	#endif
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* validate syscall limit */
	li t1, K_SYSCALL_LIMIT
	bltu t0, t1, valid_syscall_id

	/* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca0, ct0
	li t0, K_SYSCALL_BAD
	#else
	mv a0, t0
	li t0, K_SYSCALL_BAD
	#endif

valid_syscall_id:
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC ct2, _k_syscall_table

	slli t1, t0, RV_REGSHIFT	# Determine offset from indice value
	M_CINCOFFSET ct2, ct2, t1		# Table addr + offset = function addr
	clc ct2, 0(ct2)			# Load function address
	#else
	la t2, _k_syscall_table

	slli t1, t0, RV_REGSHIFT	# Determine offset from indice value
	add t2, t2, t1			# Table addr + offset = function addr
	lr t2, 0(t2)			# Load function address
	#endif

	/* Execute syscall function */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CJALR cra, ct2, 0
	#else
	jalr ra, t2, 0
	#endif

#if defined(CONFIG_RISCV_ISA_RV32E)
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, 8 /* twice as big for CHERI cap */
	#else
	addi sp, sp, 4
	#endif
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* Update a0 (return value) on the stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc ca0, __z_arch_esf_t_ca0_OFFSET(csp) /* save cap at cap */
	#else
	sr a0, __z_arch_esf_t_a0_OFFSET(sp)
	#endif

	/* Disable IRQs again before leaving */
	csrc mstatus, MSTATUS_IEN
	j might_have_rescheduled
#endif /* CONFIG_USERSPACE */

is_interrupt:

#ifdef CONFIG_PMP_STACK_GUARD
#ifdef CONFIG_USERSPACE
	/*
	 * If we came from userspace then we need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM ct3, csp, __z_arch_esf_t_mstatus_OFFSET /*0 offset with macro */
        clr t0, 0(ct3) /*load integer reg at cap (macro) */
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	clc ca0, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	M_CCALL z_riscv_pmp_stackguard_enable
	#else
	lr t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
	#endif
	j 2f
#endif /* CONFIG_USERSPACE */
1:	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
2:
#endif

	/* Increment _current_cpu->nested */
	#ifdef __CHERI_PURE_CAPABILITY__
	clw t1, ___cpu_t_nested_OFFSET(cs0) /* load word at cap */
	addi t2, t1, 1
	csw t2, ___cpu_t_nested_OFFSET(cs0) /* store word at cap */
	#else
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	#endif
	bnez t1, on_irq_stack


	/* Switch to interrupt stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ct0, csp
	clc csp, ___cpu_t_irq_stack_OFFSET(cs0) /* load cap at cap */
	#else
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)
	#endif
	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -16
	csc ct0, 0(csp) /* save cap at cap */
	#else
	addi sp, sp, -16
	sr t0, 0(sp)
	#endif

on_irq_stack:

#ifdef CONFIG_TRACING_ISR
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL sys_trace_isr_enter
	#else
	call sys_trace_isr_enter
	#endif
#endif

	/* Get IRQ causing interrupt */
	csrr a0, mcause
	li t0, SOC_MCAUSE_EXP_MASK
	and a0, a0, t0

	/*
	 * Clear pending IRQ generating the interrupt at SOC level
	 * Pass IRQ number to __soc_handle_irq via register a0
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CJAL cra, __soc_handle_irq
	#else
	jal ra, __soc_handle_irq
	#endif

	/*
	 * Call corresponding registered function in _sw_isr_table.
	 * (table is 2-word wide, we should shift index accordingly)
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC ct0, _sw_isr_table
	/* shift more as we are now using caps */
	slli a0, a0, (RV_REGSHIFT + 2)
	M_CINCOFFSET ct0, ct0, a0
	#else
	la t0, _sw_isr_table
	slli a0, a0, (RV_REGSHIFT + 1)
	add t0, t0, a0
	#endif

	/* Load argument in a0 register */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, 0(ct0) /* load cap at cap */
	#else
	lr a0, 0(t0)
	#endif

	/* Load ISR function address in register t1 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct1, RV_REGSIZE*2(ct0) /* load cap at cap - register is twice the size*/
	#else
	lr t1, RV_REGSIZE(t0)
	#endif

	/* Call ISR function */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CJALR cra, ct1, 0
	#else
	jalr ra, t1, 0
	#endif

#ifdef CONFIG_TRACING_ISR
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL sys_trace_isr_exit
	#else
	call sys_trace_isr_exit
	#endif
#endif

irq_done:
	/* Decrement _current_cpu->nested */
	#ifdef __CHERI_PURE_CAPABILITY__
	clw t2, ___cpu_t_nested_OFFSET(cs0) /* load word at cap */
	addi t2, t2, -1
	csw t2, ___cpu_t_nested_OFFSET(cs0) /* store word at cap */
	#else
	lw t2, ___cpu_t_nested_OFFSET(s0)
	addi t2, t2, -1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	#endif
	bnez t2, no_reschedule

	/* nested count is back to 0: Return to thread stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc csp, 0(csp) /* load cap at cap */
	#else
	lr sp, 0(sp)
	#endif

#ifdef CONFIG_STACK_SENTINEL
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL z_check_stack_sentinel
	#else
	call z_check_stack_sentinel
	#endif
#endif

check_reschedule:

#ifdef CONFIG_MULTITHREADING

	/* Get pointer to current thread on this CPU */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca1, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	#else
	lr a1, ___cpu_t_current_OFFSET(s0)
	#endif

	/*
	 * Get next thread to schedule with z_get_next_switch_handle().
	 * We pass it a NULL as we didn't save the whole thread context yet.
	 * If no scheduling is necessary then NULL will be returned.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, -16
	csc ca1, 0(csp) /* store cap at cap */
	M_CMOVE ca0, cnull
	M_CCALL z_get_next_switch_handle
	clc ca1, 0(csp) /* load cap at cap */
	M_CINCOFFSETIMM csp, csp, 16
	#else
	addi sp, sp, -16
	sr a1, 0(sp)
	mv a0, zero
	call z_get_next_switch_handle
	lr a1, 0(sp)
	addi sp, sp, 16
	#endif
	beqz a0, no_reschedule

reschedule:

	/*
	 * Perform context switch:
	 * a0 = new thread
	 * a1 = old thread
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL z_riscv_switch
	#else
	call z_riscv_switch
	#endif

z_riscv_thread_start:
might_have_rescheduled:
	/* reload s0 with &_current_cpu as it might have changed or be unset */
	#ifdef __CHERI_PURE_CAPABILITY__
	get_current_cpu cs0
	#else
	get_current_cpu s0
	#endif

#endif /* CONFIG_MULTITHREADING */

no_reschedule:

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca0, csp
	M_CINCOFFSETIMM ca0, csp, __z_arch_esf_t_soc_context_OFFSET
	M_CJAL cra, __soc_restore_context
	#else
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
	#endif
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#if defined(CONFIG_FPU_SHARING)
	/* FPU handling upon exception mode exit */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CMOVE ca0, csp
	M_CCALL z_riscv_fpu_exit_exc
	#else
	mv a0, sp
	call z_riscv_fpu_exit_exc
	#endif

	/* decrement _current->arch.exception_depth */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, ___cpu_t_current_OFFSET(cs0) /* load cap at cap */
	clb t1, _thread_offset_to_exception_depth(ct0) /* load byte at cap */
	add t1, t1, -1
	csb t1, _thread_offset_to_exception_depth(ct0) /* store byte at cap */
	#else
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, -1
	sb t1, _thread_offset_to_exception_depth(t0)
	#endif
fp_trap_exit:
#endif

	/* Restore MEPC and MSTATUS registers */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ct0, __z_arch_esf_t_mepcc_OFFSET(csp) /* load cap at cap */
	M_CINCOFFSETIMM ct2,csp, __z_arch_esf_t_mstatus_OFFSET /*0 offset with macro */
	clr t2, 0(ct2) /*load integer at cap (macro) */
	M_CSPECIALW mepcc, ct0
	#else
	lr t0, __z_arch_esf_t_mepc_OFFSET(sp)
	lr t2, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw mepc, t0
	#endif
	csrw mstatus, t2

#ifdef CONFIG_USERSPACE
	/*
	 * Check if we are returning to user mode. If so then we must
	 * set is_user_mode to true and preserve our kernel mode stack for
	 * the next exception to come.
	 */
	li t1, MSTATUS_MPP
	and t0, t2, t1
	bnez t0, 1f

#ifdef CONFIG_PMP_STACK_GUARD
	/* Remove kernel stack guard and Reconfigure PMP for user mode */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc ca0, ___cpu_t_current_OFFSET(cs0)
	M_CCALL z_riscv_pmp_usermode_enable
	#else
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_usermode_enable
	#endif
#endif

	/* Set our per-thread usermode flag */
	li t1, 1
	lui t0, %tprel_hi(is_user_mode) /* load upper immediate */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSET ct0, ctp, t0, %tprel_cincoffset(is_user_mode)
	csb t1, %tprel_lo(is_user_mode)(ct0)	/* cap store byte */
	#else
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb t1, %tprel_lo(is_user_mode)(t0)
	#endif

	/* preserve stack pointer for next exception entry */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM ct0, csp, __z_arch_esf_t_SIZEOF
	csc ct0, _curr_cpu_arch_user_exc_csp(cs0) /* store cap at cap */
	#else
	add t0, sp, __z_arch_esf_t_SIZEOF
	sr t0, _curr_cpu_arch_user_exc_sp(s0)
	#endif

	j 2f
1:
	/*
	 * We are returning to kernel mode. Store the stack pointer to
	 * be re-loaded further down.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM ct0, csp, __z_arch_esf_t_SIZEOF
	csc ct0, __z_arch_esf_t_csp_OFFSET(csp) /* store cap at cap */
	#else
	addi t0, sp, __z_arch_esf_t_SIZEOF
	sr t0, __z_arch_esf_t_sp_OFFSET(sp)
	#endif
2:
#endif

	/* Restore s0 (it is no longer ours) */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc cs0, __z_arch_esf_t_cs0_OFFSET(csp) /* load cap at cap */
	#else
	lr s0, __z_arch_esf_t_s0_OFFSET(sp)
	#endif

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_UNSTACKING
#else
	/* Restore caller-saved registers from thread stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	DO_CALLER_SAVED(clc)
	#else
	DO_CALLER_SAVED(lr)
	#endif

#ifdef CONFIG_USERSPACE
	/* retrieve saved stack pointer */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc csp, __z_arch_esf_t_csp_OFFSET(csp) /* load cap at cap */
	#else
	lr sp, __z_arch_esf_t_sp_OFFSET(sp)
	#endif
#else
	/* remove esf from the stack */
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSETIMM csp, csp, __z_arch_esf_t_SIZEOF
	#else
	addi sp, sp, __z_arch_esf_t_SIZEOF
	#endif
#endif

#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	mret

