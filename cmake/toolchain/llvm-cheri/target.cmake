# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

message(STATUS, "toolchain/llvm-cheri/target.cmake..............")

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld-cheri)#not yet present
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld-cheri)
endif()

#ARCH
#qemu_x86, ARCH=x86, triple=i686-pc-none-elf
#qemu_x86_64, ARCH=x86, triple=x86_64-pc-none-elf
#qemu_cortex_m0, ARCH=arm, triple=arm-none-eabi
#qemu_cortex_a53, ARCH=arm64, triple=aarch64-none-elf
message(STATUS, "ARCH: ${ARCH}")

if("${ARCH}" STREQUAL "arm64")
  set(triple aarch64-none-elf)
elseif("${ARCH}" STREQUAL "arm")
  set(triple arm-none-eabi)
  set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
elseif("${ARCH}" STREQUAL "riscv")
  if(CONFIG_64BIT)
    set(triple riscv64-unknown-elf)
  else()
    set(triple riscv32-unknown-elf)
  endif()
elseif("${ARCH}" STREQUAL "x86")
  if(CONFIG_64BIT)
    set(triple x86_64-pc-none-elf)
  else()
    set(triple i686-pc-none-elf)
  endif()
endif()

if(DEFINED triple)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

  message(STATUS, "FindTargetTools LINKER2b: ${LINKER}")

  #unset(triple) #don't unset otherwise defaults to host

  message(STATUS, "CMAKE_C_COMPILER_TARGET: ${CMAKE_C_COMPILER_TARGET}")
  message(STATUS, "CMAKE_ASM_COMPILER_TARGET: ${CMAKE_ASM_COMPILER_TARGET}")
  message(STATUS, "CMAKE_CXX_COMPILER_TARGET: ${CMAKE_CXX_COMPILER_TARGET}")
  message(STATUS, "LINKER: ${LINKER}")

endif()
