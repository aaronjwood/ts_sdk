# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# PROJ_NAME selects which example to build. Depending on the sensor
# example PROJ_NAME should be changed. Currently we have bmp180 and
# xnucleo-iks01a1 examples .By default, the Makefile assumes 'bmp180'
# as the project This means that 'make' is equivalent to
# 'make PROJ_NAME=bmp180'
#

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(MK_HELPER_PATH)/build_in_subdir.mk
else


PROJ_NAME ?= bme280-beduin
#PROJ_NAME ?=xnucleo-iks01a1

# Comment out this macro if application supported sensor are connected to the board
SIMU_SENSOR = -DSIMULATE_SENSOR

# uncomment NO_DEBUG to turn off debug messages globally.
DBG_MACRO = $(SIMU_SENSOR) #-DNO_DEBUG

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR)/$(PROJ_NAME): \
		$(SRCDIR)/bmc156: \
		$(SRCDIR)/bh1750fvi: \
	$(SRCDIR):

# User application includes
APP_INC = -I $(SRCDIR)/include

# User application sources
APP_SRC = $(wildcard $(SRCDIR)/*.c)
APP_SRC += $(wildcard $(SRCDIR)/$(PROJ_NAME)/*.c)
APP_SRC += $(wildcard $(SRCDIR)/bmc156/*.c)
APP_SRC += $(wildcard $(SRCDIR)/bh1750fvi/*.c)

# Sources are built without debug info and optimized for size.
# Use DBG_LIB_SRC to compile a subset of the sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

LDFLAGS += -u _printf_float

# Common and per-platform Makefile variables
include $(MK_HELPER_PATH)/common.mk
print-%: ; @echo $* = $($*)

endif
