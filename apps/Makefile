# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Build all the test and sample applications with their default configurations.
#
# Override one or more of the variables below to build all apps for different
# build targets (not all combinations are relevant to all apps).
#     make PROTOCOL=target-protocol
#     make MODEM_TARGET=target-modem
#     make BUILD_TARGET=target-mcu

DIRS=$(shell find . -maxdepth 1 -mindepth 1 -type d)
clean_DIRS=$(addprefix clean_,$(DIRS))

.PHONY: all clean $(DIRS) $(clean_DIRS)

all: $(DIRS)

clean: $(clean_DIRS)

$(DIRS):
	@echo [Building in $@] && $(MAKE) -C $@ all

$(clean_DIRS):
	@echo [Cleaning in $(patsubst clean_%,%,$@)] && \
		$(MAKE) -C $(patsubst clean_%,%,$@) clean
