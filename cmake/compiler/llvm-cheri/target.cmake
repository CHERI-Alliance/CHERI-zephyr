# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

# Configuration for host installed clang
#
message(STATUS, "llvm-cheri toolchain....compiler target.cmake")
set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

message(STATUS, "find_program_clang_args: ${find_program_clang_args}")
message(STATUS, "ARCH: ${ARCH}")

find_program(CMAKE_C_COMPILER   clang   ${find_program_clang_args})
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_clang_args})

if(NOT "${ARCH}" STREQUAL "posix")
  include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)

  if("${ARCH}" STREQUAL "arm")
    list(APPEND TOOLCHAIN_C_FLAGS
      -fshort-enums
      )
    list(APPEND TOOLCHAIN_LD_FLAGS
      -fshort-enums
      )
    message(STATUS, "arm found")
    include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arm.cmake)
#  endif()
  #add arm64 for aarch64
  elseif("${ARCH}" STREQUAL "arm64")
    list(APPEND TOOLCHAIN_C_FLAGS "--target=${triple}")
    list(APPEND TOOLCHAIN_LD_FLAGS "--target=${triple}")
    message(STATUS, "arm64 found")
#  endif()
    #add riscv for riscv32/64
    #-mno-relax needed to turn off riscv code optimisation by the linker which isn't present in version 14.0.0 version of llvm
    #when -mno-relax used only get error: relocation R_RISCV_HI20 out of range:
    #so also include -mcmodel=medany to tell the compiler to use auipc/jalr pair when jumping, which gives you a larger range of ±2GB
    elseif("${ARCH}" STREQUAL "riscv")

    	if(CONFIG_CHERI)
    	        #if compiling for riscv64 CHERI-PURECAP
    		if(CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI)
    		#CHERI-PURECAP new RISC-V spec v0.9.5
    		message(STATUS, "Compiling for riscv64 CHERI-PURECAP new RISC-V spec v0.9.5")
    		string(PREPEND CMAKE_ASM_FLAGS "-march=rv64imafdzcheripurecap -mabi=l64pc128d ")
		string(PREPEND CMAKE_C_FLAGS   "-march=rv64imafdzcheripurecap -mabi=l64pc128d ")
		string(PREPEND CMAKE_CXX_FLAGS "-march=rv64imafdzcheripurecap -mabi=l64pc128d ")
    		else()
    		#CHERI-PURECAP Cambs spec v8.0
    		message(STATUS, "Compiling for riscv64 CHERI-PURECAP Cambs spec v8.0")
		string(PREPEND CMAKE_ASM_FLAGS "-march=rv64gcxcheri -mabi=l64pc128d ")
		string(PREPEND CMAKE_C_FLAGS   "-march=rv64gcxcheri -mabi=l64pc128d ")
		string(PREPEND CMAKE_CXX_FLAGS "-march=rv64gcxcheri -mabi=l64pc128d ")
		endif()
    	else()
    		#if compiling for normal riscv64
    		#-march=rv64gc gc adds the floating point option and others
    		message(STATUS, "Compiling for riscv64 only")
		string(PREPEND CMAKE_ASM_FLAGS "-march=rv64gc ")
		string(PREPEND CMAKE_C_FLAGS   "-march=rv64gc ")
		string(PREPEND CMAKE_CXX_FLAGS "-march=rv64gc ")
	endif()

    list(APPEND TOOLCHAIN_C_FLAGS "--target=${triple}")
    list(APPEND TOOLCHAIN_LD_FLAGS "--target=${triple}")
    list(APPEND TOOLCHAIN_C_FLAGS "-mno-relax")
    list(APPEND TOOLCHAIN_LD_FLAGS "-mno-relax")
    list(APPEND TOOLCHAIN_C_FLAGS "-mcmodel=medany")
    list(APPEND TOOLCHAIN_LD_FLAGS "-mcmodel=medany")
    message(STATUS, "riscv found")
    else()
    message(STATUS, "default arch found")
  endif()

message(STATUS, "triple : ${triple}")
message(STATUS, "ARCH append: ${ARCH}")
message(STATUS, "TOOLCHAIN_C_FLAGS append: ${TOOLCHAIN_C_FLAGS}")
message(STATUS, "TOOLCHAIN_LD_FLAGS append: ${TOOLCHAIN_LD_FLAGS}")

  foreach(file_name include/stddef.h)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
      OUTPUT_VARIABLE _OUTPUT
      )
    get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
    string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

    list(APPEND NOSTDINC ${_OUTPUT})
  endforeach()

  foreach(isystem_include_dir ${NOSTDINC})
    list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
  endforeach()

  if(CONFIG_X86)
    if(CONFIG_64BIT)
      list(APPEND TOOLCHAIN_C_FLAGS "-m64")
    else()
      list(APPEND TOOLCHAIN_C_FLAGS "-m32")
    endif()
  endif()

#-----------------------------------
#llvm-cheri does not point to the correct runtime library directory when using --print-libgcc-file-name command to find the runtime libraries so we need a work-a-round
  # This libgcc code is partially duplicated in compiler/*/target.cmake
#  execute_process(
#    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-libgcc-file-name
#    OUTPUT_VARIABLE LIBGCC_FILE_NAME
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#    )

#  get_filename_component(LIBGCC_DIR ${LIBGCC_FILE_NAME} DIRECTORY)

#  list(APPEND LIB_INCLUDE_DIR "-L\"${LIBGCC_DIR}\"")
#  if(LIBGCC_DIR)
#    #list(APPEND TOOLCHAIN_LIBS gcc)
#  endif()

#-------------------------------------------------
     execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_FILE_NAME_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  #get only the name of the file
  get_filename_component(LIBGCC_FILE_NAME_ONLY ${LIBGCC_FILE_NAME_DIR} NAME)

  #get only the name of the directory, then, create the full path
  if("${triple}" STREQUAL "riscv32")
  set(LIBGCC_DIR ${TOOLCHAIN_HOME}../baremetal/baremetal-riscv32/lib/)
  set(LIBGCC_FILE_NAME ${LIBGCC_DIR}${LIBGCC_FILE_NAME_ONLY})
  list(APPEND LIB_INCLUDE_DIR -L${LIBGCC_DIR})
  list(APPEND TOOLCHAIN_LIBS gcc) # seems to only find the libs if include this, works because of the libcc symlink
  elseif("${triple}" STREQUAL "riscv64")
  set(LIBGCC_DIR ${TOOLCHAIN_HOME}../baremetal/baremetal-riscv64/lib/)
  set(LIBGCC_FILE_NAME ${LIBGCC_DIR}${LIBGCC_FILE_NAME_ONLY})
  list(APPEND LIB_INCLUDE_DIR -L${LIBGCC_DIR})
  #list(APPEND TOOLCHAIN_LIBS gcc)
  #for running on morello runtime libs are in morello-sdk
  elseif("${triple}" STREQUAL "aarch64-none-elf")
  set(LIBGCC_DIR ${TOOLCHAIN_HOME}../../morello-sdk/baremetal/baremetal-morello-aarch64/aarch64-unknown-elf/lib/)
   # set(LIBGCC_DIR ${TOOLCHAIN_HOME}baremetal/baremetal-morello-aarch64/aarch64-unknown-elf/lib/)
  set(LIBGCC_FILE_NAME ${LIBGCC_DIR}${LIBGCC_FILE_NAME_ONLY})
  list(APPEND LIB_INCLUDE_DIR -L${LIBGCC_DIR})
  #need to explicitly pass lib directory to linker for morello-sdk clang (v13.0) because can't find runtime lib otherwise.
  list(APPEND CMAKE_EXE_LINKER_FLAGS "-Xlinker -L${LIBGCC_DIR}")
  list(APPEND TOOLCHAIN_LIBS gcc)
  else()
  message(STATUS, "!! TRIPLE NOT FOUND !!")
  endif()
#---------------------------------------------------


  list(APPEND CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

endif()

message(STATUS, "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS, "TOOLCHAIN_C_FLAGS: ${TOOLCHAIN_C_FLAGS}")
message(STATUS, "LIBGCC_DIR: ${LIBGCC_DIR}")
message(STATUS, "LIBGCC_FILE_NAME: ${LIBGCC_FILE_NAME}")
message(STATUS, "find_program_clang_args: ${find_program_clang_args}")
message(STATUS, "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS, "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS, "CMAKE_LLVM_COV: ${CMAKE_LLVM_COV}")
message(STATUS, "TOOLCHAIN_LIBS: ${TOOLCHAIN_LIBS}")

# Load toolchain_cc-family macros

macro(toolchain_cc_nostdinc)
  if(NOT "${ARCH}" STREQUAL "posix")
    zephyr_compile_options( -nostdinc)
  endif()
endmacro()
