# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# protocol modules are built

ifeq ($(PROTOCOL),OTT_PROTOCOL)
PROTOCOL_SRC = ott_protocol.c
PROTOCOL_DIR = ott_protocol
PROTOCOL_INC = ott_protocol
else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
PROTOCOL_SRC = smsnas_protocol.c
PROTOCOL_DIR = smsnas_protocol
PROTOCOL_INC = smsnas_protocol
else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif
export PROTOCOL

INC += -I $(PROJ_ROOT)/include/protocols
INC += -I $(PROJ_ROOT)/include/protocols/$(PROTOCOL_INC)

vpath %.c $(PROJ_ROOT)/lib/protocol/$(PROTOCOL_DIR):

# OTT requires the mbed TLS library
ifeq ($(PROTOCOL),OTT_PROTOCOL)
VENDOR_INC += -I $(PROJ_ROOT)/vendor/mbedtls/include
VENDOR_LIB_DIRS += mbedtls
VENDOR_LIB_FLAGS += -L. -lmbedtls -lmbedx509 -lmbedcrypto
endif

