# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Makefile for the AT SMS driver test program.

PROJ_ROOT = $(shell git rev-parse --show-toplevel)
export PROJ_ROOT

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(PROJ_ROOT)/build_in_subdir.mk
else

BUILD_TARGET ?= stm32f4
export BUILD_TARGET

# This low-level test program bypasses the protocol layer and above.
# This requires overriding some of the normal configuration.
PROTOCOL = NO_PROTOCOL
MODEM_PROTOCOL = sms
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

# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

# Common and per-platform Makefile variables
include $(PROJ_ROOT)/common.mk

# Defines the target rules
include $(PROJ_ROOT)/rules.mk

endif
