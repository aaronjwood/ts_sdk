# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Build all the test and sample applications with their default configurations.

DIRS=$(shell find . -type d -depth 1)
clean_DIRS=$(addprefix clean_,$(DIRS))

.PHONY: all clean $(DIRS) $(clean_DIRS)

all: $(DIRS)

clean: $(clean_DIRS)

$(DIRS):
	@echo [Building in $@] && $(MAKE) -C $@ all

$(clean_DIRS):
	@echo [Cleaning in $(patsubst clean_%,%,$@)] && \
		$(MAKE) -C $(patsubst clean_%,%,$@) clean
