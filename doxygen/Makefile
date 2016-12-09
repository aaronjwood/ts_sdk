# Copyright(C) 2016 Verizon. All rights reserved.

PROJ_ROOT = $(shell git rev-parse --show-toplevel)
export PROJ_ROOT

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(PROJ_ROOT)/build_in_subdir.mk
else

.PHONY: all clean

all:
	(cd $(PROJ_ROOT); doxygen $(PROJ_ROOT)/doxygen/Doxyfile)

clean:

endif
