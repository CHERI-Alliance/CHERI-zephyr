# SPDX-License-Identifier: Apache-2.0
# Modified for CHERI 2024 by University of Birmingham

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv64cheri)
set(QEMU_CPU_TYPE_${ARCH} riscv64cheri)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  )
board_set_debugger_ifnset(qemu)
