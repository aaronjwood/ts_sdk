# Copyright(C) 2017 Verizon. All rights reserved.

PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/inc
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD)

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.
PLATFORM_TIMER_HAL_SRC = timer_hal.c timer_interface.c
PLATFORM_HAL_SRC = dbg_$(CHIPSET_FAMILY).c uart_$(CHIPSET_FAMILY).c
PLATFORM_HAL_SRC += sys_$(CHIPSET_FAMILY).c gpio_$(CHIPSET_FAMILY).c
PLATFORM_HAL_SRC += i2c_$(CHIPSET_FAMILY).c
PLATFORM_HAL_SRC += pin_map.c port_pin_api.c
PLATFORM_HAL_SRC += $(PLATFORM_TIMER_HAL_SRC)

ifneq ($(PROTOCOL),SMSNAS_PROTOCOL)
PLATFORM_HAL_SRC += hwrng_$(CHIPSET_FAMILY).c
endif

export PLATFORM_HAL_SRC
export PLATFORM_INC

PLATFORM_HAL_CFLAGS =
export PLATFORM_HAL_CFLAGS

# Search paths for device driver sources
vpath %.c $(PLATFORM_HAL_ROOT)/drivers/dbg: \
	$(PLATFORM_HAL_ROOT)/drivers/uart: \
	$(PLATFORM_HAL_ROOT)/drivers/hwrng: \
	$(PLATFORM_HAL_ROOT)/drivers/gpio: \
	$(PLATFORM_HAL_ROOT)/drivers/i2c: \
	$(PLATFORM_HAL_ROOT)/drivers/sys: \
	$(PLATFORM_HAL_ROOT)/drivers/timer: \
	$(PLATFORM_HAL_ROOT)/drivers/timer/$(CHIPSET_FAMILY)/$(CHIPSET_MCU): \
	$(PLATFORM_HAL_ROOT)/sw: \
	$(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY): \
	$(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU): \
	$(PLATFORM_HAL_ROOT)/sw/$(CHIPSET_FAMILY)/$(CHIPSET_MCU)/$(DEV_BOARD):
