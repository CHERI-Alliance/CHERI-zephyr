/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2023, University of Birmingham
 */

#include <limits.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

/* For CHERI we need to set the device memory base address as a capability with the correct bounds and permissions */
/* Import the device memory map capability*/
#ifdef __CHERI_PURE_CAPABILITY__
extern void *mmdev_root_cap;
#endif

/* andestech,machine-timer */
#if DT_HAS_COMPAT_STATUS_OKAY(andestech_machine_timer)
#define DT_DRV_COMPAT andestech_machine_timer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000010 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	MTIME_BASE_ADDR(0)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* neorv32-machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(neorv32_machine_timer)
#define DT_DRV_COMPAT neorv32_machine_timer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000010 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	MTIME_BASE_ADDR(0)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* nuclei,systimer */
#elif DT_HAS_COMPAT_STATUS_OKAY(nuclei_systimer)
#define DT_DRV_COMPAT nuclei_systimer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000010 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	MTIME_BASE_ADDR(0)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
#else
#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* sifive,clint0 */
#elif DT_HAS_COMPAT_STATUS_OKAY(sifive_clint0)
#define DT_DRV_COMPAT sifive_clint0

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x0000c000 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	(MTIME_BASE_ADDR(0) + 0xbff8U)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 0x4000U)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
#else
#define MTIME_REG	(DT_INST_REG_ADDR(0) + 0xbff8U)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 0x4000U)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* telink,machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(telink_machine_timer)
#define DT_DRV_COMPAT telink_machine_timer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000010 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	MTIME_BASE_ADDR(0)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* lowrisc,machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(lowrisc_machine_timer)
#define DT_DRV_COMPAT lowrisc_machine_timer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000120 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIME_REG	(MTIME_BASE_ADDR(0) + 0x110)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 0x118)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIME_REG	(DT_INST_REG_ADDR(0) + 0x110)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 0x118)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* niosv-machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(niosv_machine_timer)
#define DT_DRV_COMPAT niosv_machine_timer

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000010 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIMECMP_REG	MTIME_BASE_ADDR(0)
#define MTIME_REG	(MTIME_BASE_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIMECMP_REG	DT_INST_REG_ADDR(0)
#define MTIME_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

/* scr,machine-timer*/
#elif DT_HAS_COMPAT_STATUS_OKAY(scr_machine_timer)
#define DT_DRV_COMPAT scr_machine_timer
#define MTIMER_HAS_DIVIDER

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#define MTIME_MMAP_LENGTH 0x00000020 /* length of memory map */
#define MTIME_BASE_ADDR_SET(n)  __builtin_cheri_address_set(mmdev_root_cap, DT_INST_REG_ADDR_U64(n))
#define MTIME_BASE_ADDR(n)  (uintptr_t)(__builtin_cheri_bounds_set(MTIME_BASE_ADDR_SET(n), MTIME_MMAP_LENGTH))
#define MTIMEDIV_REG	(MTIME_BASE_ADDR(0) + 4)
#define MTIME_REG	(MTIME_BASE_ADDR(0) + 8)
#define MTIMECMP_REG	(MTIME_BASE_ADDR(0) + 16)
#define TIMER_IRQN	DT_INST_IRQN(0)
#else
#define MTIMEDIV_REG	(DT_INST_REG_ADDR_U64(0) + 4)
#define MTIME_REG	(DT_INST_REG_ADDR_U64(0) + 8)
#define MTIMECMP_REG	(DT_INST_REG_ADDR_U64(0) + 16)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif /* __CHERI_PURE_CAPABILITY__ */

#endif

#define CYC_PER_TICK (uint32_t)(sys_clock_hw_cycles_per_sec() \
				/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* the unsigned long cast limits divisions to native CPU register width */
#define cycle_diff_t unsigned long

static struct k_spinlock lock;
static uint64_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQN;
#endif

static uintptr_t get_hart_mtimecmp(void)
{
	return MTIMECMP_REG + (arch_proc_id() * 8);
}

static void set_mtimecmp(uint64_t time)
{
#ifdef CONFIG_64BIT
	*(volatile uint64_t *)get_hart_mtimecmp() = time;
#else
	volatile uint32_t *r = (uint32_t *)get_hart_mtimecmp();

	/* Per spec, the RISC-V MTIME/MTIMECMP registers are 64 bit,
	 * but are NOT internally latched for multiword transfers.  So
	 * we have to be careful about sequencing to avoid triggering
	 * spurious interrupts: always set the high word to a max
	 * value first.
	 */
	r[1] = 0xffffffff;
	r[0] = (uint32_t)time;
	r[1] = (uint32_t)(time >> 32);
#endif
}

static void set_divider(void)
{
#ifdef MTIMER_HAS_DIVIDER
	*(volatile uint32_t *)MTIMEDIV_REG =
		CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER;
#endif
}

static uint64_t mtime(void)
{
#ifdef CONFIG_64BIT
	return *(volatile uint64_t *)MTIME_REG;
#else
	volatile uint32_t *r = (uint32_t *)MTIME_REG;
	uint32_t lo, hi;

	/* Likewise, must guard against rollover when reading */
	do {
		hi = r[1];
		lo = r[0];
	} while (r[1] != hi);

	return (((uint64_t)hi) << 32) | lo;
#endif
}

/* CONFIG_ISR_TABLE_USE_SYMBOLS was added for CHERI to link symbols, so the compiler can determine the capability for the function in the ISR table, but can also be used for non-capabilities */
/* When using symbols in the ISR table (instead of fixed addresses) include the non-static function head here */
/* Symbols are necessary for CHERI */
#ifdef CONFIG_CHERI
/* only check if configured for CHERI */
BUILD_ASSERT(CONFIG_CHERI > CONFIG_ISR_TABLE_USE_SYMBOLS, "CONFIG_ISR_TABLE_USE_SYMBOLS is necessary for CHERI");
#endif
#ifdef CONFIG_ISR_TABLE_USE_SYMBOLS
/* The CONFIG_ISR_TABLE_USE_SYMBOLS option is only available for RISCV at present */
BUILD_ASSERT(CONFIG_ISR_TABLE_USE_SYMBOLS > CONFIG_RISCV, "CONFIG_ISR_TABLE_USE_SYMBOLS is is only available for RISCV");
#ifdef CONFIG_RISCV
void timer_isr(const void *arg)
#endif /*CONFIG_RISCV*/
#else
static void timer_isr(const void *arg)
#endif /*CONFIG_ISR_TABLE_USE_SYMBOLS */
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t now = mtime();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYC_PER_TICK;

	last_count += (cycle_diff_t)dticks * CYC_PER_TICK;
	last_ticks += dticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next = last_count + CYC_PER_TICK;

		set_mtimecmp(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(dticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		set_mtimecmp(UINT64_MAX);
		return;
	}

	/*
	 * Clamp the max period length to a number of cycles that can fit
	 * in half the range of a cycle_diff_t for native width divisions
	 * to be usable elsewhere. Also clamp it to half the range of an
	 * int32_t as this is the type used for elapsed tick announcements.
	 * The half range gives us extra room to cope with the unavoidable IRQ
	 * servicing latency. The compiler should optimize away the least
	 * restrictive of those tests automatically.
	 */

	/*
	 * when compiling with llvm-cheri (uses clang) we get a compiler warning here when we
	 * do not include the (int32_t) type cast for ticks which is int32_t type:
	 * warning: implicit conversion from 'unsigned long' to 'int32_t' (aka 'int')
	 * changes value from 92233720368547 to -702313053 [-Wconstant-conversion].
	 * When running with twister the warning is converted to an error and will
	 * not pass the tests.
	 */
	#if defined(__clang__)
	/* include the (int32_t) type cast */
	ticks = (int32_t)CLAMP(ticks, 0, (cycle_diff_t)-1 / 2 / CYC_PER_TICK);
	#else
	/* do as normal */
	ticks = CLAMP(ticks, 0, (cycle_diff_t)-1 / 2 / CYC_PER_TICK);
	#endif
	ticks = CLAMP(ticks, 0, INT32_MAX / 2);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t cyc = (last_ticks + last_elapsed + ticks) * CYC_PER_TICK;

	set_mtimecmp(cyc);
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mtime();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYC_PER_TICK;

	last_elapsed = dticks;
	k_spin_unlock(&lock, key);
	return dticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ((uint32_t)mtime()) << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return mtime() << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER;
}

static int sys_clock_driver_init(void)
{

	set_divider();

	IRQ_CONNECT(TIMER_IRQN, 0, timer_isr, NULL, 0);
	last_ticks = mtime() / CYC_PER_TICK;
	last_count = last_ticks * CYC_PER_TICK;
	set_mtimecmp(last_count + CYC_PER_TICK);
	irq_enable(TIMER_IRQN);
	return 0;
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	set_mtimecmp(last_count + CYC_PER_TICK);
	irq_enable(TIMER_IRQN);
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
