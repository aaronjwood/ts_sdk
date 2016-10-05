# Copyright(C) 2016 Verizon. All rights reserved.

# Define common parameters used by the build process.
# This is used only for building test code that can also run on OS X

# Dummy out the size command
SIZE = /usr/bin/true

DBG_OP_USER_FLAGS = -g -ggdb3 -O0

ARCHFLAGS = -DBUILD_TARGET_OSX

# Firmware name
FW_EXEC = firmware.elf
LDFLAGS = -Wl,-map,fw.map

INC = -I $(PROJ_ROOT)/include/dbg
INC += -I $(PROJ_ROOT)/include/certs

export INC

CORELIB_SRC = dbg_osx.c

# Search paths for core module sources
vpath %.c $(PROJ_ROOT)/lib/dbg:
