# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hobgoblin_uarts)

target_sources(app PRIVATE src/main.c)

if(CHERI_TEST_UART_INTERRUPTS)
  target_compile_definitions(app PRIVATE CHERI_TEST_UART_INTERRUPTS)
endif()
