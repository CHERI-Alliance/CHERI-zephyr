/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

/**
 * @file
 * @brief QEMU RISC-V virt machine hardware depended interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/*
 * Exit QEMU and tell error number.
 *   Higher 16bits: indicates error number.
 *   Lower 16bits : set FINISHER_FAIL
 */
#define FINISHER_FAIL		0x3333

/* Exit QEMU successfully */
#define FINISHER_EXIT		0x5555

/* Reboot machine */
#define FINISHER_REBOOT		0x7777

/* For CHERI we need to set the register/base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
	extern void *mmdev_root_cap; /* root capability of the device memory map */
	#define REG_LENGTH sizeof(uint32_t) /* length of register */
	/* define address as a capability, and set bounds. Permissions are set on the device memory mmdev_root_cap*/
	#define REG_ADDR_SET  __builtin_cheri_address_set(mmdev_root_cap, SIFIVE_SYSCON_TEST)
	#define SIFIVE_SYSCON_TEST_CAP  __builtin_cheri_bounds_set(REG_ADDR_SET, REG_LENGTH)
#endif

void sys_arch_reboot(int type)
{

#ifdef __CHERI_PURE_CAPABILITY__
	volatile uint32_t *reg = (uint32_t *)SIFIVE_SYSCON_TEST_CAP;
#else
	volatile uint32_t *reg = (uint32_t *)SIFIVE_SYSCON_TEST;
#endif
	*reg = FINISHER_REBOOT;

	ARG_UNUSED(type);
}
