# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Define common parameters used by the build process for stm32f429zit MCU.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root

STMSDK_HEADER_ROOT = $(CHIPSET_BUILDENV)

GCC_ROOT = /ts_sdk_bldenv/toolchain/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(STMSDK_HEADER_ROOT)/chipset_hal

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/STM32Cube_FW_F4_V1.16.0/Drivers/STM32F4xx_HAL_Driver/Src
STM32_CMSIS = $(STM32_LIB_COMMON)/STM32Cube_FW_F4_V1.16.0/Drivers/CMSIS/Device/ST/STM32F4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size
export CC OBJDUMP OBJCOPY SIZE

# Machine specific compiler, assembler settings and Linker script
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
ifeq ($(CHIPSET_MCU),stm32f415rgt)
	MDEF = -DSTM32F415xx
	LDSCRIPT = -T $(STMSDK_HEADER_ROOT)/STM32F415RGTx_FLASH.ld
else ifeq ($(CHIPSET_MCU), stm32f429zit)
	MDEF = -DSTM32F429xx
	LDSCRIPT = -T $(STMSDK_HEADER_ROOT)/STM32F429ZITx_FLASH.ld
else
	$(error "$(CHIPSET_MCU) chipset is not supported")

endif
export MDEF
export ARCHFLAGS

# Platform support is based on STMicro's HAL library
# Peripheral related headers
CHIPSET_INC = -I $(STM32_LIB_COMMON)/STM32Cube_FW_F4_V1.16.0/Drivers/STM32F4xx_HAL_Driver/Inc
# CMSIS (Core) headers
CHIPSET_INC += -I $(STM32_LIB_COMMON)/STM32Cube_FW_F4_V1.16.0/Drivers/CMSIS/Include
# CMSIS (Device specific) headers
CHIPSET_INC += -I $(STM32_CMSIS)/Include
# Standard library function headers
CHIPSET_INC += -I $(GCC_ROOT)/include

CHIPSET_LDFLAGS = -L /build/$(CHIPSET_FAMILY)/
export CHIPSET_LDFLAGS

FW_EXEC = firmware_$(PROTOCOL)_$(DEV_BOARD).elf

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs
