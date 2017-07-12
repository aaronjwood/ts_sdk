# Copyright(C) 2017 Verizon. All rights reserved.

PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/inc
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/modem/$(MODEM_TARGET)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD)

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.
PLATFORM_TIMER_HAL_SRC = timer_hal.c timer_interface.c
PLATFORM_GPS_HAL_SRC = gps.c

PLATFORM_HAL_SRC = dbg.c uart.c
PLATFORM_HAL_SRC += sys.c gpio.c
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
DRV_PATH = $(patsubst %,%:,$(sort $(dir $(DRV_COM) $(DRV_CS) $(DRV_MCU))))

# Path for software abstraction sources
SW_ROOT = $(PLATFORM_HAL_ROOT)/sw
SW_MCU = $(shell find $(SW_ROOT)/$(CHIPSET_FAMILY)/$(CHIPSET_MCU) $(FIND_PARAM))
SW_BOARD = $(shell find $(SW_ROOT)/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD) $(FIND_PARAM))
SW_PATH = $(patsubst %,%:,$(sort $(dir $(SW_MCU) $(SW_BOARD))))

# Path for GPS sources
GPS_ROOT = $(PLATFORM_HAL_ROOT)/drivers/gps
GPS_CS = $(shell find $(GPS_ROOT)/$(GPS_CHIPSET) $(FIND_PARAM))
GPS_PATH = $(patsubst %,%:,$(sort $(dir $(GPS_CS))))

# Search paths for device drivers and port pin API sources
vpath %.c $(DRV_PATH) $(SW_PATH) $(GPS_PATH)
