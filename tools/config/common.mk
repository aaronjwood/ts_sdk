# Copyright(C) 2016,2017 Verizon. All rights reserved.

CHIPSET_FAMILY ?= $(CHIPSET_FAMILY)
CHIPSET_MCU ?= $(CHIPSET_MCU)
DEV_BOARD ?= $(DEV_BOARD)
# Defines which modem to use. Currently only the ublox toby201 is supported.
MODEM_TARGET ?= toby201

# Defines which cloud protocol to use. Valid options are:
# OTT_PROTOCOL
# SMSNAS_PROTOCOL
# NO_PROTOCOL
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
FW_EXEC = firmware_$(PROTOCOL)_$(DEV_BOARD).elf
LDFLAGS ?= -Wl,-Map,fw.map,--cref

# Include application header include
INC += $(APP_INC)

# Selecting platform according to CHIPSET_FAMILY
ifeq ($(CHIPSET_FAMILY),stm32f4)
include $(MK_HELPER_PATH)/stm32f4_buildenv.mk
else ifeq ($(CHIPSET_FAMILY),stm32l4)
include $(MK_HELPER_PATH)/stm32l4_buildenv.mk
else ifeq ($(CHIPSET_FAMILY),osx)
include $(MK_HELPER_PATH)/osx_buildenv.mk
else ifeq ($(DEV_BOARD),raspberry_pi3)
include $(MK_HELPER_PATH)/raspberry_pi3_buildenv.mk
else
$(error "Makefile must define valid DEV_BOARD variable")
endif
else
$(error "Makefile must define the CHIPSET_FAMILY variable")
endif

# Header files provided by the chipset build environment
INC += $(CHIPSET_INC)

include $(SDK_ROOT)/cc_sdk.mk
INC += $(SDK_INC)

include $(PLATFORM_HAL_ROOT)/platform.mk
INC += $(PLATFORM_INC)

# SDK_SRC is provided by cc_sdk.mk
CORELIB_SRC += $(SDK_SRC)

# PLATFORM_HAL_SRC is provided by platform.mk
CORELIB_SRC += $(PLATFORM_HAL_SRC)

export INC

# Defines the target rules
include $(MK_HELPER_PATH)/rules.mk
