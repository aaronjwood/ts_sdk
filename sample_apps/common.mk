# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Get platform-specific settings
# This must also define variables needed to locate the toolchain.

PLATFORM_HAL_ROOT = $(PROJ_ROOT)/platform
SDK_ROOT = $(PROJ_ROOT)/sdk/cloud_comm

export PLATFORM_HAL_ROOT
export SDK_ROOT

# Defines which modem to use. Currently only the ublox toby201 is supported.
MODEM_TARGET ?= toby201

# Defines which cloud protocol to use. Valid options are:
#  OTT_PROTOCOL
#  SMSNAS_PROTOCOL
# Override on the command line with: make PROTOCOL=<option>
PROTOCOL ?= OTT_PROTOCOL

# Debug and optimization flags
# Comment this out to disable LTO and enable debug
#LTOFLAG = -flto
#export LTOFLAG

# To reduce the firmware size at the expense of debugging capabilities in
# the user code, use the following instead:
#DBG_OP_USER_FLAGS = -Os $(LTOFLAG)
DBG_OP_USER_FLAGS = -g -ggdb3 -O0 $(LTOFLAG)

# By default, build the library modules for reduced size
DBG_OP_LIB_FLAGS = -Os $(LTOFLAG)

# Name of the resulting firmware image file
FW_EXEC = firmware.elf
LDFLAGS ?= -Wl,-Map,fw.map,--cref

# Header files for per-platform standard "device drivers".
export INC

include $(SDK_ROOT)/cc_sdk.mk
include $(PLATFORM_HAL_ROOT)/platform.mk

# SDK_SRC is provided by cc_sdk.mk
CORELIB_SRC += $(SDK_SRC)
# PLATFORM_HAL_SRC is provided by platform.mk
CORELIB_SRC += $(PLATFORM_HAL_SRC)
