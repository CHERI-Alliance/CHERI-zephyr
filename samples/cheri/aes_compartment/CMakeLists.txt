# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aes_compartment)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/compartment_entry.c)
target_sources(app PRIVATE src/attack_entry.c)

zephyr_linker_sources(SECTIONS linker/linker_compartments.ld)

# Pass TEST_NO var from sample.yaml into tests as preprocess var
if(DEFINED TEST_NO)
    zephyr_compile_definitions(TEST_NO=${TEST_NO})
endif()

# The external static library that we are linking with does not know
# how to build for this platform so we export all the flags used in
# this zephyr build to the external build system.
#
# Other external build systems may be self-contained enough that they
# do not need any build information from zephyr. Or they may be
# incompatible with certain zephyr options and need them to be
# filtered out.
zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

if(DEFINED CMAKE_C_COMPILER_TARGET)
  set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
endif()

#set(set_captable_scope_file "-Wl,-captable-scope=file")

set(external_project_cflags
    #"${target_flag} ${set_captable_scope_file} ${includes} ${definitions} ${options} ${system_includes}"
  "${target_flag} ${includes} ${definitions} ${options} ${system_includes}"
  )



# Create variables for mabi/march to pass on to library's makefile
if(CONFIG_64BIT)
    set(AESLIB_MARCH rv64imafdzcheripurecap)
    set(AESLIB_MABI l64pc128d)
else()
    set(AESLIB_MARCH rv32imafdzcheripurecap)
    #set(AESLIB_MABI il32pc64d)
    set(AESLIB_MABI il32pc64)
endif()

include(ExternalProject)

# Add an external project to be able download and build the third
# party library. In this case downloading is not necessary as it has
# been committed to the repository.
set(aeslib_src_dir   ${CMAKE_CURRENT_SOURCE_DIR}/aeslib)
set(aeslib_build_dir ${CMAKE_CURRENT_BINARY_DIR}/aeslib)

set(AESLIB_LIB_DIR     ${aeslib_build_dir}/lib)
set(AESLIB_INCLUDE_DIR ${aeslib_src_dir}/include)

if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
# https://www.gnu.org/software/make/manual/html_node/MAKE-Variable.html
set(submake "$(MAKE)")
else() # Obviously no MAKEFLAGS. Let's hope a "make" can be found somewhere.
set(submake "make")
endif()

# This setting breaks the delicate linker maniuplations required to manually set
# up compartments
STRING(REGEX REPLACE "-msmall-data-limit=0" "" external_project_cflags ${external_project_cflags})

ExternalProject_Add(
  aeslib_project                # Name for custom target
  PREFIX     ${aeslib_build_dir} # Root dir for entire project
  SOURCE_DIR ${aeslib_src_dir}
  BINARY_DIR ${aeslib_src_dir} # This particular build system is invoked from the root
  CONFIGURE_COMMAND ""    # Skip configuring the project, e.g. with autoconf
  BUILD_COMMAND
  ${submake}
  PREFIX=${aeslib_build_dir}
  CC=${CMAKE_C_COMPILER}
  AR=${CMAKE_AR}
  # pass appropriate MARCH/MABI
  MARCH=${AESLIB_MARCH}
  MABI=${AESLIB_MABI}
  CFLAGS=${external_project_cflags}
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS ${AESLIB_LIB_DIR}/lib_chericompartment1_aeslib.a
  )

# Create a wrapper CMake library that our app can link with
add_library(aeslib_lib STATIC IMPORTED GLOBAL)
add_dependencies(
  aeslib_lib
  aeslib_project
  )
set_target_properties(aeslib_lib PROPERTIES IMPORTED_LOCATION
    ${AESLIB_LIB_DIR}/lib_chericompartment1_aeslib.a)
set_target_properties(aeslib_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${AESLIB_INCLUDE_DIR})

target_link_libraries(app PUBLIC aeslib_lib)
