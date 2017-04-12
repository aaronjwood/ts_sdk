# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Makefile for the AT TCP driver test program.

APP_ROOT = $(shell dirname $$PWD)
BUILD_MK_PATH = $(shell dirname $(APP_ROOT))/tools/config

export PROJ_ROOT
export APP_ROOT
export CHIPSET_BUILDENV

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(BUILD_MK_PATH)/build_in_subdir.mk
else

APP_ROOT = $(shell dirname $(SRCDIR))
PROJ_ROOT = $(shell dirname $(APP_ROOT))
CHIPSET_BUILDENV = $(shell dirname $(PROJ_ROOT))ts_sdk_bldenv

BUILD_TARGET ?= stm32f4
export BUILD_TARGET

# This low-level test program bypasses the protocol layer and above.
# This requires overriding some of the normal configuration.
# Must always build with NO_PROTOCOL even if PROTOCOL is set externally.
override PROTOCOL = NO_PROTOCOL
MODEM_PROTOCOL = tcp
CLOUD_COMM_SRC =
SERVICES_SRC =

# Define this macro to turn off debug messages globally.
DBG_MACRO = #-DNO_DEBUG

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR):

# User application includes
INC += -I $(SRCDIR)/include

# User application sources
USER_SRC = $(wildcard $(SRCDIR)/*.c)

# Library sources are built without debug info and optimized for size.
# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

# Selecting platform according to BUILD_TARGET
include $(CHIPSET_BUILDENV)/stm32f4/common_stm32f4.mk

# chipset device drivers, provided by common_xxxx.mk from above
INC += $(CHIPSET_INC)

# Common and per-platform Makefile variables
include $(APP_ROOT)/common.mk

# Defines the target rules
include $(APP_ROOT)/rules.mk

endif
