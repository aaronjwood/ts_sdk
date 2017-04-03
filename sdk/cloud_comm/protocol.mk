# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# protocol is selected.

ifeq ($(PROTOCOL),OTT_PROTOCOL)
PROTOCOL_SRC = ott_protocol.c
PROTOCOL_DIR = ott_protocol
PROTOCOL_INC = ott_protocol

else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
PROTOCOL_SRC = smsnas_protocol.c
PROTOCOL_DIR = smsnas_protocol
PROTOCOL_INC = smsnas_protocol

else ifeq ($(PROTOCOL),NO_PROTOCOL)
# Some lower-level test programs bypass the protocol layer.
# They may need to set additional variables.

else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

export PROTOCOL

PROTOCOLS_INC_ROOT = $(SDK_ROOT)/inc/protocols
PROTOCOLS_SRC_ROOT = $(SDK_ROOT)/src/protocols

INC += -I $(PROTOCOLS_INC_ROOT)
INC += -I $(PROTOCOLS_INC_ROOT)/$(PROTOCOL_INC)

vpath %.c $(PROTOCOLS_SRC_ROOT)/$(PROTOCOL_DIR):

# OTT requires the mbed TLS library
ifeq ($(PROTOCOL),OTT_PROTOCOL)
VENDOR_INC += -I $(PROJ_ROOT)/vendor/mbedtls/include
VENDOR_LIB_DIRS += mbedtls
VENDOR_LIB_FLAGS += -L. -lmbedtls -lmbedx509 -lmbedcrypto
endif
