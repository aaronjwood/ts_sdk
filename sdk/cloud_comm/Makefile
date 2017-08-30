# Copyright(C) 2017 Verizon. All rights reserved.

# SDK library generator makefile.

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(MK_HELPER_PATH)/build_in_subdir.mk
else

# Selecting platform according to CHIPSET_FAMILY
ifeq ($(CHIPSET_FAMILY),stm32f4)
CHIPSET_BUILDENV=/ts_sdk_bldenv
include $(TARGET_ROOT)/stmicro/chipset/$(CHIPSET_FAMILY)/common_stm32f4.mk
else ifeq ($(CHIPSET_FAMILY),stm32l4)
CHIPSET_BUILDENV=/ts_sdk_bldenv
include $(TARGET_ROOT)/stmicro/chipset/$(CHIPSET_FAMILY)/common_stm32l4.mk
else ifeq ($(CHIPSET_FAMILY),osx)
include $(MK_HELPER_PATH)/osx_buildenv.mk
else ifeq ($(CHIPSET_FAMILY),)
ifeq ($(DEV_BOARD),raspberry_pi3)
include $(MK_HELPER_PATH)/raspberry_pi3_buildenv.mk
else
$(error "Makefile must define valid DEV_BOARD variable")
endif
else
$(error "Makefile must define the CHIPSET_FAMILY variable")
endif

ifeq ($(PROTOCOL),OTT_PROTOCOL)
PROT_DIR = ott_protocol
LIMIT_INC = ott_limits.h
else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
PROT_DIR = smsnas_protocol
LIMIT_INC = smsnas_limits.h
else ifeq ($(PROTOCOL),MQTT_PROTOCOL)
PROT_DIR = mqtt_protocol
LIMIT_INC = mqtt_limits.h
else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

ifneq ($(DEV_BOARD),raspberry_pi3)
BOARD_CONFIG = $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD)/board_config.h
HSEVALUE= $(shell grep  "EXT_CLK_FREQ_HZ" $(BOARD_CONFIG) | cut -f3 -d"	")
CHIPSET_HSECLK = -DHSE_VALUE='$(HSEVALUE)'
endif

#HSEVALUE= $(shell grep  "EXT_CLK_FREQ_HZ" $(PROJ_ROOT)/platform/sw/stm32f4/stm32f415rgt/beduin/board_config.h | cut -f3 -d"	")

# CHIPSET_SRC is from above included files for the chipset
CORELIB_SRC += $(CHIPSET_SRC)

# Header files provided by the chipset build environment
INC += $(CHIPSET_INC)

include $(SDK_ROOT)/cc_sdk.mk
INC += $(SDK_INC)

include $(PLATFORM_HAL_ROOT)/platform.mk
INC += $(PLATFORM_INC)

# SDK_SRC is provided by cc_sdk.mk, includes modem and protocol source files
# based on the configurations
CORELIB_SRC += $(SDK_SRC)

# PLATFORM_HAL_SRC is provided by platform.mk
CORELIB_SRC += $(PLATFORM_HAL_SRC)

# User can select install path from make INSTALL_PATH=<path>
CLIENT_NAME = sdk_client
INSTALL_PATH ?= $(SDK_ROOT)/$(CLIENT_NAME)
ifeq ($(DEV_BOARD),raspberry_pi3)
INSTALL_INCLUDE = $(DEV_BOARD)$(CHIPSET_MCU)/include
INSTALL_LIB = $(DEV_BOARD)$(CHIPSET_MCU)/lib
else
INSTALL_INCLUDE = $(DEV_BOARD)$(CHIPSET_MCU)_$(MODEM_TARGET)_$(PROTOCOL)/include
INSTALL_LIB = $(DEV_BOARD)$(CHIPSET_MCU)_$(MODEM_TARGET)_$(PROTOCOL)/lib
endif 
#Used for docker mount point, so host can access build artifacts
TARGET_PATH = /build/$(CLIENT_NAME)

# SDK depends on this platform headers, end user needs to provide similar
# API signature if porting sdk into their own platform environment

FIND_INC = -name "*.h"
PLATFORM_DEP_HEADERS = $(shell find $(PLATFORM_HAL_ROOT)/inc/* $(FIND_INC))

#since app uses it directly
ifdef VENDOR_LIB_DIRS
ifneq (,$(findstring cJSON,$(VENDOR_LIB_DIRS)))
PLATFORM_DEP_HEADERS += $(PROJ_ROOT)/sdk/cloud_comm/vendor/cJSON/cJSON.h
endif
endif

# mbedtls library requires exporting INC
INC += -I $(INSTALL_PATH)/include
export INC

ifeq ($(DEV_BOARD),raspberry_pi3)
OBJ = $(OBJ_SDKLIB)
else
OBJ = $(OBJ_SDKLIB) $(OBJ_STARTUP)
endif

# Create lists of object files to be generated from the sources
OBJ_SDKLIB = $(addsuffix .o, $(basename $(CORELIB_SRC)))

ifeq ($(DEV_BOARD),raspberry_pi3)
CFLAGS_COM = -Wall -Wcast-align -Werror -std=c99 $(CFLAGS_SDK) $(INC) $(SDK_INC) \
        -D$(PROTOCOL) -D$(MODEM) -D$(DEV_BOARD)
else
CFLAGS_COM = -Werror -std=c99 $(CFLAGS_SDK) $(INC) $(SDK_INC) \
	-D$(PROTOCOL) -D$(MODEM) -D$(DEV_BOARD) -D$(CHIPSET_MCU)
CFLAGS_CHIPSET = -Os $(CFLAGS_COM) $(DBG_OP_LIB_FLAGS) \
        -fdata-sections -ffunction-sections
endif

NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs
#==================================RULES=======================================#
.PHONY: all build clean

all build: $(OBJ) vendor_libs libsdk.a install
print-%: ; @echo $* = $($*)


#yes, make one big blob, let user strip off unused objects
libsdk.a:
	$(AR) rcs $@ $(shell find $(PROJ_ROOT)/sdk/cloud_comm/build/ -name "*.o") \
	$(shell find $(PROJ_ROOT)/sdk/cloud_comm/vendor/* -name "*.o")

$(OBJ_STARTUP):
	$(AS) -c $(ARCHFLAGS) -o $(OBJ_STARTUP) $(STARTUP_SRC)

ifeq ($(DEV_BOARD),raspberry_pi3)
$(OBJ_SDKLIB): %.o: %.c
	$(CC) -c $(CFLAGS_COM) $< -o $@
else
$(OBJ_SDKLIB): %.o: %.c
	$(CC) -c $(CFLAGS_CHIPSET) $(ARCHFLAGS) $(MDEF) $< -o $@
endif

vendor_libs:
ifdef VENDOR_LIB_DIRS
	$(MAKE) -C $(PROJ_ROOT)/sdk/cloud_comm/vendor $(VENDOR_LIB_DIRS)
endif

clean:
ifdef VENDOR_LIB_DIRS
	@$(MAKE) -C $(PROJ_ROOT)/sdk/cloud_comm/vendor clean
endif
	rm -rf $(SDK_ROOT)/build
	rm -rf $(INSTALL_PATH)

install: install_platform
	-mkdir -p $(INSTALL_PATH)/$(INSTALL_INCLUDE)/api
	-mkdir -p $(INSTALL_PATH)/$(INSTALL_LIB)
	-mkdir -p /build/
	cp -r $(SDK_ROOT)/api/* $(INSTALL_PATH)/$(INSTALL_INCLUDE)/api
	cp -r $(SDK_ROOT)/inc/service_ids.h $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp -r $(SDK_ROOT)/inc/services/*.h $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp -r $(SDK_ROOT)/inc/protocols/$(PROT_DIR)/$(LIMIT_INC) $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp -r $(SDK_ROOT)/inc/protocols/protocol_def.h $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp $(SDK_ROOT)/build/libsdk.a $(INSTALL_PATH)/$(INSTALL_LIB)
ifneq ($(DEV_BOARD),raspberry_pi3)
	cp -r $(PROJ_ROOT)/platform/sw/*.h $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp -r $(PROJ_ROOT)/platform/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/*.h $(INSTALL_PATH)/$(INSTALL_INCLUDE)
ifeq ($(CHIPSET_MCU),stm32f415rgt)
	cp $(PROJ_ROOT)/ldscript/STM32F415RGTx_FLASH.ld $(INSTALL_PATH)/$(INSTALL_LIB)
else ifeq ($(CHIPSET_MCU),stm32f429zit)
	cp $(PROJ_ROOT)/ldscript/STM32F429ZITx_FLASH.ld $(INSTALL_PATH)/$(INSTALL_LIB)
else
	cp $(PROJ_ROOT)/ldscript/*.ld $(INSTALL_PATH)/$(INSTALL_LIB)
endif
endif
	cp -r $(INSTALL_PATH) $(TARGET_PATH)

create_platform_dir:
	-mkdir -p $(INSTALL_PATH)/$(INSTALL_INCLUDE)
	cp $(PLATFORM_DEP_HEADERS) $(INSTALL_PATH)/$(INSTALL_INCLUDE)

install_platform: create_platform_dir

endif
