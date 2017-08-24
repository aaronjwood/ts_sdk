# Copyright(C) 2017 Verizon. All rights reserved.

# Define common parameters used by the build process for stm32l476rg MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root

STMSDK_HEADER_ROOT = $(CHIPSET_BUILDENV)

GCC_ROOT = /ts_sdk_bldenv/toolchain/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(STMSDK_HEADER_ROOT)/chipset_hal

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/STM32Cube_FW_L4_V1.8.0/Drivers/STM32L4xx_HAL_Driver/Src
STM32_CMSIS = $(STM32_LIB_COMMON)/STM32Cube_FW_L4_V1.8.0/Drivers/CMSIS/Device/ST/STM32L4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
AS = $(GCC_ROOT)/bin/arm-none-eabi-as
AR = $(GCC_ROOT)/bin/arm-none-eabi-ar
RANLIB = $(GCC_ROOT)/bin/arm-none-eabi-ranlib
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size

export CC AS AR RANLIB OBJDUMP OBJCOPY SIZE


# Machine specific compiler, assembler settings and Linker script
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -u _printf_float

ifeq ($(CHIPSET_MCU),stm32l476rgt)
	MDEF = -DSTM32L476xx
	LDSCRIPT = -T $(STMSDK_HEADER_ROOT)/STM32L476RGTx_FLASH.ld
else
	$(error "$(CHIPSET_MCU) chipset is not supported")

endif

export MDEF
export ARCHFLAGS

# Platform support is based on STMicro's HAL library
# Peripheral related headers
CHIPSET_INC = -I $(STM32_LIB_COMMON)/STM32Cube_FW_L4_V1.8.0/Drivers/STM32L4xx_HAL_Driver/Inc
# CMSIS (Core) headers
CHIPSET_INC += -I $(STM32_LIB_COMMON)/STM32Cube_FW_L4_V1.8.0/Drivers/CMSIS/Include
CHIPSET_INC += -I $(STM32_CMSIS)/Include
# Standard library function headers
CHIPSET_INC += -I $(GCC_ROOT)/include

CHIPSET_LDFLAGS = -L /build/$(CHIPSET_FAMILY)/
export CHIPSET_LDFLAGS

FW_EXEC = firmware_$(PROTOCOL)_$(DEV_BOARD).elf

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs

