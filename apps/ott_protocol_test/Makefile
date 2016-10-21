# Copyright(C) 2016 Verizon. All rights reserved.

# Makefile for the OTT protocol test program for the STM32F4 platform.
# Uses the GCC toolchain and the STMicro STM32CubeF4 HAL drivers.
# Note: Library sources are built without debug info and optimized for size

PROJ_ROOT = $(shell git rev-parse --show-toplevel)
export PROJ_ROOT

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(PROJ_ROOT)/build_in_subdir.mk
else

# Defines toolchain and common modules & paths
# Debug, UART and HW RNG drivers are included by default
include $(PROJ_ROOT)/common.mk

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

# Build the mbed TLS library
VENDOR_LIB_DIRS = mbedtls
VENDOR_LIB_FLAGS = -L. -lmbedtls -lmbedx509 -lmbedcrypto

# Defines the target rules
include $(PROJ_ROOT)/rules.mk

endif
