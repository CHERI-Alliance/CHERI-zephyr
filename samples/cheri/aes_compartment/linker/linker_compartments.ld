/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/* compartment1 sections */

/* entry point */
SECTION_PROLOGUE(.compartment1.entry,,)
{
    *. = ALIGN(0x200);       /* align start (512) */
    __compartment1_start = .;*/

    KEEP(*(.compartment1.entry))
    KEEP(*(.compartment1.entry.*))

    *. = ALIGN(0x200);       /* align end (512) */
} GROUP_LINK_IN(ROMABLE_REGION)

/* compartment1 rodata section */
SECTION_PROLOGUE(.compartment1.rodata,,)
{
    *. = ALIGN(0x200);       /* align start (512) */
    __sbox_loc = .;*/

    KEEP(*tables.o*(.tables));
    KEEP(*tables.o*(.tables.*));
    KEEP(*lib_chericompartment1_aeslib.a*(.rodata));
    KEEP(*lib_chericompartment1_aeslib.a*(.rodata.*));

    *. = ALIGN(0x200);       /* align end (512) */
} GROUP_LINK_IN(ROMABLE_REGION)


/* compartment1 text section */
SECTION_PROLOGUE(.compartment1.text,,)
{
    *. = ALIGN(0x200);       /* align start (512) */

    KEEP(*lib_chericompartment1_aeslib.a*(.text));
    KEEP(*lib_chericompartment1_aeslib.a*(.text.*));

    *. = ALIGN(0x200);       /* align end (512) */
} GROUP_LINK_IN(ROMABLE_REGION)


/* compartment1 bss section */
SECTION_PROLOGUE(.compartment1.bss,,)
{
    *. = ALIGN(0x200);       /* align start (512) */

    KEEP(*lib_chericompartment1_aeslib.a*(.bss));
    KEEP(*lib_chericompartment1_aeslib.a*(.bss.*));
    KEEP(*lib_chericompartment1_aeslib.a*(.sbss));
    KEEP(*lib_chericompartment1_aeslib.a*(.sbss.*));

    *. = ALIGN(0x200);       /* align end (512) */
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)


/* compartment1 data section */
SECTION_PROLOGUE(.compartment1.data,,)
{
    *. = ALIGN(0x200);       /* align start (512) */

    KEEP(*lib_chericompartment1_aeslib.a*(.data));
    KEEP(*lib_chericompartment1_aeslib.a*(.data.*));

    KEEP(*lib_chericompartment1_aeslib.a*(.sdata));
    KEEP(*lib_chericompartment1_aeslib.a*(.sdata.*));
    KEEP(*lib_chericompartment1_aeslib.a*(.gnu.linkonce.s.*));

    *. = ALIGN(0x200);       /* align end (512) */
    __compartment1_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)



/* entry point for attack compartment */
SECTION_PROLOGUE(.compartment2.entry,,)
{
    *. = ALIGN(0x200);       /* align start (512) */
    __compartment2_start = .;

    KEEP(*(.compartment2.*))
    KEEP(*(.compartment2.text))
    KEEP(*(.compartment2.text.*))
    KEEP(*(.compartment2.entry))
    KEEP(*(.compartment2.entry.*))
    __compartment2_end = .;

    *. = ALIGN(0x200);       /* align end (512) */
} GROUP_LINK_IN(ROMABLE_REGION)



/* non-compartment1 captable */
SECTION_PROLOGUE(capabilities,,)
{
    . = ALIGN(0x10);       /* align start (16) */
    KEEP(*(.captable))
    . = ALIGN(0x10);       /* align start (16) */
} GROUP_LINK_IN(ROMABLE_REGION)



/* relocs */
SECTION_PROLOGUE(relocs,,)
{
    . = ALIGN(0x10);       /* align start (16) */
    /*KEEP(*(.__cap_relocs))*/
    __start___cap_relocs = .;
    *(__cap_relocs*) /* For CHERI */
    __stop___cap_relocs = .;
    . = ALIGN(0x10);       /* align start (16) */
} GROUP_LINK_IN(ROMABLE_REGION)
