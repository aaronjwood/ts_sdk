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
INC += -I $(PROJ_ROOT)/include/platform
INC += -I $(PROJ_ROOT)/include/certs
INC += -I $(PROJ_ROOT)/include/dev_creds

# mbedtls library header files
INC += -I $(PROJ_ROOT)/vendor/mbedtls/include

# OTT protocol API header
INC += -I $(PROJ_ROOT)/include/ott_protocol/

export INC

CORELIB_SRC = dbg_osx.c osx_platform.c

# Cloud communication / OTT protocol API sources
CLOUD_COMM_SRC = ott_protocol.c

# Search paths for core module sources
vpath %.c $(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/lib/platform: \
	$(PROJ_ROOT)/lib/ott_protocol:
