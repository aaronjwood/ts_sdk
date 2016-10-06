# Copyright(C) 2016 Verizon. All rights reserved.

# Define common parameters used by the build process such as paths to the
# toolchain, compiler and linker flags and names of the core modules needed
# for a basic build on this MCU.
# It is assumed that every app will require access to the debug and UART modules

# Paths to toolchain and HAL root
TOOLS_ROOT = $(PROJ_ROOT)/tools/installed/stm32f4
GCC_ROOT = $(TOOLS_ROOT)/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(TOOLS_ROOT)/STM32Cube_FW_F4_V1.13.0

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Src
STM32_CMSIS = $(STM32_LIB_COMMON)/Drivers/CMSIS/Device/ST/STM32F4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
AS = $(GCC_ROOT)/bin/arm-none-eabi-as
AR = $(GCC_ROOT)/bin/arm-none-eabi-ar
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size
export CC AS AR OBJDUMP OBJCOPY SIZE

# Debug and optimization flags
DBG_OP_USER_FLAGS = -g -ggdb3 -O0
DBG_OP_LIB_FLAGS = -Os

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx
export MDEF
export ARCHFLAGS

# Firmware name
FW_EXEC = firmware.elf
LDFLAGS ?= -Wl,-Map,fw.map,--cref

# Device drivers
INC = -I $(PROJ_ROOT)/include/dbg
INC += -I $(PROJ_ROOT)/include/uart
INC += -I $(PROJ_ROOT)/include/hwrng

# Certificates
INC += -I $(PROJ_ROOT)/include/certs

# Peripheral related headers
INC += -I $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Inc

# CMSIS (Core) headers
INC += -I $(STM32_LIB_COMMON)/Drivers/CMSIS/Include

# CMSIS (Device specific) headers
INC += -I $(STM32_CMSIS)/Include

# Standard library function headers
INC += -I $(GCC_ROOT)/include
export INC

# Linker script
LDSCRIPT = -T $(STM32_LIB_COMMON)/Projects/STM32F429ZI-Nucleo/Templates/SW4STM32/STM32F429ZI_Nucleo_144/STM32F429ZITx_FLASH.ld

# Point to the C Runtime startup code
STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32f429xx.s
OBJ_STARTUP = startup_stm32f429xx.o

# List of core library components to be included in the build process
# This includes debugging, UART and HW RNG modules.
CORELIB_SRC = stm32f4xx_hal.c system_stm32f4xx.c dbg.c uart.c hwrng.c

# OTT Protocol API source
CLOUD_COMM_SRC = ott_protocol.c

# Peripheral HAL sources
LIB_SRC = stm32f4xx_hal_cortex.c
LIB_SRC += stm32f4xx_hal_gpio.c
LIB_SRC += stm32f4xx_hal_rcc.c
LIB_SRC += stm32f4xx_hal_rcc_ex.c
LIB_SRC += stm32f4xx_hal_uart.c
LIB_SRC += stm32f4xx_hal_pwr.c
LIB_SRC += stm32f4xx_hal_pwr_ex.c
LIB_SRC += stm32f4xx_hal_tim.c
LIB_SRC += stm32f4xx_hal_tim_ex.c
LIB_SRC += stm32f4xx_hal_rng.c

# Search paths for core module sources
vpath %.c $(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/lib/uart: \
	$(PROJ_ROOT)/lib/hwrng: \
	$(PROJ_ROOT)/lib/ott_protocol: \
	$(STM32_PLIB): \
	$(STM32_CMSIS)/Source/Templates:
