/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

#ifndef ZEPHYR_INCLUDE_RISCV_STRUCTS_H_
#define ZEPHYR_INCLUDE_RISCV_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_USERSPACE
	#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * The capability registers are always the size of a pointer which is bigger
	 * than an unsigned long for CHERI
	 */
	uintptr_t user_exc_csp;
	uintptr_t user_exc_ctmp0;
	uintptr_t user_exc_ctmp1;
	#else
	unsigned long user_exc_sp;
	unsigned long user_exc_tmp0;
	unsigned long user_exc_tmp1;
	#endif
#endif
#if defined(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS > 1)
	unsigned long hartid;
	bool online;
#endif
#ifdef CONFIG_FPU_SHARING
	atomic_ptr_val_t fpu_owner;
	uint32_t fpu_state;
#endif
};

#endif /* ZEPHYR_INCLUDE_RISCV_STRUCTS_H_ */
