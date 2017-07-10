# Copyright(C) 2017 Verizon. All rights reserved.

# Define common parameters used by the build process for raspberry_pi3 dev kit.
# This includes paths to the toolchain, compiler and linker flags, and the
# names of the core modules needed.
# Paths to toolchain and HAL root if any

GCC_ROOT = /ts_sdk_bldenv/toolchain

# Compiler, assembler, object code dumper and object code section copier
CC = $(GCC_ROOT)/bin/arm-linux-gnueabihf-gcc
AR = $(GCC_ROOT)/bin/arm-linux-gnueabihf-ar
LD = $(GCC_ROOT)/bin/arm-linux-gnueabihf-ld
OBJDUMP = $(GCC_ROOT)/bin/arm-linux-gnueabihf-objdump
OBJCOPY = $(GCC_ROOT)/bin/arm-linux-gnueabihf-objcopy
SIZE = $(GCC_ROOT)/bin/arm-linux-gnueabihf-size
export CC AR LD OBJDUMP OBJCOPY SIZE

# Machine specific compiler, assembler settings and Linker script
ARCHFLAGS =
MDEF =
export MDEF
export ARCHFLAGS

CHIPSET_INC =

CHIPSET_LDFLAGS = -lrt
export CHIPSET_LDFLAGS

FW_EXEC = firmware
