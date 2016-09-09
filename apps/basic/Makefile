# Copyright(C) 2016 Verizon. All rights reserved.

# Makefile for the OTT Reference firmware for the STM32F4 platform.
# Uses the GCC toolchain and the STMicro STM32CubeF4 HAL drivers.
# Note the library sources are built without debug info and optimized for size

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include build_in_subdir.mk
else

PROJ_ROOT = $(shell git rev-parse --show-toplevel)
TOOLS_ROOT = $(PROJ_ROOT)/tools/installed/stm32f4
GCC_ROOT = $(TOOLS_ROOT)/gcc-arm-none-eabi-5_4-2016q2
STM32_LIB_COMMON = $(TOOLS_ROOT)/STM32Cube_FW_F4_V1.13.0

# Source paths
STM32_PLIB = $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Src
STM32_CMSIS = $(STM32_LIB_COMMON)/Drivers/CMSIS/Device/ST/STM32F4xx

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-none-eabi-gcc
AS = $(GCC_ROOT)/bin/arm-none-eabi-as
OBJDUMP = $(GCC_ROOT)/bin/arm-none-eabi-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-none-eabi-objcopy
SIZE = $(GCC_ROOT)/bin/arm-none-eabi-size

# Debug and optimization flags
DBG_OP_USER_FLAGS = -g -ggdb3 -O0
DBG_OP_LIB_FLAGS = -Os

# The following invokes an unused sections garbage collector
NOSYSLIB =  -Wl,--gc-sections -Wl,--as-needed --specs=nosys.specs

# Machine specific compiler and assembler settings
MFLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
MDEF = -DSTM32F429xx

# Firmware name
FW_EXEC = firmware.elf

# OTT services library component headers
INC = -I $(PROJ_ROOT)/include/dbg

# App specific include
INC += -I $(SRCDIR)/include

# Peripheral related headers
INC += -I $(STM32_LIB_COMMON)/Drivers/STM32F4xx_HAL_Driver/Inc

# CMSIS (Core) headers
INC += -I $(STM32_LIB_COMMON)/Drivers/CMSIS/Include

# CMSIS (Device specific) headers
INC += -I $(STM32_CMSIS)/Include

# Standard library function headers
INC += -I $(GCC_ROOT)/include

# Linker script
LDSCRIPT = $(STM32_LIB_COMMON)/Projects/STM32F429ZI-Nucleo/Templates/SW4STM32/STM32F429ZI_Nucleo_144/STM32F429ZITx_FLASH.ld

# Point to the C Runtime startup code
STARTUP_SRC = $(STM32_CMSIS)/Source/Templates/gcc/startup_stm32f429xx.s
OBJ_STARTUP = startup_stm32f429xx.o

# Use 'vpath' to search specific directories for library and user sources
vpath %.c $(SRCDIR): \
	$(PROJ_ROOT)/lib/dbg: \
	$(PROJ_ROOT)/vendor: \
	$(STM32_PLIB): \
	$(STM32_CMSIS)/Source/Templates:

# List of core library components to be included in the build process.
# This includes debugging facilities.
CORELIB_SRC = stm32f4xx_hal.c system_stm32f4xx.c dbg.c

# List of OTT service library components
OTT_SRC =

# Peripheral list
LIB_SRC += stm32f4xx_hal_cortex.c
LIB_SRC += stm32f4xx_hal_gpio.c
LIB_SRC += stm32f4xx_hal_rcc.c
LIB_SRC += stm32f4xx_hal_rcc_ex.c
LIB_SRC += stm32f4xx_hal_uart.c
LIB_SRC += stm32f4xx_hal_pwr.c
LIB_SRC += stm32f4xx_hal_pwr_ex.c

# Use DBG_LIB_SRC to compile a subset of the peripheral library sources with the
# debug flag enabled
# Eg: DBG_LIB_SRC = stm32l0xx_hal_uart.c stm32l0xx_hal_uart_ex.c
DBG_LIB_SRC =

# Remove files common to both LIB_SRC and DBG_LIB_SRC from LIB_SRC
LIB_SRC := $(filter-out $(DBG_LIB_SRC), $(LIB_SRC))

# User application source
USER_SRC = $(wildcard $(SRCDIR)/*.c)

OBJ_USER = $(addsuffix .o, $(basename $(notdir $(USER_SRC))))
OBJ_LIB = $(addsuffix .o, $(basename $(CORELIB_SRC)))
OBJ_LIB += $(addsuffix .o, $(basename $(LIB_SRC)))
OBJ_DBG_LIB = $(addsuffix .o, $(basename $(DBG_LIB_SRC)))

OBJ = $(OBJ_USER) $(OBJ_LIB) $(OBJ_DBG_LIB)

CFLAGS_USER = -Wall -Werror -std=c99 -Wcast-align $(INC) $(DBG_OP_USER_FLAGS)
CFLAGS_LIB = -Werror -std=c99 $(INC) $(DBG_OP_LIB_FLAGS) -fdata-sections \
	     -ffunction-sections
CFLAGS_DBG_LIB = -Werror -std=c99 $(INC) $(DBG_OP_USER_FLAGS) -fdata-sections \
		 -ffunction-sections

#==================================RULES=======================================#
.PHONY: all build dump bin clean upload

all build: $(FW_EXEC)
	$(SIZE) $(FW_EXEC)

$(FW_EXEC): $(OBJ) $(OBJ_STARTUP)
	$(CC) -Wl,-Map,fw.map,--cref $(NOSYSLIB) $(INC) -Os $(MFLAGS) -T $(LDSCRIPT) \
		$(OBJ) $(OBJ_STARTUP) -o $(FW_EXEC)

$(OBJ_STARTUP):
	$(AS) $(MFLAGS) -o $(OBJ_STARTUP) $(STARTUP_SRC)

$(OBJ_LIB): %.o: %.c
	$(CC) -c $(CFLAGS_LIB) $(MFLAGS) $(MDEF) $< -o $@

$(OBJ_DBG_LIB): %.o: %.c
	$(CC) -c $(CFLAGS_DBG_LIB) $(MFLAGS) $(MDEF) $< -o $@

$(OBJ_USER): %.o: %.c
	$(CC) -c $(CFLAGS_USER) $(MFLAGS) $(MDEF) $< -o $@

clean:
	rm -rf $(SRCDIR)/build

dump: $(FW_EXEC)
	$(OBJDUMP) -DsS $(FW_EXEC) > dump.s

bin: $(FW_EXEC)
	$(OBJCOPY) -O binary $(FW_EXEC) fw.bin
	$(OBJCOPY) -O ihex $(FW_EXEC) fw.hex

upload: $(FW_EXEC)
	openocd -f $(TOOLS_ROOT)/openocdcfg-stm32f4/board/st_nucleo_f4.cfg \
		-c init -c "reset halt" \
		-c "flash write_image erase $(FW_EXEC)" \
		-c reset -c shutdown
endif
