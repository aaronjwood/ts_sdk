# Copyright(C) 2016 Verizon. All rights reserved.

# Define common parameters used by the build process.
# This is used only for building test code that can also run on OS X.

# Make on OSX doesn't have this macro (?)
SIZE = size

ARCHFLAGS = -DBUILD_TARGET_OSX

LDFLAGS = -Wl,-map,fw.map

# Based on BUILD_TARGET, it will select appropriate file from platform.mk
PLATFORM_INC =

# Use native networking on this platform
MODEM_TARGET = none
