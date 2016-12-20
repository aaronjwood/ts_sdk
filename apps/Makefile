# Copyright(C) 2016 Verizon. All rights reserved.

# Build all the test and sample applications with their default configurations.

.PHONY: all clean

all:
	@for d in *; do \
		[ -d $$d ] && echo [Building in $$d] && $(MAKE) -C $$d all ; \
	done

clean:
	@for d in *; do \
		[ -d $$d ] && echo [Cleaning in $$d] && $(MAKE) -C $$d clean ; \
	done
