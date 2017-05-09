# Copyright(C) 2017 Verizon. All rights reserved.

# SDK library generator makefile.

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(MK_HELPER_PATH)/build_in_subdir.mk
else

include $(SDK_ROOT)/cc_sdk.mk

# User can select install path from make INSTALL_PATH=<path>
INSTALL_PATH ?= $(SDK_ROOT)/install

# Compiler and archive, user needs to overwrite these variables if different
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
RANLIB = arm-none-eabi-ranlib
export CC AR RANLIB

# SDK depends on this platform headers, end user needs to provide similar
# API signature if porting sdk into their own platform environment

PLATFORM_DEP_HEADERS = ts_sdk_board_config.h timer_hal.h timer_interface.h \
	uart_hal.h sys.h dbg.h port_pin_api.h pin_map.h pin_defs.h pin_std_defs.h \
	hwrng_hal.h

# mbedtls library requires exporting INC
INC += -I $(INSTALL_PATH)/platform_inc
export INC

# Create lists of object files to be generated from the sources
OBJ_SDKLIB = $(addsuffix .o, $(basename $(SDK_SRC)))

CFLAGS_COM = -Wall -Wcast-align -Werror -std=c99 $(CFLAGS_SDK) $(INC) $(SDK_INC) -D$(PROTOCOL) -D$(MODEM)

#==================================RULES=======================================#
.PHONY: all build clean

all build: install_platform $(OBJ_SDKLIB) vendor_libs libsdk.a install

libsdk.a:
	$(AR) rc $@ $(OBJ_SDKLIB)
	$(RANLIB) $@

$(OBJ_SDKLIB): %.o: %.c
	$(CC) -c $(CFLAGS_COM) $< -o $@

vendor_libs:
ifdef VENDOR_LIB_DIRS
	$(MAKE) -C $(PROJ_ROOT)/sdk/cloud_comm/vendor $(VENDOR_LIB_DIRS)
endif

clean:
ifdef VENDOR_LIB_DIRS
	@$(MAKE) -C $(PROJ_ROOT)/sdk/cloud_comm/vendor clean
endif
	rm -rf $(SDK_ROOT)/build

install:
	-mkdir -p $(INSTALL_PATH)/sdk_inc
	-mkdir -p $(INSTALL_PATH)/library
	cp -r $(SDK_ROOT)/inc/* $(INSTALL_PATH)/sdk_inc
	cp $(SDK_ROOT)/build/*.a $(INSTALL_PATH)/library
ifdef VENDOR_LIB_DIRS
	cp -r $(SDK_ROOT)/vendor/mbedtls/include/ $(INSTALL_PATH)/sdk_inc
endif

create_platform_dir:
	-mkdir -p $(INSTALL_PATH)/platform_inc
%.h:
	cp $(shell find $(PLATFORM_HAL_ROOT) -type f -name "$@") $(INSTALL_PATH)/platform_inc/

install_platform: create_platform_dir $(PLATFORM_DEP_HEADERS)

endif
