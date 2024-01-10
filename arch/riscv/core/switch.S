/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include "asm_macros.inc"


/* Convenience macros for loading/storing register states. */

#ifdef __CHERI_PURE_CAPABILITY__
  #define DO_CALLEE_SAVED(op, reg) \
	RV_E(	op cra, _thread_offset_to_cra(reg)	);\
	RV_E(	op cs0, _thread_offset_to_cs0(reg)	);\
	RV_E(	op cs1, _thread_offset_to_cs1(reg)	);\
	RV_I(	op cs2, _thread_offset_to_cs2(reg)	);\
	RV_I(	op cs3, _thread_offset_to_cs3(reg)	);\
	RV_I(	op cs4, _thread_offset_to_cs4(reg)	);\
	RV_I(	op cs5, _thread_offset_to_cs5(reg)	);\
	RV_I(	op cs6, _thread_offset_to_cs6(reg)	);\
	RV_I(	op cs7, _thread_offset_to_cs7(reg)	);\
	RV_I(	op cs8, _thread_offset_to_cs8(reg)	);\
	RV_I(	op cs9, _thread_offset_to_cs9(reg)	);\
	RV_I(	op cs10, _thread_offset_to_cs10(reg)	);\
	RV_I(	op cs11, _thread_offset_to_cs11(reg)	)
#else
  #define DO_CALLEE_SAVED(op, reg) \
	RV_E(	op ra, _thread_offset_to_ra(reg)	);\
	RV_E(	op s0, _thread_offset_to_s0(reg)	);\
	RV_E(	op s1, _thread_offset_to_s1(reg)	);\
	RV_I(	op s2, _thread_offset_to_s2(reg)	);\
	RV_I(	op s3, _thread_offset_to_s3(reg)	);\
	RV_I(	op s4, _thread_offset_to_s4(reg)	);\
	RV_I(	op s5, _thread_offset_to_s5(reg)	);\
	RV_I(	op s6, _thread_offset_to_s6(reg)	);\
	RV_I(	op s7, _thread_offset_to_s7(reg)	);\
	RV_I(	op s8, _thread_offset_to_s8(reg)	);\
	RV_I(	op s9, _thread_offset_to_s9(reg)	);\
	RV_I(	op s10, _thread_offset_to_s10(reg)	);\
	RV_I(	op s11, _thread_offset_to_s11(reg)	)
#endif

GTEXT(z_riscv_switch)
GTEXT(z_thread_mark_switched_in)
GTEXT(z_riscv_configure_stack_guard)
GTEXT(z_riscv_fpu_thread_context_switch)

/* void z_riscv_switch(k_thread_t *switch_to, k_thread_t *switch_from) */
SECTION_FUNC(TEXT, z_riscv_switch)

	/* Save the old thread's callee-saved registers */
	#ifdef __CHERI_PURE_CAPABILITY__
	DO_CALLEE_SAVED(csc, ca1) /* store cap reg at cap, use csc*/
	#else
	DO_CALLEE_SAVED(sr, a1)
	#endif

	/* Save the old thread's stack pointer */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc csp, _thread_offset_to_csp(ca1)
	#else
	sr sp, _thread_offset_to_sp(a1)
	#endif

	/* Set thread->switch_handle = thread to mark completion */
	#ifdef __CHERI_PURE_CAPABILITY__
	csc ca1, ___thread_t_switch_handle_OFFSET(ca1)
	#else
	sr a1, ___thread_t_switch_handle_OFFSET(a1)
	#endif

	/* Get the new thread's stack pointer */
	#ifdef __CHERI_PURE_CAPABILITY__
	clc csp, _thread_offset_to_csp(ca0) /* load cap reg from cap, use clc*/
	#else
	lr sp, _thread_offset_to_sp(a0)
	#endif

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* Get the new thread's tls pointer */
	#ifdef __CHERI_PURE_CAPABILITY__
	/*
	* Offset name doesn't need changing in zephyr/kernel/include/offsets_short.h
	* because tls is defined as uintptr_t type in zephyr/kernel/thread.h so the
	* offset size will adjust correctly for CHERI.
	*/
	clc ctp, _thread_offset_to_tls(ca0)
	#else
	lr tp, _thread_offset_to_tls(a0)
	#endif
#endif

#if defined(CONFIG_FPU_SHARING)
	 /* Preserve a0 across following call. s0 is not yet restored. */
	#ifdef __CHERI_PURE_CAPABILITY__
	cmove cs0, ca0
	ccall z_riscv_fpu_thread_context_switch
	cmove ca0, cs0
	#else
	mv s0, a0
	call z_riscv_fpu_thread_context_switch
	mv a0, s0
	#endif
#endif

#if defined(CONFIG_PMP_STACK_GUARD)
	/* Stack guard has priority over user space for PMP usage. */
	#ifdef __CHERI_PURE_CAPABILITY__
	cmove cs0, ca0
	ccall z_riscv_pmp_stackguard_enable
	cmove ca0, cs0
	#else
	mv s0, a0
	call z_riscv_pmp_stackguard_enable
	mv a0, s0
	#endif
#elif defined(CONFIG_USERSPACE)
	/*
	 * When stackguard is not enabled, we need to configure the PMP only
	 * at context switch time as the PMP is not in effect while inm-mode.
	 * (it is done on every exception return otherwise).
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	clb t0, _thread_offset_to_user_options(ca0) /* load byte user options from cap*/
	andi t0, t0, K_USER
	beqz t0, not_user_task
	cmove cs0, ca0
	ccall z_riscv_pmp_usermode_enable
	cmove ca0, cs0
	#else
	lb t0, _thread_offset_to_user_options(a0)
	andi t0, t0, K_USER
	beqz t0, not_user_task
	mv s0, a0
	call z_riscv_pmp_usermode_enable
	mv a0, s0
	#endif
not_user_task:
#endif

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	#ifdef __CHERI_PURE_CAPABILITY__
	cmove cs0, ca0
	ccall z_thread_mark_switched_in
	cmove ca0, cs0
	#else
	mv s0, a0
	call z_thread_mark_switched_in
	mv a0, s0
	#endif
#endif


	/* Restore the new thread's callee-saved registers */
	#ifdef __CHERI_PURE_CAPABILITY__
	DO_CALLEE_SAVED(clc, ca0)
	#else
	DO_CALLEE_SAVED(lr, a0)
	#endif

	/* Return to arch_switch() or _irq_wrapper() */

	#ifdef __CHERI_PURE_CAPABILITY__
	cret
	#else
	ret
	#endif

