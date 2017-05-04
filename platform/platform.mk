# Copyright(C) 2017 Verizon. All rights reserved.

PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/inc
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)
PLATFORM_INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)/$(BUILD_BOARD)

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.
PLATFORM_TIMER_HAL_SRC = timer_hal.c timer_interface.c
PLATFORM_HAL_SRC = dbg_$(BUILD_TARGET).c uart_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += sys_$(BUILD_TARGET).c gpio_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += i2c_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += pin_map.c port_pin_api.c
PLATFORM_HAL_SRC += $(PLATFORM_TIMER_HAL_SRC)

ifneq ($(PROTOCOL),SMSNAS_PROTOCOL)
PLATFORM_HAL_SRC += hwrng_$(BUILD_TARGET).c
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
	$(PLATFORM_HAL_ROOT)/drivers/timer/$(BUILD_TARGET)/$(BUILD_MCU): \
	$(PLATFORM_HAL_ROOT)/sw: \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET): \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU): \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)/$(BUILD_BOARD):
