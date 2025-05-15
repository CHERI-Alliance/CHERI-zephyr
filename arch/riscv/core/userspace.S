/*
 * Userspace and service handler hooks
 *
 * Copyright (c) 2020 BayLibre, SAS
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/riscv/csr.h>
#include "asm_macros.inc"

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

/* exports */
GTEXT(arch_user_string_nlen)
GTEXT(z_riscv_user_string_nlen_fault_start)
GTEXT(z_riscv_user_string_nlen_fault_end)
GTEXT(z_riscv_user_string_nlen_fixup)

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
	li	a5, 0		# Counter
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CSW	a5, 0(ca2)	# Init error value to 0
	#else
	sw	a5, 0(a2)	# Init error value to 0
	#endif

loop:
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CINCOFFSET	ca4, ca0, a5	# Determine character address
	#else
	add	a4, a0, a5	# Determine character address
	#endif
z_riscv_user_string_nlen_fault_start:
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLBU	a4, 0(ca4)	# Load string's character
	#else
	lbu	a4, 0(a4)	# Load string's character
	#endif
z_riscv_user_string_nlen_fault_end:
	beqz	a4, exit	# Test string's end of line

	bne	a5, a1, continue	# Check if max length is reached

exit:
	#ifdef __CHERI_PURE_CAPABILITY__
	mv	a0, a5	# Return counter value (length)
	M_CRET
	#else
	mv	a0, a5		# Return counter value (length)
	ret
	#endif

continue:
	addi	a5, a5, 1	# Increment counter
	j	loop

z_riscv_user_string_nlen_fixup:
	li	a4, -1		# Put error to -1
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CSW	a4, 0(ca2)
	#else
	sw	a4, 0(a2)
	#endif
	j	exit

