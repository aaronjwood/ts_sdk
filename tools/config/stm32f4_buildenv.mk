# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Define common parameters used by the build process for stm32f429 MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root

STMSDK_HEADER_ROOT = $(CHIPSET_BUILDENV)

GCC_ROOT = /ts_sdk_bldenv/toolchain/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(STMSDK_HEADER_ROOT)

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/src
STM32_CMSIS = $(STM32_LIB_COMMON)/src/CMSIS/Device/ST/STM32F4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size
export CC OBJDUMP OBJCOPY SIZE

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx
export MDEF
export ARCHFLAGS

# Platform support is based on STMicro's HAL library
# Peripheral related headers
CHIPSET_INC = -I $(STM32_LIB_COMMON)/inc
# CMSIS (Core) headers
CHIPSET_INC += -I $(STM32_LIB_COMMON)/src/CMSIS/Include
# CMSIS (Device specific) headers
CHIPSET_INC += -I $(STM32_CMSIS)/Include
# Standard library function headers
CHIPSET_INC += -I $(GCC_ROOT)/include

CHIPSET_LDFLAGS = -L /build/$(CHIPSET_FAMILY)/
export CHIPSET_LDFLAGS

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs

# Linker script
LDSCRIPT = -T $(STM32_LIB_COMMON)/STM32F429ZITx_FLASH.ld
