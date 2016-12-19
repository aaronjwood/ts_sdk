# Copyright(C) 2016 Verizon. All rights reserved.

# Get platform-specific settings
# This must also define variables needed to locate the toolchain.
ifeq ($(BUILD_TARGET),osx)
include $(PROJ_ROOT)/common_osx.mk
else ifeq ($(BUILD_TARGET),stm32f4)
include $(PROJ_ROOT)/common_stm32f4.mk
else
	$(error "Makefile must define the BUILD_TARGET variable")
endif

# Defines which modem to use. Currently only ublox-toby201 is supported.
MODEM_TARGET ?= toby201

# Defines which cloud protocol to use. Currently only OTT is supported.
CLOUD_PROTOCOL ?= ott

# Debug and optimization flags
DBG_OP_USER_FLAGS ?= -g -ggdb3 -O0
DBG_OP_LIB_FLAGS ?= -Os

# Firmware name
FW_EXEC = firmware.elf
LDFLAGS ?= -Wl,-Map,fw.map,--cref

# Header files for per-platform standard "device drivers".
export INC
INC += -I $(PROJ_ROOT)/include/platform
INC += -I $(PROJ_ROOT)/include/dbg
INC += -I $(PROJ_ROOT)/include/uart
INC += -I $(PROJ_ROOT)/include/hwrng

# Platform related headers (beyond standard device drivers)
INC += $(PLATFORM_INC)

# Header files for certificates
INC += -I $(PROJ_ROOT)/include/certs

# Header files for device credentials
INC += -I $(PROJ_ROOT)/include/dev_creds

# Header files for vendor libraries
INC += $(VENDOR_INC)

ifeq ($(MODEM_TARGET),toby201)
MODEM_SRC += at_toby201.c
MODEM_DIR += toby201
endif

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers and other platform
# support files.
CORELIB_SRC = dbg_$(BUILD_TARGET).c uart_$(BUILD_TARGET).c
CORELIB_SRC += hwrng_$(BUILD_TARGET).c platform_$(BUILD_TARGET).c
CORELIB_SRC +=  $(PLATFORM_SRC)

# Search paths for device driver sources
vpath %.c $(PROJ_ROOT)/lib/platform: \
	$(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/lib/uart: \
	$(PROJ_ROOT)/lib/hwrng:

include $(PROJ_ROOT)/protocol.mk
include $(PROJ_ROOT)/net.mk

