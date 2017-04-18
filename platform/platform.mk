# Copyright(C) 2017 Verizon. All rights reserved.

INC += -I $(PLATFORM_HAL_ROOT)/inc/dbg
INC += -I $(PLATFORM_HAL_ROOT)/inc/uart
INC += -I $(PLATFORM_HAL_ROOT)/inc/hwrng
INC += -I $(PLATFORM_HAL_ROOT)/inc/gpio
INC += -I $(PLATFORM_HAL_ROOT)/inc/i2c
INC += -I $(PLATFORM_HAL_ROOT)/inc/sys
INC += -I $(PLATFORM_HAL_ROOT)/inc/timer
INC += -I $(PLATFORM_HAL_ROOT)/sw
INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)
INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)
INC += -I $(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)/$(BUILD_BOARD)

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.

PLATFORM_HAL_SRC = dbg_$(BUILD_TARGET).c uart_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += platform_$(BUILD_TARGET).c gpio_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += i2c_$(BUILD_TARGET).c timer_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += uart_buf.c pin_map.c port_pin_api.c

ifneq ($(PROTOCOL),SMSNAS_PROTOCOL)
PLATFORM_HAL_SRC += hwrng_$(BUILD_TARGET).c
endif

export PLATFORM_HAL_SRC

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
	$(PLATFORM_HAL_ROOT)/sw: \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET): \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU): \
	$(PLATFORM_HAL_ROOT)/sw/$(BUILD_TARGET)/$(BUILD_MCU)/$(BUILD_BOARD):
