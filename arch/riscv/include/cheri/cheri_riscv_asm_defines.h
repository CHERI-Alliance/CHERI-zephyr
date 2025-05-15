/*
 * Copyright (c) 2025 University of Birmingham, to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CHERI_RISCV_ASM_DEFINES_H
#define _CHERI_RISCV_ASM_DEFINES_H


/* CHERI-ISA versions */
/* versions supported:*/
/* 1: CHERI-ISA V8.0 - see https://www.cl.cam.ac.uk/techreports/ ISSN 1476-2986 */
/* 2: RISC-V Specification for CHERI Extensions  Version v0.9.5, 2025-02-14: Stable */

/* support checks */
#ifdef CONFIG_RISCV_ISA_ZCHERIHYBRID_ABI
#error "This code does not yet support CHERI RISC-V hybrid. Check your -march and CONFIG settings"
#endif

/*----------------------------------------------------------------------------------------------------*/
/* CAPABILITY MASKS */
/*----------------------------------------------------------------------------------------------------*/
#ifdef __CHERI_PURE_CAPABILITY__
/* needed for setting device memory map capabilities */
#define RISCV_DEVICE_MMAP_SIZE 0x80000000 /* 0x0 to 0x80000000 */
/* exception masks */
  #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
    #define CHERI_TYPE_MASK 0x0f0000
    #define CHERI_CAUSE_MASK 0x0f
  #else
    #define CHERI_MASK 0x1f
  #endif
#endif

/*----------------------------------------------------------------------------------------------------*/
/* CAPABILITY PERMISSIONS */
/*----------------------------------------------------------------------------------------------------*/
#ifdef __CHERI_PURE_CAPABILITY__
/* v0.9.x version of the CHERI spec */
/*MXLEN=64*/
/*AP field bits 44 to 51*/
/* treated as a 20 bit block 63 downto 44 when using the M_CANDPERM instruction (so includes M, SDP, and reserved fields), use ~ to remove a permission from a capability.  */
#if defined(__riscv_zcheripurecap)
  #define CHERI_PERM_CAP			__CHERI_CAP_PERMISSION_CAPABILITY__
  #define CHERI_PERM_WRITE		__CHERI_CAP_PERMISSION_WRITE__
  #define CHERI_PERM_READ			__CHERI_CAP_PERMISSION_READ__
  #define CHERI_PERM_EXECUTE		__CHERI_CAP_PERMISSION_EXECUTE__
  #define CHERI_PERM_SYSTEM_REGS		__CHERI_CAP_PERMISSION_ACCESS_SYSTEM_REGISTERS__
  #define CHERI_PERM_LOAD_MUTABLE		__CHERI_CAP_PERMISSION_LOAD_MUTABLE__
  #if defined(__riscv_zcherilevels)
   #define CHERI_PERM_ELEVATE_LEVEL	__CHERI_CAP_PERMISSION_ELEVATE_LEVEL__
   #define CHERI_PERM_STORE_LEVEL		__CHERI_CAP_PERMISSION_STORE_LEVEL__
   /* not a permission */
   #define CHERI_PERM_CAPABILITY_LEVEL	__CHERI_CAP_PERMISSION_CAPABILITY_LEVEL__
  #endif /* defined(__riscv_zcherilevels) */

/* Capability permissions for Executable with load/store access*/
#define CHERI_PERM_EXECUTABLE_AND_LOADSTORE (~0x0)

/* Capability permissions for Executable */
#define CHERI_PERM_EXECUTABLE	(~CHERI_PERM_WRITE)

/* Capability permissions for Interrupt stacks */
#define CHERI_PERM_INTERRUPT_STACKS (~CHERI_PERM_EXECUTE)

/* Capability permissions for kernel data */
#define CHERI_PERM_KERNEL_DATA (~CHERI_PERM_EXECUTE)

/* Capability permissions for device memory region */
/* Permit normal load and stores only */
#define CHERI_PERM_DEVICE_MEMORY \
		(~(CHERI_PERM_CAP | \
		CHERI_PERM_EXECUTE | \
		CHERI_PERM_SYSTEM_REGS | \
		CHERI_PERM_LOAD_MUTABLE))

/* Capability permissions for Read Only Data */
#define CHERI_PERM_R_DATA (~(CHERI_PERM_WRITE | CHERI_PERM_EXECUTE))

/* Capability permissions for Write Only Data */
#define CHERI_PERM_W_DATA (~(CHERI_PERM_READ | CHERI_PERM_EXECUTE))

#else /* ! __riscv_zcheripurecap */
/* CHERI-ISA V8.0 */
/* PERMISSIONS */
/* treated as a 20 bit block when using the M_CANDPERM instruction (so includes other fields), use ~ to remove a permission from a capability.  */
#define	CHERI_GLOBAL				(1 << 0)	/* 0x00000001 */
#define	CHERI_PERMIT_EXECUTE			(1 << 1)	/* 0x00000002 */
#define	CHERI_PERMIT_LOAD			(1 << 2)	/* 0x00000004 */
#define	CHERI_PERMIT_STORE			(1 << 3)	/* 0x00000008 */
#define	CHERI_PERMIT_LOAD_CAPABILITY		(1 << 4)	/* 0x00000010 */
#define	CHERI_PERMIT_STORE_CAPABILITY		(1 << 5)	/* 0x00000020 */
#define	CHERI_PERMIT_STORE_LOCAL_CAPABILITY	(1 << 6)	/* 0x00000040 */
#define	CHERI_PERMIT_SEAL			(1 << 7)	/* 0x00000080 */
#define	CHERI_PERMIT_CINVOKE			(1 << 8)	/* 0x00000100 */
#define	CHERI_PERMIT_UNSEAL			(1 << 9)	/* 0x00000200 */
#define	CHERI_PERMIT_ACCESS_SYSTEM_REGISTERS	(1 << 10)	/* 0x00000400 */
#define	CHERI_PERMIT_SET_CID			(1 << 11)	/* 0x00000800 */


/* Capability permissions for Executable with load/store access*/
#define CHERI_PERM_EXECUTABLE_AND_LOADSTORE \
		(~(CHERI_PERMIT_SEAL | \
		CHERI_PERMIT_CINVOKE | \
		CHERI_PERMIT_UNSEAL | \
		CHERI_PERMIT_SET_CID))

/* Capability permissions for Executable */
#define CHERI_PERM_EXECUTABLE \
		(~(CHERI_PERMIT_STORE | \
		CHERI_PERMIT_SEAL | \
		CHERI_PERMIT_CINVOKE | \
		CHERI_PERMIT_UNSEAL | \
		CHERI_PERMIT_SET_CID))

/* Capability permissions for Interrupt stacks */
#define CHERI_PERM_INTERRUPT_STACKS \
		(~(CHERI_PERMIT_EXECUTE | \
		CHERI_PERMIT_SEAL | \
		CHERI_PERMIT_CINVOKE | \
		CHERI_PERMIT_UNSEAL | \
		CHERI_PERMIT_SET_CID))

/* Capability permissions for kernel data */
#define CHERI_PERM_KERNEL_DATA \
		(~(CHERI_PERMIT_EXECUTE | \
		CHERI_PERMIT_SEAL | \
		CHERI_PERMIT_CINVOKE | \
		CHERI_PERMIT_UNSEAL | \
		CHERI_PERMIT_SET_CID))

/* Capability permissions for device memory region */
/* Permit normal load and stores only */
#define CHERI_PERM_DEVICE_MEMORY \
		(~(CHERI_GLOBAL | \
		CHERI_PERMIT_EXECUTE | \
		CHERI_PERMIT_LOAD_CAPABILITY | \
		CHERI_PERMIT_STORE_CAPABILITY | \
		CHERI_PERMIT_STORE_LOCAL_CAPABILITY | \
		CHERI_PERMIT_SEAL | \
		CHERI_PERMIT_CINVOKE | \
		CHERI_PERMIT_UNSEAL | \
		CHERI_PERMIT_SET_CID))


 #endif /* defined(__riscv_zcheripurecap) */
#endif /* __CHERI_PURE_CAPABILITY__*/

/*----------------------------------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------------*/
/* INSTRUCTION MAPPING FOR DIFFERENT CHERI SPECS */
/*----------------------------------------------------------------------------------------------------*/
/* instructions */
#ifdef __CHERI_PURE_CAPABILITY__
  #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
  /* 0.9.5 spec */
  #define M_CLLC			llc /* integer and cap */
  #define M_CSPECIALW			csrw
  #define M_CSPECIALR			csrr
  #define M_CGETBASE			gcBase
  #define M_CSETADDR			scAddr
  #define M_CSETBOUNDS			scBndsr
  #define M_CGETTAG			gcTag
  #define M_CGETPERM			gcPerm
  #define M_CANDPERM			acPerm
  #define M_CMOVE			cmv
  #define M_CJR				jalr /* integer and cap */
  #define M_CINCOFFSET			cadd
  #define M_CINCOFFSETIMM		caddi
  #define M_CTAIL			tail /* integer and cap */
  #define M_CJAL			jal /* integer and cap */
  #define M_CJALR			jalr /* integer and cap */
  #define M_CCALL			call /* integer and cap */
  #define M_CMOVE_PCC			cmv


  #define M_CFLD			fld /* integer and cap */
  #define M_CFSD			fsd /* integer and cap */
  #define M_CFLW			flw /* integer and cap */
  #define M_CFSW			fsw /* integer and cap */

  #define M_CSW				sw /* integer and cap */
  #define M_CRET			ret /* integer and cap */
  #define M_CLW				lw /* integer and cap */
  #define M_CSC				sc /* integer and cap */
  #define M_CLC				lc /* integer and cap */
  #define M_CLB				lb /* integer and cap */
  #define M_CSW				sw /* integer and cap */
  #define M_CLBU			lbu

  #ifdef _ASMLANGUAGE
 /* assembly only */
	.macro M_CLGC cd, sym
	 1 : auipc \cd, % captab_pcrel_hi(\sym)
	 clc \cd, % pcrel_lo(1b)(\cd)
	.endm
  #else
  /* c only */
	#define M_CLGC(cd, sym) ({ \
	asm volatile ("1: auipc cd, %captab_pcrel_hi(sym)\n\t" \
	"clc cd, %pcrel_lo(1b)(cd)\n\t" \
	: "=r" (cd) : "r" (sym)); \
	cd; \
})

  #endif /* __ASSEMBLY__ */


  #else
  /* camb 8.0 spec */
  #define M_CLLC			cllc
  #define M_CSPECIALW			cSpecialW
  #define M_CSPECIALR			cSpecialR
  #define M_CGETBASE			cGetBase
  #define M_CSETADDR			cSetAddr
  #define M_CSETBOUNDS			cSetbounds
  #define M_CGETTAG			cGetTag
  #define M_CGETPERM			cGetPerm
  #define M_CANDPERM			cAndPerm
  #define M_CMOVE			cMove
  #define M_CJR				cjr
  #define M_CINCOFFSET			cincoffset
  #define M_CINCOFFSETIMM		cincoffsetImm
  #define M_CTAIL			ctail
  #define M_CJAL			cjal
  #define M_CJALR			cjalr
  #define M_CCALL			ccall
  #define M_CMOVE_PCC			M_CSPECIALR
  #define M_CLGC			clgc

  #define M_CFLD			cfld
  #define M_CFSD			cfsd
  #define M_CFLW			cflw
  #define M_CFSW			cfsw

  #define M_CSW				csw
  #define M_CRET			cret
  #define M_CLW				clw
  #define M_CSC				csc
  #define M_CLC				clc
  #define M_CSW				csw
  #define M_CLBU			clbu

  #endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#else
  /* riscv spec */
#endif /* __CHERI_PURE_CAPABILITY__ */
/*----------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------*/
/* CSR REGISTER MAPPING FOR DIFFERENT CHERI SPECS */
/*----------------------------------------------------------------------------------------------------*/
#ifdef __CHERI_PURE_CAPABILITY__
  #ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
  /* 0.9.5 spec */
  #define M_MTCC	mtvecc
  #define M_MTDC	mtdc
  #else
  /* camb 8.0 spec */
  #define M_MTCC	mtcc
  #define M_MTDC	mtdc
  #endif
#else
/*riscv spec */
  #define M_MTCC	mtvec
#endif
/*----------------------------------------------------------------------------------------------------*/

#endif /* _CHERI_RISCV_ASM_DEFINES_H */

