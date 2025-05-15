/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets.h>
#include "asm_macros.inc"

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri_riscv_asm_defines.h>
#endif

/* exports */
GTEXT(__reset)
GTEXT(__initialize)

/* imports */
GTEXT(_PrepC)
GTEXT(riscv_cpu_wake_flag)
GTEXT(riscv_cpu_sp)
GTEXT(z_riscv_secondary_cpu_init)

#if CONFIG_INCLUDE_RESET_VECTOR
SECTION_FUNC(reset, __reset)
	/*
	 * jump to __initialize
	 * use call opcode in case __initialize is far away.
	 * This will be dependent on linker.ld configuration.
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
	 M_CCALL __initialize
	 #else
	 call __initialize
	 #endif
#endif /* CONFIG_INCLUDE_RESET_VECTOR */

/* use ABI name of registers for the sake of simplicity */

/*
 * Remainder of asm-land initialization code before we can jump into
 * the C domain
 */
 /* select cpu hart to run boot process */
 SECTION_FUNC(TEXT, __initialize)
 		/* mhartid is an integer register so don't need special capability code here */
		csrr a0, mhartid
		li t0, CONFIG_RV_BOOT_HART
		beq a0, t0, boot_first_core
		j boot_secondary_core

boot_first_core:

#ifdef CONFIG_FPU
	/*
	 * Enable floating-point.
	 */
	li t0, MSTATUS_FS_INIT
	/* mstatus is an integer register so don't need special capability code here */
	csrs mstatus, t0

	/*
	 * Floating-point rounding mode set to IEEE-754 default, and clear
	 * all exception flags.
	 */
	fscsr zero
#endif

#ifdef CONFIG_INIT_STACKS

 	#ifdef __CHERI_PURE_CAPABILITY__
 		/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
		M_CLLC ct0, z_interrupt_stacks /* Load PCC-relative capability */
		li t1, __z_interrupt_stack_SIZEOF
		add t1, t1, t0 /* end addr */

		/* Populate z_interrupt_stacks with 0xaaaaaaaa */
		li t2, 0xaaaaaaaa
	aa_loop:
		csw t2, 0x00(ct0)
		M_CINCOFFSETIMM ct0, ct0, 4
		blt t0, t1, aa_loop

 	#else
 		/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
		la t0, z_interrupt_stacks
		li t1, __z_interrupt_stack_SIZEOF
		add t1, t1, t0

		/* Populate z_interrupt_stacks with 0xaaaaaaaa */
		li t2, 0xaaaaaaaa
	aa_loop:
		sw t2, 0x00(t0)
		addi t0, t0, 4
		blt t0, t1, aa_loop
 	#endif

#endif
interrupt_stacks:
	/*
	 * Initially, setup stack pointer to
	 * z_interrupt_stacks + __z_interrupt_stack_SIZEOF
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
	 	M_CLLC ct0, z_interrupt_stacks /* Load PCC-relative capability */
	 	li t1, __z_interrupt_stack_SIZEOF
	 	/* set bounds of sp, needs to be aligned to boundary */
	 	M_CSETADDR ct0, ct0, t0 /* set base to address */
	 	M_CSETBOUNDS ct0, ct0, t1 /* set bounds to size */
	 	/* set permissions */
		li	t2, CHERI_PERM_INTERRUPT_STACKS
	 	M_CANDPERM ct0, ct0, t2
		M_CINCOFFSET ct0, ct0, t1 /* set to start at end of stack */
		M_CMOVE csp, ct0 /* move to csp */

	 #else
		la sp, z_interrupt_stacks
		li t0, __z_interrupt_stack_SIZEOF
		add sp, sp, t0
	#endif

/* do CHERI code bits here to start with */
#ifdef __CHERI_PURE_CAPABILITY__
cheribss:
	/*
	* Clear BSS - need to do this before initialising any global capabilities in the bss section
	* zephyr does this later in boot code for non-cap mode _PrepC
	*/
	M_CLLC	cs0, __bss_start
	M_CLLC	cs1, __bss_end
	/* if nothing in bss skip else bounds exception when write */
	beq s1, s0, 2f
1:
	csd	zero, 0(cs0)
	M_CINCOFFSETIMM cs0, cs0, 8
	bltu	s0, s1, 1b
2:

/* We have now created stacks and cleared the bss, now restrict PCC perms so can only execute code - so can't write to executable code section */
	M_CLLC	ct1, 1f /* where to go now */
	/* set permissions */
	li	t2, CHERI_PERM_EXECUTABLE
	M_CANDPERM ct1, ct1, t2
	M_CJR	ct1 /* jump to new PCC with reduced perms */
/*align jump to 16 bytes for 64 bit arch */
.balign 16
1:

initrelocs:
	/* Save rootcap DDC and clear kernel's DDC. */
	M_CSPECIALR cs0, ddc /* save in cs0 */
	M_CMOVE	ct0, cnull
	M_CSPECIALW ddc, ct0

	/* Initialize cap relocs and set up cap table. */

	/* Set permissions on kernel data cap - arg 1, use ddc in cs0 */
	li	t0, CHERI_PERM_KERNEL_DATA
	M_CANDPERM ca0, cs0, t0

	/* call init_cap_relocs func and return */
	/* also set this as code cap - arg 2 */
	M_CLLC	ca1, init_cap_relocs
	M_CJALR	ca1

initcap:
	/* Initialize capabilities - save some main capabilities need to use later. */
	M_CMOVE	ca0, cs0 /* arg1 ddc root cap */
	/* can now get the capability out of the table (M_CLGC) because has been initialised above */
	M_CLGC	cra, init_cap_roots
	M_CJALR	cra

	/*any boot params to save*/

	/* Discard ddc in cs0. */
	M_CMOVE	cs0, cnull
	/* Clear other capabilities no longer needed. */
	M_CMOVE	cs1, cnull
	M_CMOVE	cs2, cnull
	M_CMOVE	cs3, cnull

#endif
/* do this after init capabilities because purecap uses them to get cap to memory map hardware wdog */
wdog:
#ifdef CONFIG_WDOG_INIT
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CCALL _WdogInit
	#else
	call _WdogInit
	#endif
#endif

prepc:
	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	 #ifdef __CHERI_PURE_CAPABILITY__
	 	/* M_CLLC	ct0, _PrepC */ /* Load PCC-relative capability */
	 	M_CLGC	ct0, _PrepC /* Load cap from table */
		M_CJALR ct0
	 #else
		call _PrepC
         #endif

 stop:
 j stop

/* end here */


/* secondary core wait */
boot_secondary_core:
#if CONFIG_MP_MAX_NUM_CPUS > 1
	#ifdef __CHERI_PURE_CAPABILITY__
	M_CLLC ct0, riscv_cpu_wake_flag
	clw t0, 0(ct0)
	bne a0, t0, boot_secondary_core /* a0 hartid */

	/* Set up stack */
	M_CLLC ct0, riscv_cpu_sp /* load addr relative to pcc */
	clc ct0, 0(ct0) /* load contents at 0 offset, assumes stack saved here already */
        M_CMOVE csp, ct0 /* move to csp */

	M_CLLC ct0, riscv_cpu_wake_flag
	csc cnull, 0(ct0)
	j z_riscv_secondary_cpu_init /* hart id in a0 */

	#else
	la t0, riscv_cpu_wake_flag
	lr t0, 0(t0)
	bne a0, t0, boot_secondary_core

	/* Set up stack */
	la t0, riscv_cpu_sp
	lr sp, 0(t0)

	la t0, riscv_cpu_wake_flag
	sr zero, 0(t0)
	j z_riscv_secondary_cpu_init /* hart id in a0 */
	#endif
#else
	j loop_unconfigured_cores
#endif

loop_unconfigured_cores:
	wfi
	j loop_unconfigured_cores

