# Copyright(C) 2017 Verizon. All rights reserved.

# Makefile for to generate static library for the chipset sdk
BUILD_MK_PATH = $(CURDIR)

export CHIPSET_ROOT
export CHIPSET_BUILDENV

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(BUILD_MK_PATH)/build_in_subdir.mk
else

CHIPSET_ROOT = $(shell dirname $(SRCDIR))
CHIPSET_BUILDENV = $(shell dirname $(CHIPSET_ROOT))ts_sdk_bldenv


# Define this macro to turn off debug messages globally.
DBG_MACRO = #-DNO_DEBUG

# Library sources are built without debug info and optimized for size.
# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

include $(CHIPSET_BUILDENV)/stm32l4/common_stm32l4.mk

# Defines the target rules
include $(SRCDIR)/rules.mk

endif
