# SPDX-License-Identifier: Apache-2.0
# Added to support CHERI 2023, University of Birmingham

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_base)
message(STATUS, "linker/lld-cheri...target_base...............")

  if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
    set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__LLD_LINKER_CMD__)
  endif()

  # TOOLCHAIN_LD_FLAGS comes from compiler/llvm-cheri/target.cmake
  # LINKERFLAGPREFIX comes from linker/lld-cheri/target.cmake
  zephyr_ld_options(
    ${TOOLCHAIN_LD_FLAGS}
  )

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--gc-sections
    ${LINKERFLAGPREFIX},--build-id=none
  )

  # Sort each input section by alignment.
  zephyr_ld_option_ifdef(
    CONFIG_LINKER_SORT_BY_ALIGNMENT
    ${LINKERFLAGPREFIX},--sort-section=alignment
  )

  if (NOT CONFIG_LINKER_USE_RELAX)
    zephyr_ld_options(
      ${LINKERFLAGPREFIX},--no-relax
    )
  endif()

  if(CONFIG_CPP)
    # LLVM lld complains:
    #   error: section: init_array is not contiguous with other relro sections
    #
    # So do not create RELRO program header.
    zephyr_link_libraries(
      -Wl,-z,norelro
    )
  endif()

  zephyr_link_libraries(
    --config ${ZEPHYR_BASE}/cmake/toolchain/llvm-cheri/clang.cfg
  )
endmacro()
