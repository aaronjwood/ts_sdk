# Copyright(C) 2017 Verizon. All rights reserved.

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(MK_HELPER_PATH)/build_in_subdir.mk
else

# default virtual device
PROJ_NAME ?= light_bulb

ifeq ($(PROJ_NAME),light_bulb)
VIRT_DEV = -DLIGHT_BULB
VIRTUAL_DEV_NAME = light_bulb
else ifeq ($(PROJ_NAME),accelerometer)
VIRT_DEV = -DACCELEROMETER
VIRTUAL_DEV_NAME = accelerometer
else ifeq ($(PROJ_NAME),gps)
VIRT_DEV = -DGPS
VIRTUAL_DEV_NAME = gps
else
$(error provide valid virtual device value)
endif

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR)/$(VIRTUAL_DEV_NAME): \
	$(SRCDIR)/common_source:

# User application includes
APP_INC = -I $(SRCDIR)/include -I $(SRCDIR)/$(VIRTUAL_DEV_NAME)/include

# User application sources
APP_SRC = $(wildcard $(SRCDIR)/$(VIRTUAL_DEV_NAME)/*.c)
APP_SRC += $(wildcard $(SRCDIR)/common_source/*.c)

# User defined compilation flags
APP_CFLAGS += $(VIRT_DEV)

# Sources are built without debug info and optimized for size.
# Use DBG_LIB_SRC to compile a subset of the sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = abc.c xvz.c
DBG_LIB_SRC =

# Common and per-platform Makefile variables
include $(MK_HELPER_PATH)/common.mk
print-%: ; @echo $* = $($*)

endif
