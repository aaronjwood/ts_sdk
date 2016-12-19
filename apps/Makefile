# Copyright(C) 2016 Verizon. All rights reserved.

# Build all the test and sample applications with their default configurations.

.PHONY: all clean

all:
	for f in *; do \
		[ -d $$f ] && echo "[Building in $$f]" && (cd $$f; make); \
	done

clean:
	@for f in *; do \
		[ -d $$f ] && echo "[Cleaning in $$f]" && (cd $$f; make clean); \
	done
