# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(msgq_usage)

if(CONFIG_CHERI)
    target_sources(app PRIVATE src/msgq_usage_cheri.c)
else()
    target_sources(app PRIVATE src/main.c)
endif()
target_sources(app PRIVATE ${app_sources})
