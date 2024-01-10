/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

/**
 * @file
 * @brief RISCV public exception handling
 *
 * RISCV-specific kernel exception handling interface.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
#include <soc_context.h>
#endif

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
#include <soc_isr_stacking.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The name of the structure which contains soc-specific state, if
 * any, as well as the soc_esf_t typedef below, are part of the RISC-V
 * arch API.
 *
 * The contents of the struct are provided by a SOC-specific
 * definition in soc_context.h.
 */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
struct soc_esf {
	SOC_ESF_MEMBERS;
};
#endif

#if defined(CONFIG_RISCV_SOC_HAS_ISR_STACKING)
	SOC_ISR_STACKING_ESF_DECLARE;
#else
struct __esf {
	/*
	 * The capability registers are always the size of a pointer which is bigger than an
	 * unsigned long for CHERI.
	 */
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t cra;		/* return address */

	uintptr_t ct0;		/* Caller-saved temporary register */
	uintptr_t ct1;		/* Caller-saved temporary register */
	uintptr_t ct2;		/* Caller-saved temporary register */
	#else
	unsigned long ra;		/* return address */

	unsigned long t0;		/* Caller-saved temporary register */
	unsigned long t1;		/* Caller-saved temporary register */
	unsigned long t2;		/* Caller-saved temporary register */
	#endif
#if !defined(CONFIG_RISCV_ISA_RV32E)
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t ct3;		/* Caller-saved temporary register */
	uintptr_t ct4;		/* Caller-saved temporary register */
	uintptr_t ct5;		/* Caller-saved temporary register */
	uintptr_t ct6;		/* Caller-saved temporary register */
	#else
	unsigned long t3;		/* Caller-saved temporary register */
	unsigned long t4;		/* Caller-saved temporary register */
	unsigned long t5;		/* Caller-saved temporary register */
	unsigned long t6;		/* Caller-saved temporary register */
	#endif
#endif /* !CONFIG_RISCV_ISA_RV32E */
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t ca0;		/* function argument/return value */
	uintptr_t ca1;		/* function argument */
	uintptr_t ca2;		/* function argument */
	uintptr_t ca3;		/* function argument */
	uintptr_t ca4;		/* function argument */
	uintptr_t ca5;		/* function argument */
	#else
	unsigned long a0;		/* function argument/return value */
	unsigned long a1;		/* function argument */
	unsigned long a2;		/* function argument */
	unsigned long a3;		/* function argument */
	unsigned long a4;		/* function argument */
	unsigned long a5;		/* function argument */
	#endif
#if !defined(CONFIG_RISCV_ISA_RV32E)
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t ca6;		/* function argument */
	uintptr_t ca7;		/* function argument */
	#else
	unsigned long a6;		/* function argument */
	unsigned long a7;		/* function argument */
	#endif
#endif /* !CONFIG_RISCV_ISA_RV32E */
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t mepcc;		/* machine exception program counter */
	#else
	unsigned long mepc;		/* machine exception program counter */
	#endif

	unsigned long mstatus;	/* machine status register */

	#ifdef __CHERI_PURE_CAPABILITY__
	/* callee-saved cs0  - automatically creates an offset cap aligned after mstatus*/
	uintptr_t cs0;
	#else
	unsigned long s0;		/* callee-saved s0 */
	#endif

#ifdef CONFIG_USERSPACE
	#ifdef __CHERI_PURE_CAPABILITY__
	uintptr_t csp;		/* preserved (user or kernel) stack pointer */
	#else
	unsigned long sp;		/* preserved (user or kernel) stack pointer */
	#endif
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	struct soc_esf soc_context;
#endif
} __aligned(16);
#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

typedef struct __esf z_arch_esf_t;
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
typedef struct soc_esf soc_esf_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_EXP_H_ */
