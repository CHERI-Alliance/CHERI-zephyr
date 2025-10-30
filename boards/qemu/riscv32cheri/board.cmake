# SPDX-License-Identifier: Apache-2.0
# Modified for CHERI 2025 by University of Birmingham

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv32cheri)
set(QEMU_CPU_TYPE_${ARCH} riscv32cheri)


set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  )
if (CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI)
list(APPEND QEMU_FLAGS_${ARCH}
-cpu rv32,Xcheri_purecap=on,cheri_v090=on,cheri_levels=2,m_flip=on
)
endif()
board_set_debugger_ifnset(qemu)
