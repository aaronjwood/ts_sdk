# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Define common parameters used by the build process such as paths to the
# toolchain, compiler and linker flags and names of the core modules needed
# for a basic build on this MCU.
# It is assumed that every app will require access to the debug and UART modules

# Paths to toolchain and HAL root
TOOLS_ROOT = $(PROJ_ROOT)/tools/installed/stm32f4
GCC_ROOT = $(TOOLS_ROOT)/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(firstword $(wildcard $(TOOLS_ROOT)/STM32Cube_FW_F4_*))

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

# Select protocol, valid options make PROTOCOL=<OTT_PROTOCOL or SMSNAS_PROTOCOL>
PROTOCOL ?= OTT_PROTOCOL

ifeq ($(PROTOCOL),OTT_PROTOCOL)
PROTOCOL_SRC = ott_protocol.c
PROTOCOL_DIR = ott_protocol
PROTOCOL_INC = ott_protocol
else
ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
PROTOCOL_SRC = smsnas_protocol.c
PROTOCOL_DIR = smsnas_protocol
PROTOCOL_INC = smsnas_protocol
else
$(error define valid protocol option from OTT_PROTOCOL and SMSNAS_PROTOCOL)
endif
endif

export PROTOCOL

# Comment this out to disable LTO and enable debug
#LTOFLAG = -flto
#export LTOFLAG

# Debug and optimization flags
# To reduce the firmware size at the expense of debugging capabilities in
# the user code, use the following instead:
#DBG_OP_USER_FLAGS = -Os $(LTOFLAG)
DBG_OP_USER_FLAGS = -g -ggdb3 -O0 $(LTOFLAG)
DBG_OP_LIB_FLAGS = -Os $(LTOFLAG)

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs --specs=nano.specs

# Machine specific compiler and assembler settings
ARCHFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx
export MDEF
export ARCHFLAGS

# Firmware name
FW_EXEC = firmware.elf
LDFLAGS ?= -Wl,-Map,fw.map,--cref

# defines which modem to use, currently only ublox-toby201 is supported
MODEM_TARGET = toby201

# Define the transport mechanism. Currently, TCP over LTE and SMS over NAS are supported
MODEM_TRANS = TCP
#MODEM_TRANS = SMS

MODEM_SRC += at_core.c

ifeq ($(MODEM_TARGET),toby201)
	MOD_TAR = -DMODEM_TOBY201
	export MOD_TAR
ifeq ($(MODEM_TRANS),TCP)
	MODEM_SRC += at_toby201_tcp.c
	MODEM_DIR += $(MODEM_TARGET)/tcp
else ifeq ($(MODEM_TRANS),SMS)
	MODEM_SRC += at_toby201_sms.c
	MODEM_SRC += smscodec.c
	MODEM_DIR += $(MODEM_TARGET)/sms
endif
endif

# platform related header files
INC += -I $(PROJ_ROOT)/include/platform

# Device drivers
INC += -I $(PROJ_ROOT)/include/dbg
INC += -I $(PROJ_ROOT)/include/uart
INC += -I $(PROJ_ROOT)/include/hwrng

# Certificates
INC += -I $(PROJ_ROOT)/include/certs

# net and at layer library includes
INC += -I $(PROJ_ROOT)/include/network/at
INC += -I $(PROJ_ROOT)/include/network
INC += -I $(PROJ_ROOT)/lib/network/at/core
INC += -I $(PROJ_ROOT)/lib/network/at/smscodec

# Peripheral related headers
INC += -I $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Inc

# CMSIS (Core) headers
INC += -I $(STM32_LIB_COMMON)/Drivers/CMSIS/Include

# CMSIS (Device specific) headers
INC += -I $(STM32_CMSIS)/Include

# Standard library function headers
INC += -I $(GCC_ROOT)/include

# mbedtls library header files
INC += -I $(PROJ_ROOT)/vendor/mbedtls/include

# Protocol API header
INC += -I $(PROJ_ROOT)/include/protocols
INC += -I $(PROJ_ROOT)/include/protocols/$(PROTOCOL_INC)

# Cloud communication API header
INC += -I $(PROJ_ROOT)/include/cloud_comm

# Device credentials header
INC += -I $(PROJ_ROOT)/include/dev_creds

export INC

# Linker script
LDSCRIPT = -T $(STM32_LIB_COMMON)/Projects/STM32F429ZI-Nucleo/Templates/SW4STM32/STM32F429ZI_Nucleo_144/STM32F429ZITx_FLASH.ld

# Point to the C Runtime startup code
STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32f429xx.s
OBJ_STARTUP = startup_stm32f429xx.o

# List of core library components to be included in the build process
# This includes debugging, UART, NET and HW RNG modules.
CORELIB_SRC = dbg.c uart.c hwrng.c net.c $(MODEM_SRC)

# Cloud communication / OTT protocol API sources
CLOUD_COMM_SRC = $(PROTOCOL_SRC) cloud_comm.c

# Peripheral HAL sources
HAL_LIB_SRC = stm32f4xx_hal.c
HAL_LIB_SRC += system_stm32f4xx.c
HAL_LIB_SRC += stm32f4xx_platform.c
HAL_LIB_SRC += stm32f4xx_hal_cortex.c
HAL_LIB_SRC += stm32f4xx_hal_gpio.c
HAL_LIB_SRC += stm32f4xx_hal_rcc.c
HAL_LIB_SRC += stm32f4xx_hal_rcc_ex.c
HAL_LIB_SRC += stm32f4xx_hal_uart.c
HAL_LIB_SRC += stm32f4xx_hal_pwr.c
HAL_LIB_SRC += stm32f4xx_hal_pwr_ex.c
HAL_LIB_SRC += stm32f4xx_hal_tim.c
HAL_LIB_SRC += stm32f4xx_hal_tim_ex.c
HAL_LIB_SRC += stm32f4xx_hal_rng.c

# Search paths for core module sources
vpath %.c $(PROJ_ROOT)/lib/platform: \
	$(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/lib/uart: \
	$(PROJ_ROOT)/lib/hwrng: \
	$(PROJ_ROOT)/lib/protocols/$(PROTOCOL_DIR): \
	$(PROJ_ROOT)/lib/cloud_comm: \
	$(PROJ_ROOT)/lib/network: \
	$(PROJ_ROOT)/lib/network/at/core: \
	$(PROJ_ROOT)/lib/network/at/smscodec: \
	$(PROJ_ROOT)/lib/network/at/$(MODEM_DIR): \
	$(STM32_PLIB): \
	$(STM32_CMSIS)/Source/Templates:
