# Copyright(C) 2016 Verizon. All rights reserved.

.PHONY: clean mbedtls

VENDOR_ROOT=$(PROJ_ROOT)/vendor

mbedtls:
	@echo "[Building mbedtls libraries.}"
	$(MAKE) -C $(VENDOR_ROOT)/mbedtls \
		CFLAGS='-O2 $(LTOFLAG) $(INC) $(ARCHFLAGS) $(MDEF) \
		-fdata-sections -ffunction-sections' \
		lib
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedcrypto.a $(SRCDIR)/build
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedtls.a $(SRCDIR)/build
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedx509.a $(SRCDIR)/build

clean:
	@echo "[Cleaning vendor libraries.]"
	$(MAKE) -C $(VENDOR_ROOT)/mbedtls clean

# Rebuild our version of the mbedtls config.h file from an unmodified copy.
# This is one-time configuration when we have a new version of mbedtls.
# Using the vendor-supplied perl script rather than editing by hand should
# make it easier to see what we are changing and why.
mbedtls_config:
	(cd $(VENDOR_ROOT)/mbedtls; ./vz_configure_mbedtls)
