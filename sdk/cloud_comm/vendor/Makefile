# Copyright(C) 2016 Verizon. All rights reserved.

.PHONY: clean mbedtls paho_mqtt cJSON

VENDOR_ROOT=$(PROJ_ROOT)/sdk/cloud_comm/vendor

mbedtls:
	@echo "[Building mbedtls libraries.]"
	$(MAKE) -C $(VENDOR_ROOT)/mbedtls \
		CFLAGS='-O2 $(CHIPSET_CFLAGS) $(LTOFLAG) $(INC) $(ARCHFLAGS) $(MDEF) \
		-fdata-sections -ffunction-sections' \
		lib
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedcrypto.a $(SRCDIR)/build
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedtls.a $(SRCDIR)/build
	cp $(VENDOR_ROOT)/mbedtls/library/libmbedx509.a $(SRCDIR)/build

paho_mqtt:
	@echo "[Building the Paho MQTT library.]"
	$(MAKE) -C $(VENDOR_ROOT)/paho_mqtt -f Makefile.static \
		CC='$(CC)' AR='$(AR)' CFLAGS='$(CHIPSET_CFLAGS) -O2 $(ARCHFLAGS)'
	cp $(VENDOR_ROOT)/paho_mqtt/build/libpahomqtt.a $(SRCDIR)/build

cJSON:
	@echo "[Building the cJSON library.]"
	$(MAKE) -C $(VENDOR_ROOT)/cJSON static \
	CFLAGS='-g -ggdb3 $(ARCHFLAGS) $(MDEF)'
	cp $(VENDOR_ROOT)/cJSON/libcjson.a $(SRCDIR)/build
clean:
	@echo "[Cleaning vendor libraries.]"
	$(MAKE) -C $(VENDOR_ROOT)/mbedtls clean
	$(MAKE) -C $(VENDOR_ROOT)/paho_mqtt -f Makefile.static clean
	$(MAKE) -C $(VENDOR_ROOT)/cJSON clean

# Rebuild our version of the mbedtls config.h file from an unmodified copy.
# This is one-time configuration when we have a new version of mbedtls.
# Using the vendor-supplied perl script rather than editing by hand should
# make it easier to see what we are changing and why.
mbedtls_config:
	(cd mbedtls; ./vz_configure_mbedtls no-net)
