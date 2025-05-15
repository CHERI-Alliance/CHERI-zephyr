/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <inttypes.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/logging/log.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <stddef.h>
#include <cheri/cheri_riscv_asm_defines.h>
#endif
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_riscv_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_riscv_user_string_nlen),
};
#endif /* CONFIG_USERSPACE */

#if __riscv_xlen == 32
 #define PR_REG "%08" PRIxPTR
 #define NO_REG "        "
#elif __riscv_xlen == 64
 #define PR_REG "%016" PRIxPTR
 #define NO_REG "                "
#endif

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		#ifdef __CHERI_PURE_CAPABILITY__
		/* cast to unsigned long before printing or change PR_REG format */
		/* to print unsigned long we need to cast from pointer type to unsigned long which are different sizes in CHERI*/
		LOG_ERR("     ca0: " PR_REG "    ct0: " PR_REG, (unsigned long)esf->ca0, (unsigned long)esf->ct0);
		LOG_ERR("     ca1: " PR_REG "    ct1: " PR_REG, (unsigned long)esf->ca1, (unsigned long)esf->ct1);
		LOG_ERR("     ca2: " PR_REG "    ct2: " PR_REG, (unsigned long)esf->ca2, (unsigned long)esf->ct2);
		#else
		LOG_ERR("     a0: " PR_REG "    t0: " PR_REG, esf->a0, esf->t0);
		LOG_ERR("     a1: " PR_REG "    t1: " PR_REG, esf->a1, esf->t1);
		LOG_ERR("     a2: " PR_REG "    t2: " PR_REG, esf->a2, esf->t2);
		#endif
#if defined(CONFIG_RISCV_ISA_RV32E)
		#ifdef __CHERI_PURE_CAPABILITY__
		LOG_ERR("     ca3: " PR_REG, (unsigned long)esf->ca3);
		LOG_ERR("     ca4: " PR_REG, (unsigned long)esf->ca4);
		LOG_ERR("     ca5: " PR_REG, (unsigned long)esf->ca5);
		#else
		LOG_ERR("     a3: " PR_REG, esf->a3);
		LOG_ERR("     a4: " PR_REG, esf->a4);
		LOG_ERR("     a5: " PR_REG, esf->a5);
		#endif
#else
		#ifdef __CHERI_PURE_CAPABILITY__
		LOG_ERR("     ca3: " PR_REG "    ct3: " PR_REG, (unsigned long)esf->ca3, (unsigned long)esf->ct3);
		LOG_ERR("     ca4: " PR_REG "    ct4: " PR_REG, (unsigned long)esf->ca4, (unsigned long)esf->ct4);
		LOG_ERR("     ca5: " PR_REG "    ct5: " PR_REG, (unsigned long)esf->ca5, (unsigned long)esf->ct5);
		LOG_ERR("     ca6: " PR_REG "    ct6: " PR_REG, (unsigned long)esf->ca6, (unsigned long)esf->ct6);
		LOG_ERR("     ca7: " PR_REG, (unsigned long)esf->ca7);
		#else
		LOG_ERR("     a3: " PR_REG "    t3: " PR_REG, esf->a3, esf->t3);
		LOG_ERR("     a4: " PR_REG "    t4: " PR_REG, esf->a4, esf->t4);
		LOG_ERR("     a5: " PR_REG "    t5: " PR_REG, esf->a5, esf->t5);
		LOG_ERR("     a6: " PR_REG "    t6: " PR_REG, esf->a6, esf->t6);
		LOG_ERR("     a7: " PR_REG, esf->a7);
		#endif
#endif /* CONFIG_RISCV_ISA_RV32E */
#ifdef CONFIG_USERSPACE
		#ifdef __CHERI_PURE_CAPABILITY__
		LOG_ERR("     csp: " PR_REG, (unsigned long)esf->csp);
		#else
		LOG_ERR("     sp: " PR_REG, esf->sp);
		#endif
#endif
		#ifdef __CHERI_PURE_CAPABILITY__
		LOG_ERR("     cra: " PR_REG, (unsigned long)esf->cra);
		LOG_ERR("   mepcc: " PR_REG, (unsigned long)esf->mepcc);
		#else
		LOG_ERR("     ra: " PR_REG, esf->ra);
		LOG_ERR("   mepc: " PR_REG, esf->mepc);
		#endif
		LOG_ERR("mstatus: " PR_REG, esf->mstatus);
		LOG_ERR("");
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *cause_str(unsigned long cause)
{
	switch (cause) {
	case 0:
		return "Instruction address misaligned";
	case 1:
		return "Instruction Access fault";
	case 2:
		return "Illegal instruction";
	case 3:
		return "Breakpoint";
	case 4:
		return "Load address misaligned";
	case 5:
		return "Load access fault";
	case 6:
		return "Store/AMO address misaligned";
	case 7:
		return "Store/AMO access fault";
	case 8:
		return "Environment call from U-mode";
	case 9:
		return "Environment call from S-mode";
	case 11:
		return "Environment call from M-mode";
	case 12:
		return "Instruction page fault";
	case 13:
		return "Load page fault";
	case 15:
		return "Store/AMO page fault";

	#ifdef __CHERI_PURE_CAPABILITY__
	/* see CHERI spec - riscv */
	#ifndef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
	case 26:
		return "Load capability page fault";
	case 27:
		return "Store/AMO capability page fault";
	#endif
	case 28:
		return "CHERI exception";
	#endif

	default:
		return "unknown";
	}
}

#ifdef __CHERI_PURE_CAPABILITY__
#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
/* 0.9.5 spec */
static char *cap95_type_str(unsigned long mtval2)
{
uint8_t exccode;
	exccode = mtval2 & CHERI_TYPE_MASK;
	switch (exccode) {
	case 0x00:
		return "CHERI instruction fetch fault";
	case 0x01:
		return "CHERI data fault due to load, store or AMO";
	case 0x02:
		return "CHERI jump or branch fault";
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		return "CHERI reserved";
	default:
		return "CHERI exception - unknown code";
		}
}

static char *cap95_cause_str(unsigned long mtval2)
{
uint8_t exccode;

	exccode = mtval2 & CHERI_CAUSE_MASK;
	switch (exccode) {
	case 0x00:
		return "CHERI tag violation";
	case 0x01:
		return "CHERI seal violation";
	case 0x02:
		return "CHERI permission violation";
	case 0x03:
		return "CHERI invalid address violation";
	case 0x04:
		return "CHERI bounds violation";
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		return "CHERI reserved";
	default:
		return "CHERI exception - unknown code";
		}
}

#else
/* cambs v8 spec */
static char *cap_cause_str(unsigned long mtval)
{
	#ifdef __CHERI_PURE_CAPABILITY__

	/*  see CHERI spec - riscv */

	uint8_t exccode;

	exccode = mtval & CHERI_MASK;
	switch (exccode) {
	case 0x01:
		return "CHERI length violation";
	case 0x02:
		return "CHERI tag violation";
	case 0x03:
		return "CHERI seal violation";
	case 0x04:
		return "CHERI type violation";
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x17:
	case 0x1b:
	case 0x08:
	case 0x1c:
		return "CHERI permissions violation";
	case 0x0a:
		return "CHERI bounds cannot be represented";
	case 0x0b:
		return "CHERI unaligned pcc base";
	case 0x10:
		return "CHERI global violation";
	case 0x16:
		return "CHERI permit store local capability violation";
	case 0x19:
		return "CHERI permit cinvoke violation";
	case 0x18:
		return "CHERI access system registers violation";
	case 0x00:
		return "none";
	default:
		return "CHERI exception - unknown code";
		}
	#else
		return "Illegal operation - not in CHERI mode";
	#endif
}
#endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#endif /* __CHERI_PURE_CAPABILITY__ */

static bool bad_stack_pointer(z_arch_esf_t *esf)
{
#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * Check if the kernel stack pointer prior this exception (before
	 * storing the exception stack frame) was in the stack guard area.
	 */
	uintptr_t sp = (uintptr_t)esf + sizeof(z_arch_esf_t);

#ifdef CONFIG_USERSPACE
	if (_current->arch.priv_stack_start != 0 &&
	    sp >= _current->arch.priv_stack_start &&
	    sp <  _current->arch.priv_stack_start + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}

	if (z_stack_is_user_capable(_current->stack_obj) &&
	    sp >= _current->stack_info.start - K_THREAD_STACK_RESERVED &&
	    sp <  _current->stack_info.start - K_THREAD_STACK_RESERVED
		  + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}
#endif /* CONFIG_USERSPACE */

	if (sp >= _current->stack_info.start - K_KERNEL_STACK_RESERVED &&
	    sp <  _current->stack_info.start - K_KERNEL_STACK_RESERVED
		  + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE

	#ifdef __CHERI_PURE_CAPABILITY__
	if ((esf->mstatus & MSTATUS_MPP) == 0 &&
	    (esf->csp < _current->stack_info.start ||
	     esf->csp > _current->stack_info.start +
		       _current->stack_info.size -
		       _current->stack_info.delta)) {
		/* user stack pointer moved outside of its allowed stack */
		return true;
	}
	#else
	if ((esf->mstatus & MSTATUS_MPP) == 0 &&
	    (esf->sp < _current->stack_info.start ||
	     esf->sp > _current->stack_info.start +
		       _current->stack_info.size -
		       _current->stack_info.delta)) {
		/* user stack pointer moved outside of its allowed stack */
		return true;
	}
	#endif
#endif

	return false;
}

void _Fault(z_arch_esf_t *esf)
{
#ifdef CONFIG_USERSPACE
	/*
	 * Perform an assessment whether an PMP fault shall be
	 * treated as recoverable.
	 */
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		unsigned long start = (unsigned long)exceptions[i].start;
		unsigned long end = (unsigned long)exceptions[i].end;
		#ifdef __CHERI_PURE_CAPABILITY__
		if (esf->mepcc >= start && esf->mepcc < end) {
			esf->mepcc = (uintptr_t)exceptions[i].fixup;
			return;
		}
		#else
		if (esf->mepc >= start && esf->mepc < end) {
			esf->mepc = (unsigned long)exceptions[i].fixup;
			return;
		}
		#endif
	}
#endif /* CONFIG_USERSPACE */

	unsigned long mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

#ifndef CONFIG_SOC_OPENISA_RV32M1_RISCV32
	unsigned long mtval;

	__asm__ volatile("csrr %0, mtval" : "=r" (mtval));

	#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
	unsigned long mtval2;

	__asm__ volatile("csrr %0, mtval2" : "=r" (mtval2));


	#endif
#endif

	mcause &= SOC_MCAUSE_EXP_MASK;
	LOG_ERR("");
	LOG_ERR(" mcause: %ld, %s", mcause, cause_str(mcause));
#ifndef CONFIG_SOC_OPENISA_RV32M1_RISCV32
	#ifdef __CHERI_PURE_CAPABILITY__
	#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
	/* log specific CHERI 0.9.5 spec violation */
	LOG_ERR(" mtval2type: %ld, %s", mtval, cap95_type_str(mtval2));
	LOG_ERR(" mtval2cause: %ld, %s", mtval, cap95_cause_str(mtval2));
	#else
	/* log specific CHERI violation */
	LOG_ERR(" mtval: %ld, %s", mtval, cap_cause_str(mtval));
	#endif


	#else
	LOG_ERR("  mtval: %lx", mtval);
	#endif
#endif

	unsigned int reason = K_ERR_CPU_EXCEPTION;

	if (bad_stack_pointer(esf)) {
		reason = K_ERR_STACK_CHK_FAIL;
	}

	z_riscv_fatal_error(reason, esf);
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	user_fault(K_ERR_KERNEL_OOPS);
	CODE_UNREACHABLE;
}

void z_impl_user_fault(unsigned int reason)
{
	z_arch_esf_t *oops_esf = _current->syscall_frame;

	if (((_current->base.user_options & K_USER) != 0) &&
		reason != K_ERR_STACK_CHK_FAIL) {
		reason = K_ERR_KERNEL_OOPS;
	}
	z_riscv_fatal_error(reason, oops_esf);
}

static void z_vrfy_user_fault(unsigned int reason)
{
	z_impl_user_fault(reason);
}

#include <syscalls/user_fault_mrsh.c>

#endif /* CONFIG_USERSPACE */
