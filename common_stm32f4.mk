# Define common parameters used by the build process such as paths to the
# toolchain, compiler and linker flags and names of the core modules needed
# for a basic build on this MCU.

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

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx
export MDEF
export ARCHFLAGS

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs

# Linker script
LDSCRIPT = -T $(STM32_LIB_COMMON)/Projects/STM32F429ZI-Nucleo/Templates/SW4STM32/STM32F429ZI_Nucleo_144/STM32F429ZITx_FLASH.ld

# Point to the C Runtime startup code
STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32f429xx.s
OBJ_STARTUP = startup_stm32f429xx.o

# Default to networking via LTE modem and AT commands on this platform.
NET_TYPE ?= at

# Platform support is based on STMicro's HAL library
# Peripheral related headers
PLATFORM_INC = -I $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Inc
# CMSIS (Core) headers
PLATFORM_INC += -I $(STM32_LIB_COMMON)/Drivers/CMSIS/Include
# CMSIS (Device specific) headers
PLATFORM_INC += -I $(STM32_CMSIS)/Include
# Standard library function headers
PLATFORM_INC += -I $(GCC_ROOT)/include

# Base HAL sources
PLATFORM_SRC = stm32f4xx_hal.c system_stm32f4xx.c
# Peripheral HAL sources
PLATFORM_SRC += stm32f4xx_hal_cortex.c
PLATFORM_SRC += stm32f4xx_hal_gpio.c
PLATFORM_SRC += stm32f4xx_hal_rcc.c
PLATFORM_SRC += stm32f4xx_hal_rcc_ex.c
PLATFORM_SRC += stm32f4xx_hal_uart.c
PLATFORM_SRC += stm32f4xx_hal_pwr.c
PLATFORM_SRC += stm32f4xx_hal_pwr_ex.c
PLATFORM_SRC += stm32f4xx_hal_tim.c
PLATFORM_SRC += stm32f4xx_hal_tim_ex.c
PLATFORM_SRC += stm32f4xx_hal_rng.c
PLATFORM_SRC += stm32f4xx_hal_i2c.c


vpath %.c $(STM32_PLIB): \
	  $(STM32_CMSIS)/Source/Templates:
