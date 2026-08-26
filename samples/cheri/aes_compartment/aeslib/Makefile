#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# Copyright (c) 2026 University of Birmingham
#
# SPDX-License-Identifier: Apache-2.0
#

PREFIX ?= .
OBJ_DIR ?= $(PREFIX)/obj
LIB_DIR ?= $(PREFIX)/lib
MARCH ?=
MABI ?=

ifeq ($(findstring Windows, $(OS)),Windows)
	DEL := rmdir /S /Q
	MKDIR := mkdir
else
	DEL := rm -rf
	MKDIR := mkdir -p
endif

all:
	-$(MKDIR) "$(OBJ_DIR)"
	-$(MKDIR) "$(LIB_DIR)"
	$(CC) -c $(CFLAGS) -march=$(MARCH) -mabi=$(MABI) -MD -Iinclude src/aeslib.c -o $(OBJ_DIR)/aeslib.o
	$(CC) -c $(CFLAGS) -march=$(MARCH) -mabi=$(MABI) -MD -Iinclude src/tables.s -o $(OBJ_DIR)/tables.o
	$(AR) -rcs $(LIB_DIR)/lib_chericompartment1_aeslib.a $(OBJ_DIR)/aeslib.o $(OBJ_DIR)/tables.o

clean:
	$(DEL) "$(OBJ_DIR)" "$(LIB_DIR)"
