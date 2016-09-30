# Copyright(C) 2016 Verizon. All rights reserved.

# Makefile for the OTT protocol test program for the STM32F4 platform.
# Uses the GCC toolchain and the STMicro STM32CubeF4 HAL drivers.
# Note: Library sources are built without debug info and optimized for size

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include build_in_subdir.mk
else

PROJ_ROOT = $(shell git rev-parse --show-toplevel)

# Defines toolchain and common modules & paths
# Debug and UART commuication drivers are included by default
include $(PROJ_ROOT)/common.mk

# Define this macro to turn off debug messages globally.
DBG_MACRO = #-DNO_DEBUG

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR): \
	$(PROJ_ROOT)/lib/ott_protocol: \
	$(PROJ_ROOT)/vendor/mbedtls/library:

# mbedTLS headers
INC += -I $(PROJ_ROOT)/vendor/mbedtls/include

# OTT protocol API header
INC += -I $(PROJ_ROOT)/include/ott_protocol/

# User application includes
INC += -I $(SRCDIR)/include

# User application sources
USER_SRC = $(wildcard $(SRCDIR)/*.c)

# OTT protocol API source
LIB_SRC += ott_protocol.c

# mbedTLS sources
# NOTE: Include custom net.c here
# XXX: This is an incomplete list for now and so it is not possible to compile
# ott_protocol_test just yet. The configuration header must be updated to exclude
# those modules which won't be used depending on the cipher suite we end up using.
LIB_SRC += ssl_tls.c \
	   ctr_drbg.c \
	   entropy.c \
	   sha512.c \
	   entropy_poll.c \
	   x509_crt.c \
	   certs.c \
	   cipher.c \
	   md.c \
	   md5.c \
	   bignum.c \
	   asn1parse.c \
	   aes.c \
	   debug.c \
	   pem.c

# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC = ott_protocol.c

# Defines the target rules
include $(PROJ_ROOT)/rules.mk

endif
