/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h> /*auto generated from build/generated/offsets */

#ifdef __CHERI_PURE_CAPABILITY__

#define _thread_offset_to_csp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_csp_OFFSET)

#define _thread_offset_to_cra \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cra_OFFSET)

#define _thread_offset_to_cs0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs0_OFFSET)

#define _thread_offset_to_cs1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs1_OFFSET)

#define _thread_offset_to_cs2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs2_OFFSET)

#define _thread_offset_to_cs3 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs3_OFFSET)

#define _thread_offset_to_cs4 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs4_OFFSET)

#define _thread_offset_to_cs5 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs5_OFFSET)

#define _thread_offset_to_cs6 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs6_OFFSET)

#define _thread_offset_to_cs7 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs7_OFFSET)

#define _thread_offset_to_cs8 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs8_OFFSET)

#define _thread_offset_to_cs9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs9_OFFSET)

#define _thread_offset_to_cs10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs10_OFFSET)

#define _thread_offset_to_cs11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_cs11_OFFSET)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#else


#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

#define _thread_offset_to_ra \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_ra_OFFSET)

#define _thread_offset_to_s0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s0_OFFSET)

#define _thread_offset_to_s1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s1_OFFSET)

#define _thread_offset_to_s2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s2_OFFSET)

#define _thread_offset_to_s3 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s3_OFFSET)

#define _thread_offset_to_s4 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s4_OFFSET)

#define _thread_offset_to_s5 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s5_OFFSET)

#define _thread_offset_to_s6 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s6_OFFSET)

#define _thread_offset_to_s7 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s7_OFFSET)

#define _thread_offset_to_s8 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s8_OFFSET)

#define _thread_offset_to_s9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s9_OFFSET)

#define _thread_offset_to_s10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s10_OFFSET)

#define _thread_offset_to_s11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s11_OFFSET)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#endif /* CHERI */



#if defined(CONFIG_FPU_SHARING)

#define _thread_offset_to_exception_depth \
	(___thread_t_arch_OFFSET + ___thread_arch_t_exception_depth_OFFSET)

#endif



#ifdef CONFIG_USERSPACE

#ifdef __CHERI_PURE_CAPABILITY__
/* used in isr.s */
#define _curr_cpu_arch_user_exc_csp \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_csp_OFFSET)

#define _curr_cpu_arch_user_exc_ctmp0 \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_ctmp0_OFFSET)

#define _curr_cpu_arch_user_exc_ctmp1 \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_ctmp1_OFFSET)

#else

#define _curr_cpu_arch_user_exc_sp \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_sp_OFFSET)

#define _curr_cpu_arch_user_exc_tmp0 \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_tmp0_OFFSET)

#define _curr_cpu_arch_user_exc_tmp1 \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_user_exc_tmp1_OFFSET)

#endif /* CHERI */

#endif

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
