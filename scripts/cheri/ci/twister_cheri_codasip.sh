#!/bin/sh

#
# Copyright (c) 2026 University of Birmingham
#
# SPDX-License-Identifier: Apache-2.0
#


# This is a CHERI specific test script.
# It can only be run with the codasip llvm-cheri toolchain,
#  where:
#  ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
#  LLVM_CHERI_TOOLCHAIN_PATH=<path-to-llvm-cheri-toolchain>
#  QEMU_BIN_PATH=<path-to-cheri-qemu>
# It can be run locally or by a CHERI specific workflow.
# It runs a set of twister tests that pass for CHERI-based Zephyr.
# It supports the following boards:
#  qemu_riscv64cheri
#  qemu_riscv64cheri_smp
#  qemu_riscv64cheri_zcheripurecap
#  qemu_riscv64cheri_smp_zcheripurecap
#  qemu_riscv32cheri
#  qemu_riscv32cheri_smp
#  qemu_riscv32cheri_zcheripurecap
#  qemu_riscv32cheri_smp_zcheripurecap

#SAMPLES
#64bit
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri -p qemu_riscv64cheri_smp \
-p qemu_riscv64cheri_zcheripurecap -p qemu_riscv64cheri_smp_zcheripurecap \
-T samples/hello_world -T samples/synchronization \
-T samples/philosophers -T samples/basic/sys_heap \
-T samples/cheri \
-v --short-build-path -O/tmp/twister-out

#32bit
./scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri -p qemu_riscv32cheri_smp \
-p qemu_riscv32cheri_zcheripurecap -p qemu_riscv32cheri_smp_zcheripurecap \
-T samples/hello_world -T samples/synchronization \
-T samples/philosophers -T samples/basic/sys_heap \
-T samples/cheri \
-v --short-build-path -O/tmp/twister-out

#TESTS
#without PMP enabled in zcheripurecap
#64bit - WITH and WITHOUT smp
./scripts/twister --retry-failed 1 --timeout-multiplier 2 \
--runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri -p qemu_riscv64cheri_smp \
-p qemu_riscv64cheri_zcheripurecap -p qemu_riscv64cheri_smp_zcheripurecap \
-s tests/lib/heap/libraries.heap \
-s tests/cheri/lib/heap/cheri.libraries.sys_heap \
-s tests/cheri/lib/heap_sys_heap/cheri.libraries.sys_heap_kheap \
-s tests/lib/heap_align/libraries.heap_align \
-s tests/cheri/lib/heap_align/cheri.libraries.heap_align \
-s tests/lib/multi_heap/libraries.multi_heap \
-s tests/lib/multi_heap/libraries.multi_heap.no_mt \
-s tests/lib/multi_heap/libraries.multi_heap.cheri \
-s tests/cheri/lib/multi_heap/cheri.libraries.multi_heap \
-s tests/cheri/lib/multi_heap/cheri.libraries.multi_heap.no_mt \
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
-s tests/cheri/lib/mem_blocks/cheri.libraries.mem_blocks \
-s tests/kernel/threads/thread_stack/kernel.threads.thread_stack \
-s tests/cheri/kernel/stack/cheri.kernel.stack \
-s tests/kernel/mem_heap/k_heap_api \
-s tests/kernel/threads/thread_apis/kernel.threads.apis \
-s tests/kernel/threads/tls/kernel.threads.tls.userspace \
-s tests/kernel/msgq/msgq_usage/kernel.message_queue.usage \
-s tests/kernel/msgq/msgq_api/kernel.message_queue \
-s tests/cheri/lib/c_lib/common/cheri.libraries.libc.common \
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
./scripts/twister --retry-failed 1 --timeout-multiplier 2 \
--runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri -p qemu_riscv32cheri_zcheripurecap \
-p qemu_riscv32cheri_smp -p qemu_riscv32cheri_smp_zcheripurecap \
-s tests/lib/heap/libraries.heap \
-s tests/cheri/lib/heap/cheri.libraries.sys_heap \
-s tests/cheri/lib/heap_sys_heap/cheri.libraries.sys_heap_kheap \
-s tests/lib/heap_align/libraries.heap_align \
-s tests/cheri/lib/heap_align/cheri.libraries.heap_align \
-s tests/kernel/mem_heap/k_heap_api/kernel.k_heap_api \
-s tests/cheri/kernel/mem_heap/k_heap_api/cheri.kernel.k_heap_api \
-s tests/lib/multi_heap/libraries.multi_heap \
-s tests/lib/multi_heap/libraries.multi_heap.no_mt \
-s tests/lib/multi_heap/libraries.multi_heap.cheri \
-s tests/cheri/lib/multi_heap/cheri.libraries.multi_heap \
-s tests/cheri/lib/multi_heap/cheri.libraries.multi_heap.no_mt \
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
-s tests/cheri/lib/mem_blocks/cheri.libraries.mem_blocks \
-s tests/kernel/threads/thread_stack/kernel.threads.thread_stack \
-s tests/cheri/kernel/stack/cheri.kernel.stack \
-s tests/kernel/mem_heap/k_heap_api \
-s tests/kernel/threads/thread_apis/kernel.threads.apis \
-s tests/kernel/threads/tls/kernel.threads.tls.userspace \
-s tests/kernel/msgq/msgq_usage/kernel.message_queue.usage \
-s tests/kernel/msgq/msgq_api/kernel.message_queue \
-s tests/cheri/lib/c_lib/common/cheri.libraries.libc.common \
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
