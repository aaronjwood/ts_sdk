# Copyright(C) 2017 Verizon. All rights reserved.

# Define common parameters used by the build process for linux virtual devices
# This includes paths to the toolchain, compiler and linker flags

# Compiler, assembler, object code dumper and object code section copier
CC = gcc
AR = ar
LD = ld
OBJDUMP = objdump
OBJCOPY = objcopy
SIZE = size
RANLIB = ranlib
export CC AR LD OBJDUMP OBJCOPY SIZE RANLIB

# Machine specific compiler, assembler settings and Linker script
ARCHFLAGS =
MDEF =
export MDEF
export ARCHFLAGS

CHIPSET_INC =

CHIPSET_LDFLAGS = -lrt
export CHIPSET_LDFLAGS
export CHIPSET_CFLAGS

FW_EXEC = firmware
