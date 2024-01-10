# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

# Purpose of the generic.cmake is to define a generic C compiler which can be
# used for devicetree pre-processing and other pre-processing tasks which must
# be performed before the target can be determined.

#check toolchain path exists and then set the toolchain home path
zephyr_get(LLVM_CHERI_TOOLCHAIN_PATH)
assert(LLVM_CHERI_TOOLCHAIN_PATH "LLVM-CHERI_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${LLVM_CHERI_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at LLVM_CHERI_TOOLCHAIN_PATH: '${LLVM_CHERI_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${LLVM_CHERI_TOOLCHAIN_PATH}/bin/)

#these are used by FindTargetTools.cmake to include directories
#Note compiler/clang/target.cmake is configured for 'host' installed clang so we need
#to modify for our toolchain clang
#STATUS,include : ~/zephyrproject/zephyr/cmake/compiler/llvm-cheri/target.cmake
#STATUS,include : ~/zephyrproject/zephyr/cmake/linker/lld-cheri/target.cmake
#STATUS,include : ~/zephyrproject/zephyr/cmake/bintools/bintools_template.cmake
#STATUS,include : ~/zephyrproject/zephyr/cmake/bintools/llvm-cheri/target.cmake
set(COMPILER llvm-cheri)
set(LINKER lld-cheri)
set(BINTOOLS llvm-cheri)

message(STATUS, "llvm-cheri toolchain....")
message(STATUS, "COMPILER: ${COMPILER}")
message(STATUS, "LINKER: ${LINKER}")
message(STATUS, "BINTOOLS: ${BINTOOLS}")

#linker - see Kconfig for default linker
set(LD_SEARCH_PATH ${TOOLCHAIN_HOME})
set(LLVMLLD_LINKER ${TOOLCHAIN_HOME})

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

list(APPEND TOOLCHAIN_C_FLAGS --config ${ZEPHYR_BASE}/cmake/toolchain/llvm-cheri/clang.cfg)
list(APPEND TOOLCHAIN_LD_FLAGS --config ${ZEPHYR_BASE}/cmake/toolchain/llvm-cheri/clang.cfg)

message(STATUS "Found toolchain: llvm-cheri (clang/ld)")

message(STATUS, "CLANG_ROOT_DIR: ${CLANG_ROOT_DIR}")
message(STATUS, "TOOLCHAIN_HOME: ${TOOLCHAIN_HOME}")
message(STATUS, "LLVM_TOOLCHAIN_PATH: ${LLVM_TOOLCHAIN_PATH}")
message(STATUS, "TOOLCHAIN_C_FLAGS: ${TOOLCHAIN_C_FLAGS}")
message(STATUS, "TOOLCHAIN_LD_FLAGS: ${TOOLCHAIN_LD_FLAGS}")


