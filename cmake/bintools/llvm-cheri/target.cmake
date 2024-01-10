# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

# Configures binary tools as llvm binary tool set

message(STATUS, "bintools/llvm-cheri..................")
message(STATUS, "LINKER: ${LINKER}")

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  #added NO_DEFAULT_PATH here to make sure points to llvm-cheri path and not usr/
  set(find_program_binutils_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

message(STATUS, "find_program_clang_args: ${find_program_clang_args}")
message(STATUS, "find_program_binutils_args: ${find_program_binutils_args}")

# Extract the clang version.  Debian (maybe other distros?) will
# append a version to llvm-objdump/objcopy
execute_process(COMMAND ${CMAKE_C_COMPILER} --version
                OUTPUT_VARIABLE CLANGVER)
string(REGEX REPLACE "[^0-9]*([0-9]+).*" "\\1" CLANGVER ${CLANGVER})

find_program(CMAKE_AR      llvm-ar      ${find_program_clang_args})
find_program(CMAKE_NM      llvm-nm      ${find_program_clang_args})
find_program(CMAKE_OBJDUMP NAMES
                           llvm-objdump
                           llvm-objdump-${CLANGVER}
                           ${find_program_clang_args})
find_program(CMAKE_RANLIB  llvm-ranlib  ${find_program_clang_args})
find_program(CMAKE_STRIP   llvm-strip   ${find_program_clang_args})
#aarch64-none-elf-objcopy supports --gap-fill for aarch64, llvm-objcopy does not support --gap-fill
#objcopy does support --gap-fill but can't process the aarch64 elf file so need a check in place to work out
#which one to use

if("${triple}" STREQUAL "aarch64-none-elf")
find_program(CMAKE_OBJCOPY NAMES
			   aarch64-none-elf-objcopy
                           llvm-objcopy
                           llvm-objcopy-${CLANGVER}
                           objcopy
                           PATHS
                           ~/morello/gnu-toolchain/x86_64-aarch64-none-elf/bin/
                           ${find_program_binutils_args})
else()
find_program(CMAKE_OBJCOPY NAMES
                           llvm-objcopy
                           llvm-objcopy-${CLANGVER}
                           objcopy
                           ${find_program_binutils_args})
endif()

#add llvm-readelf
find_program(CMAKE_READELF llvm-readelf
			   readelf
			   ${find_program_binutils_args})

message(STATUS, "CMAKE_OBJDUMP: ${CMAKE_OBJDUMP}")
message(STATUS, "CMAKE_OBJCOPY: ${CMAKE_OBJCOPY}")
message(STATUS, "CMAKE_READELF: ${CMAKE_READELF}")
message(STATUS, "find_program_clang_args: ${find_program_clang_args}")
message(STATUS, "find_program_binutils_args: ${find_program_binutils_args}")


# Use the gnu binutil abstraction
include(${ZEPHYR_BASE}/cmake/bintools/llvm-cheri/target_bintools.cmake)
