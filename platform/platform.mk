# Copyright(C) 2017 Verizon. All rights reserved.

# raspberry_pi3 and linux based virtual devices use same code base
ifeq ($(DEV_BOARD),$(filter $(DEV_BOARD),raspberry_pi3 virtual))
DEV_BOARD_MOD = raspberry_pi3
else
DEV_BOARD_MOD = $(DEV_BOARD)
PLATFORM_TIMER_HAL_SRC = timer_hal.c timer_interface.c
PLATFORM_GPS_HAL_SRC = gps.c
endif

PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/inc
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/modem/$(MODEM_TARGET)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD_MOD)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/drivers/gps/$(GPS_CHIPSET)/inc
PLATFORM_INC += -I $(SDK_ROOT)/inc/network/at

FIND_INC = -name "*.h"
ifeq ($(DEV_BOARD_MOD),$(filter $(DEV_BOARD_MOD),raspberry_pi3 virtual))
PLATFORM_DRV_INC = $(shell find $(PLATFORM_HAL_ROOT)/drivers/oem/$(DEV_BOARD_MOD)/* $(FIND_INC))
PLATFORM_INC += -I $(dir $(PLATFORM_DRV_INC))
else
ifeq ($(CHIPSET_MCU), stm32l476rgt)
PLATFORM_DRV_INC = $(shell find $(PLATFORM_HAL_ROOT)/drivers/oem/$(CHIPSET_FAMILY)/$(CHIPSET_MCU) $(FIND_INC))
PLATFORM_INC += -I $(dir $(PLATFORM_DRV_INC))
endif
endif

PLATFORM_HAL_SRC = dbg.c uart.c
PLATFORM_HAL_SRC += sys.c gpio.c utils.c oem.c
PLATFORM_HAL_SRC += i2c.c
PLATFORM_HAL_SRC += pin_map.c port_pin_api.c
PLATFORM_HAL_SRC += $(PLATFORM_TIMER_HAL_SRC)
ifneq ($(GPS_CHIPSET),)
PLATFORM_HAL_SRC += $(PLATFORM_GPS_HAL_SRC)
endif

ifneq ($(PROTOCOL),SMSNAS_PROTOCOL)
PLATFORM_HAL_SRC += hwrng.c
endif

export PLATFORM_HAL_SRC
export PLATFORM_INC

PLATFORM_HAL_CFLAGS =
export PLATFORM_HAL_CFLAGS


FIND_PARAM = -maxdepth 1 -name "*.c"

# Path for driver sources
DRV_ROOT = $(PLATFORM_HAL_ROOT)/drivers/*
DRV_COM = $(shell find $(DRV_ROOT) $(FIND_PARAM))
DRV_CS = $(shell find $(DRV_ROOT)/$(CHIPSET_FAMILY) $(FIND_PARAM))
DRV_MCU = $(shell find $(DRV_ROOT)/$(CHIPSET_FAMILY)/$(CHIPSET_MCU) $(FIND_PARAM))

# Path for software abstraction sources
SW_ROOT = $(PLATFORM_HAL_ROOT)/sw
SW_MCU = $(shell find $(SW_ROOT)/$(CHIPSET_FAMILY)/$(CHIPSET_MCU) $(FIND_PARAM))
SW_BOARD = $(shell find $(SW_ROOT)/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD_MOD) $(FIND_PARAM))

# Path for GPS sources
GPS_ROOT = $(PLATFORM_HAL_ROOT)/drivers/gps
GPS_CS = $(shell find $(GPS_ROOT)/$(GPS_CHIPSET) $(FIND_PARAM))

# This eases the strict conditions for specifying chipset details and directly
# includes files from the specified dev board
ifeq ($(CHIPSET_FAMILY),)
ifeq ($(CHIPSET_MCU),)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(DEV_BOARD_MOD)
DRV_BYPASS_CHIP = $(shell find $(DRV_ROOT)/$(DEV_BOARD_MOD) $(FIND_PARAM))
SW_BYPASS_CHIP = $(shell find $(SW_ROOT)/$(DEV_BOARD_MOD) $(FIND_PARAM))
endif
endif

DRV_PATH = $(patsubst %,%:,$(sort $(dir $(DRV_COM) $(DRV_CS) $(DRV_MCU) $(DRV_BYPASS_CHIP))))
SW_PATH = $(patsubst %,%:,$(sort $(dir $(SW_MCU) $(SW_BOARD) $(SW_BYPASS_CHIP))))
GPS_PATH = $(patsubst %,%:,$(sort $(dir $(GPS_CS))))

# Search paths for device drivers and port pin API sources
vpath %.c $(DRV_PATH) $(SW_PATH) $(GPS_PATH)
