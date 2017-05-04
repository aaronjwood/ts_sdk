# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Define common parameters used by the build process for a basic build for
# this MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root
CHIPSET_BUILDENV = $(shell dirname $(PROJ_ROOT))ts_sdk_bldenv
STMSDK_HEADER_ROOT = $(CHIPSET_BUILDENV)/stm32f4
GCC_ROOT = $(CHIPSET_BUILDENV)/toolchain/gcc-arm-none-eabi-5_4-2016q2
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
