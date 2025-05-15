/*
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2023 University of Birmingham, Modified to support CHERI
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI codasip xa730, v0.9.x CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/pm_cpu_ops.h>

/* For CHERI we need to set the device memory base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
#include <zephyr/arch/riscv/cheri/cheri_funcs.h> /* cheri_build_device_cap */
#endif

volatile struct {
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_init[CONFIG_MP_MAX_NUM_CPUS];

volatile uintptr_t riscv_cpu_wake_flag;
volatile void *riscv_cpu_sp;

extern void __start(void);

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	riscv_cpu_init[cpu_num].fn = fn;
	riscv_cpu_init[cpu_num].arg = arg;

	riscv_cpu_sp = Z_KERNEL_STACK_BUFFER(stack) + sz;
	riscv_cpu_wake_flag = _kernel.cpus[cpu_num].arch.hartid;

#ifdef CONFIG_PM_CPU_OPS
	if (pm_cpu_on(cpu_num, (uintptr_t)&__start)) {
		printk("Failed to boot secondary CPU %d\n", cpu_num);
		return;
	}
#endif

	while (riscv_cpu_wake_flag != 0U) {
		;
	}
}

void z_riscv_secondary_cpu_init(int hartid)
{
	unsigned int i;
	unsigned int cpu_num = 0;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (_kernel.cpus[i].arch.hartid == hartid) {
			cpu_num = i;
		}
	}
	#ifdef __CHERI_PURE_CAPABILITY__
	/* CHERI extends mscratch register to mscratchc */
	csr_cap_write(mscratchc, &_kernel.cpus[cpu_num]);
	#else
	csr_write(mscratch, &_kernel.cpus[cpu_num]);
	#endif
#ifdef CONFIG_SMP
	_kernel.cpus[cpu_num].arch.online = true;
#endif
#if defined(CONFIG_MULTITHREADING) && defined(CONFIG_THREAD_LOCAL_STORAGE)
	#ifdef __CHERI_PURE_CAPABILITY__
	/* CHERI extends thread pointer register tp to ctp */
	/* assign like this to remove error: couldn't allocate input reg for constraint 'r' */
	register uintptr_t ca0 __asm__ ("ca0") = (uintptr_t)z_idle_threads[cpu_num].tls;
	__asm__("#M_CMOVE ctp, %0" : : "r" (ca0));
	#else
	__asm__("mv tp, %0" : : "r" (z_idle_threads[cpu_num].tls));
	#endif
#endif
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
#ifdef CONFIG_RISCV_PMP
	z_riscv_pmp_init();
#endif
#ifdef CONFIG_SMP
	irq_enable(RISCV_MACHINE_SOFT_IRQ);
#endif
	riscv_cpu_init[cpu_num].fn(riscv_cpu_init[cpu_num].arg);
}

#ifdef CONFIG_SMP

/* For CHERI we need to set the base address as a capability with the correct bounds and permissions */
#ifdef __CHERI_PURE_CAPABILITY__
/* change bound length depending upon number of CPU/hartid */
#define RISCV_MSIP_BASE_LENGTH sizeof(uint32_t)*CONFIG_MP_MAX_NUM_CPUS
#define RISCV_MSIP_BASE_ADDR (uintptr_t) cheri_build_device_cap(RISCV_MSIP_BASE, RISCV_MSIP_BASE_LENGTH)
#define MSIP(hartid) ((volatile uint32_t *)RISCV_MSIP_BASE_ADDR)[hartid]
/* Otherwise set as normal */
#else
#define MSIP(hartid) ((volatile uint32_t *)RISCV_MSIP_BASE)[hartid]
#endif

static atomic_val_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];
#define IPI_SCHED	0
#define IPI_FPU_FLUSH	1

void arch_sched_ipi(void)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if (i != id && _kernel.cpus[i].arch.online) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			MSIP(_kernel.cpus[i].arch.hartid) = 1;
		}
	}

	arch_irq_unlock(key);
}

#ifdef CONFIG_FPU_SHARING
void z_riscv_flush_fpu_ipi(unsigned int cpu)
{
	atomic_set_bit(&cpu_pending_ipi[cpu], IPI_FPU_FLUSH);
	MSIP(_kernel.cpus[cpu].arch.hartid) = 1;
}
#endif

/* CONFIG_ISR_TABLE_USE_SYMBOLS was added for CHERI to link symbols, so the compiler can determine the capability for the function in the ISR table, but can also be used for non-capabilities */
/* When using symbols in the ISR table (instead of fixed addresses) include the non-static function head here */
/* Symbols are necessary for CHERI */
#ifdef CONFIG_CHERI
/* only check if configured for CHERI */
BUILD_ASSERT(CONFIG_CHERI > CONFIG_ISR_TABLE_USE_SYMBOLS, "CONFIG_ISR_TABLE_USE_SYMBOLS is necessary for CHERI");
#endif
#ifdef CONFIG_ISR_TABLE_USE_SYMBOLS
/* The CONFIG_ISR_TABLE_USE_SYMBOLS option is only available for RISCV at present */
BUILD_ASSERT(CONFIG_ISR_TABLE_USE_SYMBOLS > CONFIG_RISCV, "CONFIG_ISR_TABLE_USE_SYMBOLS is only available for RISCV");
#ifdef CONFIG_RISCV
void ipi_handler(const void *unused)
#endif /*CONFIG_RISCV*/
#else
static void ipi_handler(const void *unused)
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
		z_riscv_flush_local_fpu();
		/*
		 * No need to re-enable IRQs here as long as
		 * this remains the last case.
		 */
	}
#endif
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
		 * z_riscv_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#endif

static int riscv_smp_init(void)
{

	IRQ_CONNECT(RISCV_MACHINE_SOFT_IRQ, 0, ipi_handler, NULL, 0);
	irq_enable(RISCV_MACHINE_SOFT_IRQ);

	return 0;
}

SYS_INIT(riscv_smp_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_SMP */
