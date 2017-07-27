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
RANLIB = $(GCC_ROOT)/bin/arm-linux-gnueabihf-ranlib
export CC AR LD OBJDUMP OBJCOPY SIZE RANLIB

# Machine specific compiler, assembler settings and Linker script
ARCHFLAGS =
MDEF =
export MDEF
export ARCHFLAGS

CHIPSET_INC =

CHIPSET_LDFLAGS = -lrt
CHIPSET_CFLAGS = --sysroot=/ts_sdk_bldenv/toolchain/arm-linux-gnueabihf/libc
export CHIPSET_LDFLAGS
export CHIPSET_CFLAGS

FW_EXEC = firmware
