# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Define common parameters used by the build process for a basic build for
# this MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root

TOOLS_ROOT = $(CHIPSET_BUILDENV)/stm32f429zi
GCC_ROOT = $(CHIPSET_BUILDENV)/toolchain/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(TOOLS_ROOT)

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/src
STM32_CMSIS = $(STM32_LIB_COMMON)/src/CMSIS/Device/ST/STM32F4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
AS = $(GCC_ROOT)/bin/arm-none-eabi-as
AR = $(GCC_ROOT)/bin/arm-none-eabi-ar
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size
export CC AS AR OBJDUMP OBJCOPY SIZE

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx
export MDEF
export ARCHFLAGS

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs

# Linker script
LDSCRIPT = -T $(STM32_LIB_COMMON)/ldscript/STM32F429ZITx_FLASH.ld

# Point to the C Runtime startup code
STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32f429xx.s
OBJ_STARTUP = startup_stm32f429xx.o

# Platform support is based on STMicro's HAL library
# Peripheral related headers
CHIPSET_INC = -I $(STM32_LIB_COMMON)/inc
# CMSIS (Core) headers
CHIPSET_INC += -I $(STM32_LIB_COMMON)/src/CMSIS/Include
# CMSIS (Device specific) headers
CHIPSET_INC += -I $(STM32_CMSIS)/Include
# Standard library function headers
CHIPSET_INC += -I $(GCC_ROOT)/include

# Base HAL sources
CHIPSET_SRC = stm32f4xx_hal.c system_stm32f4xx.c
# Peripheral HAL sources
CHIPSET_SRC += stm32f4xx_hal_cortex.c
CHIPSET_SRC += stm32f4xx_hal_gpio.c
CHIPSET_SRC += stm32f4xx_hal_rcc.c
CHIPSET_SRC += stm32f4xx_hal_rcc_ex.c
CHIPSET_SRC += stm32f4xx_hal_uart.c
CHIPSET_SRC += stm32f4xx_hal_i2c.c
CHIPSET_SRC += stm32f4xx_hal_pwr.c
CHIPSET_SRC += stm32f4xx_hal_pwr_ex.c
CHIPSET_SRC += stm32f4xx_hal_tim.c
CHIPSET_SRC += stm32f4xx_hal_tim_ex.c
CHIPSET_SRC += stm32f4xx_hal_rng.c
CHIPSET_SRC += $(CHIPSET_SRC_$(BUILD_TARGET))

vpath %.c $(STM32_PLIB): \
	  $(STM32_CMSIS)/Source/Templates:
