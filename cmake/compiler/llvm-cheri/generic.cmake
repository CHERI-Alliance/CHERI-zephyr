# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

message(STATUS, "llvm-cheri toolchain....compiler generic.cmake")
if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER clang ${find_program_clang_args} REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_clang_args})
find_program(CMAKE_LLVM_COV llvm-cov ${find_program_clang_args})
set(CMAKE_GCOV "${CMAKE_LLVM_COV} gcov")

message(STATUS, "find_program_clang_args: ${find_program_clang_args}")
message(STATUS, "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS, "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS, "CMAKE_LLVM_COV: ${CMAKE_LLVM_COV}")

