# Copyright(C) 2017 Verizon. All rights reserved.

# Define common parameters used by the build process for a basic build for
# this MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root

TOOLS_ROOT = $(CHIPSET_BUILDENV)/stm32l4
GCC_ROOT = $(CHIPSET_BUILDENV)/toolchain/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(TOOLS_ROOT)

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/src
STM32_CMSIS = $(STM32_LIB_COMMON)/src/CMSIS/Device/ST/STM32L4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
AS = $(GCC_ROOT)/bin/arm-none-eabi-as
AR = $(GCC_ROOT)/bin/arm-none-eabi-ar
RANLIB = $(GCC_ROOT)/bin/arm-none-eabi-ranlib
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Point to the C Runtime startup code
ifeq ($(CHIPSET_MCU),stm32l476rgt)
        MDEF = -DSTM32L476xx $(CHIPSET_HSECLK)
	STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32l476xx.s
	OBJ_STARTUP = startup_stm32l476xx.o
else
	$(error "$(CHIPSET_MCU) chipset is not supported")
endif


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
CHIPSET_SRC = stm32l4xx_hal.c system_stm32l4xx.c
# Peripheral HAL sources
CHIPSET_SRC += stm32l4xx_hal_cortex.c
CHIPSET_SRC += stm32l4xx_hal_gpio.c
CHIPSET_SRC += stm32l4xx_hal_rcc.c
CHIPSET_SRC += stm32l4xx_hal_rcc_ex.c
CHIPSET_SRC += stm32l4xx_hal_uart.c
CHIPSET_SRC += stm32l4xx_hal_i2c.c
CHIPSET_SRC += stm32l4xx_hal_pwr.c
CHIPSET_SRC += stm32l4xx_hal_pwr_ex.c
CHIPSET_SRC += stm32l4xx_hal_tim.c
CHIPSET_SRC += stm32l4xx_hal_tim_ex.c
CHIPSET_SRC += stm32l4xx_hal_rng.c

vpath %.c $(STM32_PLIB): \
	  $(STM32_CMSIS)/Source/Templates:
