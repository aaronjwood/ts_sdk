# Copyright(C) 2017 Verizon. All rights reserved.

# Makefile for i2c driver test program.


ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(MK_HELPER_PATH)/build_in_subdir.mk
else

# This low-level test program bypasses the protocol layer and above.
# This requires overriding some of the normal configuration.
# Must always build with NO_PROTOCOL even if PROTOCOL is set externally.
override PROTOCOL = NO_PROTOCOL
override MODEM_PROTOCOL = none
override MODEM_TARGET = none
override SDK_SRC =

# Define this macro to turn off debug messages globally.
DBG_MACRO = #-DNO_DEBUG

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR):

# User application includes
APP_INC = 
APP_INC += -I $(PROJ_ROOT)/module_tests/inc/

# User application sources
APP_SRC = $(wildcard $(SRCDIR)/*.c)

# Library sources are built without debug info and optimized for size.
# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

# Common and per-platform Makefile variables
include $(MK_HELPER_PATH)/common.mk

endif
