# Copyright(C) 2016 Verizon. All rights reserved.

PROJ_ROOT = $(shell git rev-parse --show-toplevel)
export PROJ_ROOT

ifneq (build,$(notdir $(CURDIR)))
# If not invoked in the build directory, change to that directory and
# re-invoke the Makefile with SRCDIR set.
include $(PROJ_ROOT)/tools/config/build_in_subdir.mk
else

.PHONY: all clean sdk-docs hal-docs

all: sdk-docs hal-docs

sdk-docs:
	(cd $(PROJ_ROOT); doxygen $(PROJ_ROOT)/Documents/doxygen/Doxyfile.sdk)

hal-docs:
	(cd $(PROJ_ROOT); doxygen $(PROJ_ROOT)/Documents/doxygen/Doxyfile.hal)

clean:
	rm -rf $(PROJ_ROOT)/Documents/doxygen/build

endif
