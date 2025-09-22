/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ipi.h>
#include <ksched.h>

#include <zephyr/kernel.h>

/*
 * For CHERI we need to set the device memory base address as a capability with
 * the correct bounds and permissions, import the device memory map capability.
 */
#ifdef __CHERI_PURE_CAPABILITY__
extern void *mmdev_root_cap;
#endif

#define CLINT_NODE DT_NODELABEL(clint)
#if !DT_NODE_EXISTS(CLINT_NODE)
#error "Label 'clint' is not defined in the devicetree."
#endif
#define MSIP_BASE DT_REG_ADDR_RAW(CLINT_NODE)

/*
 * For CHERI we need to set the base address as a capability with
 * the correct bounds and permissions
 */
#ifdef __CHERI_PURE_CAPABILITY__
/* change bound length depending upon number of CPU/hartid */
#define MSIP_BASE_LENGTH sizeof(uint32_t) * CONFIG_MP_MAX_NUM_CPUS
#define MSIP_BASE_SET    __builtin_cheri_address_set(mmdev_root_cap, MSIP_BASE)
#define MSIP_BASE_ADDR   __builtin_cheri_bounds_set(MSIP_BASE_SET, MSIP_BASE_LENGTH)
#define MSIP(hartid)     ((volatile uint32_t *)MSIP_BASE_ADDR)[hartid]

/* Otherwise set as normal */
#else
#define MSIP(hartid) ((volatile uint32_t *)MSIP_BASE)[hartid]
#endif

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED     0
#define IPI_FPU_FLUSH 1

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online && ((cpu_bitmap & BIT(i)) != 0)) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			MSIP(_kernel.cpus[i].arch.hartid) = 1;
		}
	}

	arch_irq_unlock(key);
}

#ifdef CONFIG_FPU_SHARING
void arch_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	MSIP(_kernel.cpus[cpu].arch.hartid) = 1;
}
#endif /* CONFIG_FPU_SHARING */

/*
 * CONFIG_ISR_TABLE_USE_SYMBOLS was added for CHERI to link symbols,
 * so the compiler can determine the capability for the function in
 * the ISR table, but can also be used for non-capabilities.
 * When using symbols in the ISR table (instead of fixed addresses)
 * include the non-static function head here.
 * Symbols are necessary for CHERI
 */
#ifdef CONFIG_CHERI
/* only check if configured for CHERI */
BUILD_ASSERT(CONFIG_CHERI > CONFIG_ISR_TABLE_USE_SYMBOLS,
	     "CONFIG_ISR_TABLE_USE_SYMBOLS is necessary for CHERI");
#endif
#ifdef CONFIG_ISR_TABLE_USE_SYMBOLS
/* The CONFIG_ISR_TABLE_USE_SYMBOLS option is only available for RISCV at present */
BUILD_ASSERT(CONFIG_ISR_TABLE_USE_SYMBOLS > CONFIG_RISCV,
	     "CONFIG_ISR_TABLE_USE_SYMBOLS is only available for RISCV");
#ifdef CONFIG_RISCV
void sched_ipi_handler(const void *unused)
#endif /*CONFIG_RISCV*/
#else
static void sched_ipi_handler(const void *unused)
#endif /*CONFIG_ISR_TABLE_USE_SYMBOLS */
{
	ARG_UNUSED(unused);

	MSIP(csr_read(mhartid)) = 0;

	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[_current_cpu->id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
#ifdef CONFIG_FPU_SHARING
	if (pending_ipi & ATOMIC_MASK(IPI_FPU_FLUSH)) {
		/* disable IRQs */
		csr_clear(mstatus, MSTATUS_IEN);
		/* perform the flush */
		arch_flush_local_fpu();
		/*
		 * No need to re-enable IRQs here as long as
		 * this remains the last case.
		 */
	}
#endif /* CONFIG_FPU_SHARING */
}

#ifdef CONFIG_FPU_SHARING
/*
 * Make sure there is no pending FPU flush request for this CPU while
 * waiting for a contended spinlock to become available. This prevents
 * a deadlock when the lock we need is already taken by another CPU
 * that also wants its FPU content to be reinstated while such content
 * is still live in this CPU's FPU.
 */
void arch_spin_relax(void)
{
	atomic_val_t *pending_ipi = &cpu_pending_ipi[_current_cpu->id];

	if (atomic_test_and_clear_bit(pending_ipi, IPI_FPU_FLUSH)) {
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#endif /* CONFIG_FPU_SHARING */

int arch_smp_init(void)
{

	IRQ_CONNECT(RISCV_IRQ_MSOFT, 0, sched_ipi_handler, NULL, 0);
	irq_enable(RISCV_IRQ_MSOFT);

	return 0;
}
