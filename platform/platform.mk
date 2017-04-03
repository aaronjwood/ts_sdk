# Copyright(C) 2017 Verizon. All rights reserved.

INC += -I $(PLATFORM_HAL_ROOT)/inc
INC += -I $(PLATFORM_HAL_ROOT)/inc/dbg
INC += -I $(PLATFORM_HAL_ROOT)/inc/uart
INC += -I $(PLATFORM_HAL_ROOT)/inc/hwrng

# List of core library components to be included in the build process
# This includes the standard per-platform device drivers but not any
# toolchain or externally supplied files that may need to be built with
# different compiler options.

PLATFORM_HAL_SRC = dbg_$(BUILD_TARGET).c uart_$(BUILD_TARGET).c
PLATFORM_HAL_SRC += hwrng_$(BUILD_TARGET).c platform_$(BUILD_TARGET).c

CFLAGS_PLATFORM_HAL =
export CFLAGS_PLATFORM_HAL

# Search paths for device driver sources
vpath %.c $(PLATFORM_HAL_ROOT)/drivers/dbg: \
	$(PLATFORM_HAL_ROOT)/drivers/uart: \
	$(PLATFORM_HAL_ROOT)/drivers/hwrng: \
	$(PLATFORM_HAL_ROOT)/sw:
