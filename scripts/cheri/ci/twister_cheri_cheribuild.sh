#!/bin/sh

#
# Copyright (c) 2026 University of Birmingham
#
# SPDX-License-Identifier: Apache-2.0
#


# This is a CHERI specific test script.
# It can only be run with the cheribuild llvm-cheri toolchain,
#  where:
#  ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
#  LLVM_CHERI_TOOLCHAIN_PATH=<path-to-llvm-cheri-toolchain>
#  QEMU_BIN_PATH=<path-to-cheri-qemu>
# It can be run locally or by a CHERI specific workflow.
# It runs a set of twister tests that pass for CHERI-based Zephyr.
# It supports the following boards:
#  qemu_riscv64cheri
#  qemu_riscv64cheri_smp
#  qemu_riscv64cheri_purecap
#  qemu_riscv64cheri_smp_purecap
#  qemu_riscv32cheri
#  qemu_riscv32cheri_smp
#  qemu_riscv32cheri_purecap
#  qemu_riscv32cheri_smp_purecap

#SAMPLES
#64bit
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri -p qemu_riscv64cheri_smp \
-p qemu_riscv64cheri_purecap -p qemu_riscv64cheri_smp_purecap \
-T samples/hello_world -T samples/synchronization \
-T samples/philosophers -T samples/basic/sys_heap \
-v --short-build-path -O/tmp/twister-out

#32bit
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri -p qemu_riscv32cheri_smp \
-p qemu_riscv32cheri_purecap -p qemu_riscv32cheri_smp_purecap \
-T samples/hello_world -T samples/synchronization \
-T samples/philosophers -T samples/basic/sys_heap \
-v --short-build-path -O/tmp/twister-out

#TESTS
#without PMP enabled IN PURECAP
#64bit WITH and WITHOUT smp
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri -p qemu_riscv64cheri_purecap \
-p qemu_riscv64cheri_smp -p qemu_riscv64cheri_smp_purecap \
-s tests/lib/heap/libraries.heap \
-s tests/lib/heap_align/libraries.heap_align \
-s tests/kernel/mem_slab/mslab/kernel.memory_slabs \
-s tests/kernel/mem_slab/mslab_threadsafe/kernel.memory_slabs.threadsafe \
-s tests/kernel/mem_slab/mslab_api/kernel.memory_slabs.api \
-s tests/kernel/mem_slab/mslab_stats/kernel.memory_slabs.stats \
-s tests/kernel/mem_slab/mslab_concept/kernel.memory_slabs.concept \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds.relax \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds.reduced \
-s tests/lib/mem_blocks_stats/libraries.mem_blocks.stats \
-s tests/lib/mem_blocks/libraries.mem_blocks \
-s tests/kernel/mem_heap/k_heap_api \
-s tests/subsys/logging/log_output/logging.output \
-s tests/subsys/logging/log_output/logging.output.ts64 \
-s tests/subsys/logging/log_output/logging.output.ts64.date \
-s tests/subsys/logging/log_output/logging.output.ts64.iso8601 \
-s tests/subsys/logging/log_output/logging.output.thread_id \
-s tests/subsys/logging/log_timestamp/logging.output.default_timestamp \
-s tests/subsys/logging/log_timestamp/logging.output.custom_timestamp \
-s tests/subsys/logging/log_custom_header/logging.log_custom_header \
-s tests/subsys/logging/log_core_additional/logging.async \
-s tests/subsys/logging/log_core_additional/logging.sync \
-s tests/subsys/logging/log_cache/logging.cache \
-s tests/subsys/logging/log_benchmark/logging.benchmark \
-s tests/subsys/logging/log_benchmark/logging.benchmark_speed \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.input_limited \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.matched \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.stalled \
-s tests/subsys/logging/log_core_additional/logging.thread \
-s tests/subsys/logging/log_backend_uart/logging.backend.uart.multi \
-s tests/subsys/logging/log_backend_uart/logging.backend.uart.single \
-s tests/subsys/logging/log_frontend_stmesp_demux/debug.coresight.stp_demux \
-s tests/subsys/logging/log_frontend_stmesp_demux/debug.coresight.stp_demux_max_utilization \
-v --short-build-path -O/tmp/twister-out

#32bit WITH and WITHOUT smp
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri -p qemu_riscv32cheri_purecap \
-p qemu_riscv32cheri_smp -p qemu_riscv32cheri_smp_purecap \
-s tests/lib/heap/libraries.heap \
-s tests/lib/heap_align/libraries.heap_align \
-s tests/kernel/mem_slab/mslab/kernel.memory_slabs \
-s tests/kernel/mem_slab/mslab_concept/kernel.memory_slabs.concept \
-s tests/kernel/mem_slab/mslab_threadsafe/kernel.memory_slabs.threadsafe \
-s tests/kernel/mem_slab/mslab_api/kernel.memory_slabs.api \
-s tests/kernel/mem_slab/mslab_stats/kernel.memory_slabs.stats \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds.relax \
-s tests/cheri/kernel/mem_slab/mslab_slab_bounds/cheri.kernel.memory_slabs_bounds.reduced \
-s tests/lib/mem_blocks_stats/libraries.mem_blocks.stats \
-s tests/lib/mem_blocks/libraries.mem_blocks \
-s tests/kernel/mem_heap/k_heap_api \
-s tests/subsys/logging/log_output/logging.output \
-s tests/subsys/logging/log_output/logging.output.ts64 \
-s tests/subsys/logging/log_output/logging.output.ts64.date \
-s tests/subsys/logging/log_output/logging.output.ts64.iso8601 \
-s tests/subsys/logging/log_output/logging.output.thread_id \
-s tests/subsys/logging/log_timestamp/logging.output.default_timestamp \
-s tests/subsys/logging/log_timestamp/logging.output.custom_timestamp \
-s tests/subsys/logging/log_custom_header/logging.log_custom_header \
-s tests/subsys/logging/log_core_additional/logging.async \
-s tests/subsys/logging/log_core_additional/logging.sync \
-s tests/subsys/logging/log_cache/logging.cache \
-s tests/subsys/logging/log_benchmark/logging.benchmark \
-s tests/subsys/logging/log_benchmark/logging.benchmark_speed \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.input_limited \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.matched \
-s tests/subsys/logging/log_blocking/logging.blocking.rate.stalled \
-s tests/subsys/logging/log_core_additional/logging.thread \
-s tests/subsys/logging/log_backend_uart/logging.backend.uart.multi \
-s tests/subsys/logging/log_backend_uart/logging.backend.uart.single \
-s tests/subsys/logging/log_frontend_stmesp_demux/debug.coresight.stp_demux \
-s tests/subsys/logging/log_frontend_stmesp_demux/debug.coresight.stp_demux_max_utilization \
-v --short-build-path -O/tmp/twister-out
