/*
 * Copyright (c) 2023 BayLibre SAS
 * Written by: Nicolas Pitre
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
 #ifdef __CHERI_PURE_CAPABILITY__
  #define LOAD	M_CFLD
  #define STORE	M_CFSD
 #else
  #define LOAD	fld
  #define STORE	fsd
 #endif
#else
 #ifdef __CHERI_PURE_CAPABILITY__
  #define LOAD	M_CFLW
  #define STORE	M_CFSW
 #else
  #define LOAD	flw
  #define STORE	fsw
 #endif
#endif

#define DO_FP_REGS(op, ptr) \
	op fa0,  __z_riscv_fp_context_t_fa0_OFFSET (ptr); \
	op fa1,  __z_riscv_fp_context_t_fa1_OFFSET (ptr); \
	op fa2,  __z_riscv_fp_context_t_fa2_OFFSET (ptr); \
	op fa3,  __z_riscv_fp_context_t_fa3_OFFSET (ptr); \
	op fa4,  __z_riscv_fp_context_t_fa4_OFFSET (ptr); \
	op fa5,  __z_riscv_fp_context_t_fa5_OFFSET (ptr); \
	op fa6,  __z_riscv_fp_context_t_fa6_OFFSET (ptr); \
	op fa7,  __z_riscv_fp_context_t_fa7_OFFSET (ptr); \
	op fs0,  __z_riscv_fp_context_t_fs0_OFFSET (ptr); \
	op fs1,  __z_riscv_fp_context_t_fs1_OFFSET (ptr); \
	op fs2,  __z_riscv_fp_context_t_fs2_OFFSET (ptr); \
	op fs3,  __z_riscv_fp_context_t_fs3_OFFSET (ptr); \
	op fs4,  __z_riscv_fp_context_t_fs4_OFFSET (ptr); \
	op fs5,  __z_riscv_fp_context_t_fs5_OFFSET (ptr); \
	op fs6,  __z_riscv_fp_context_t_fs6_OFFSET (ptr); \
	op fs7,  __z_riscv_fp_context_t_fs7_OFFSET (ptr); \
	op fs8,  __z_riscv_fp_context_t_fs8_OFFSET (ptr); \
	op fs9,  __z_riscv_fp_context_t_fs9_OFFSET (ptr); \
	op fs10, __z_riscv_fp_context_t_fs10_OFFSET(ptr); \
	op fs11, __z_riscv_fp_context_t_fs11_OFFSET(ptr); \
	op ft0,  __z_riscv_fp_context_t_ft0_OFFSET (ptr); \
	op ft1,  __z_riscv_fp_context_t_ft1_OFFSET (ptr); \
	op ft2,  __z_riscv_fp_context_t_ft2_OFFSET (ptr); \
	op ft3,  __z_riscv_fp_context_t_ft3_OFFSET (ptr); \
	op ft4,  __z_riscv_fp_context_t_ft4_OFFSET (ptr); \
	op ft5,  __z_riscv_fp_context_t_ft5_OFFSET (ptr); \
	op ft6,  __z_riscv_fp_context_t_ft6_OFFSET (ptr); \
	op ft7,  __z_riscv_fp_context_t_ft7_OFFSET (ptr); \
	op ft8,  __z_riscv_fp_context_t_ft8_OFFSET (ptr); \
	op ft9,  __z_riscv_fp_context_t_ft9_OFFSET (ptr); \
	op ft10, __z_riscv_fp_context_t_ft10_OFFSET(ptr); \
	op ft11, __z_riscv_fp_context_t_ft11_OFFSET(ptr)

GTEXT(z_riscv_fpu_save)
SECTION_FUNC(TEXT, z_riscv_fpu_save)

	frcsr t0
	#ifdef __CHERI_PURE_CAPABILITY__
	DO_FP_REGS(STORE, ca0)
	M_CSW t0, __z_riscv_fp_context_t_fcsr_OFFSET(ca0)
	M_CRET
	#else
	DO_FP_REGS(STORE, a0)
	sw t0, __z_riscv_fp_context_t_fcsr_OFFSET(a0)
	ret
	#endif

GTEXT(z_riscv_fpu_restore)
SECTION_FUNC(TEXT, z_riscv_fpu_restore)
	#ifdef __CHERI_PURE_CAPABILITY__
	DO_FP_REGS(LOAD, ca0)
	M_CLW t0, __z_riscv_fp_context_t_fcsr_OFFSET(ca0)
	fscsr t0
	M_CRET
	#else
	DO_FP_REGS(LOAD, a0)
	lw t0, __z_riscv_fp_context_t_fcsr_OFFSET(a0)
	fscsr t0
	ret
	#endif

