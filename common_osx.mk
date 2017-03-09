# Copyright(C) 2016 Verizon. All rights reserved.

# Define common parameters used by the build process.
# This is used only for building test code that can also run on OS X.

# Make on OSX doesn't have this macro (?)
SIZE = size

ARCHFLAGS = -DBUILD_TARGET_OSX

LDFLAGS = -Wl,-map,fw.map

# No platform support files beyond the standard device drivers.
PLATFORM_SRC = 
PLATFORM_INC =

# Use native networking on this platform
MODEM_TARGET = none
