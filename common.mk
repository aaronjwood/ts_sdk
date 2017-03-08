# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Get platform-specific settings
# This must also define variables needed to locate the toolchain.
ifeq ($(BUILD_TARGET),osx)
include $(PROJ_ROOT)/common_osx.mk
else ifeq ($(BUILD_TARGET),stm32f4)
include $(PROJ_ROOT)/common_stm32f4.mk
else
$(error "Makefile must define the BUILD_TARGET variable")
endif

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
INC += -I $(PROJ_ROOT)/include/platform
INC += -I $(PROJ_ROOT)/include/dbg
INC += -I $(PROJ_ROOT)/include/uart
INC += -I $(PROJ_ROOT)/include/hwrng

# Platform related headers (beyond standard device drivers)
INC += $(PLATFORM_INC)

# Header files for the cloud_comm API
INC += -I $(PROJ_ROOT)/include/cloud_comm

# Header files for services
INC += -I $(PROJ_ROOT)/include/services

# Header files for certificates
INC += -I $(PROJ_ROOT)/include/certs

# Header files for device credentials
INC += -I $(PROJ_ROOT)/include/dev_creds

# Header files for vendor libraries
INC += $(VENDOR_INC)

# Source for the main cloud API
CLOUD_COMM_SRC ?= cloud_comm.c

# Source for the standard services.
# An application may append to this variable if it uses additional services.
SERVICES_SRC ?= cc_basic_service.c cc_control_service.c

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.
CORELIB_SRC = dbg_$(BUILD_TARGET).c uart_$(BUILD_TARGET).c
CORELIB_SRC += hwrng_$(BUILD_TARGET).c platform_$(BUILD_TARGET).c
CORELIB_SRC +=  $(MODEM_SRC) $(PROTOCOL_SRC) $(CLOUD_COMM_SRC) $(SERVICES_SRC)

# Search paths for device driver sources
vpath %.c $(PROJ_ROOT)/lib/platform: \
	$(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/lib/uart: \
	$(PROJ_ROOT)/lib/hwrng: \
	$(PROJ_ROOT)/lib/cloud_comm: \
	$(PROJ_ROOT)/lib/services: \
	$(PROJ_ROOT)/lib/

include $(PROJ_ROOT)/protocol.mk
include $(PROJ_ROOT)/modem.mk

