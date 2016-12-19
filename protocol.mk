# Copyright(C) 2016 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# protocol modules are built

ifeq ($(CLOUD_PROTOCOL),ott)
# Build for OTT protocol (which uses cloud_comm API)
PROTOCOL_SRC = ott_protocol.c cloud_comm.c

INC += -I $(PROJ_ROOT)/include/ott_protocol
INC += -I $(PROJ_ROOT)/include/cloud_comm

vpath %.c $(PROJ_ROOT)/lib/ott_protocol: \
	$(PROJ_ROOT)/lib/cloud_comm:

# OTT requires the mbed TLS library
VENDOR_INC += -I $(PROJ_ROOT)/vendor/mbedtls/include
VENDOR_LIB_DIRS += mbedtls
VENDOR_LIB_FLAGS += -L. -lmbedtls -lmbedx509 -lmbedcrypto

endif

