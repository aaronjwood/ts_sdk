# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# modem and protocol is selected.

ifeq ($(PROTOCOL),OTT_PROTOCOL)
MODEM_PROTOCOL = tcp
else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
MODEM_PROTOCOL = sms
else ifeq ($(PROTOCOL),NO_PROTOCOL)
# Some lower-level test programs bypass the protocol layer.
# They may need to set MODEM_PROTOCOL and other variables.
else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

ifneq ($(MODEM_TARGET),none)
# Currently, all modems supporting TCP and SMS do so via AT commands.
ifeq ($(MODEM_PROTOCOL),$(filter $(MODEM_PROTOCOL),tcp sms))
MODEM_SRC += at_core.c
INC += -I $(PROJ_ROOT)/include/network/at
INC += -I $(PROJ_ROOT)/lib/network/at -I $(PROJ_ROOT)/lib/network/at/core
endif

# All TCP currently uses "TCP over AT commands" for TLS.
# Provide the network abstraction module used by the TLS library.
ifeq ($(MODEM_PROTOCOL),tcp)
MODEM_SRC += net_mbedtls_at.c
VENDOR_INC += -I $(PROJ_ROOT)/vendor/mbedtls/include
endif

ifeq ($(MODEM_PROTOCOL),sms)
MODEM_SRC += smscodec.c
INC += -I $(PROJ_ROOT)/lib/network/at/smscodec
endif
endif

#
# Support for specific modems.
#
ifeq ($(MODEM_TARGET),toby201)
MODEM = MODEM_TOBY201
MODEM_SRC += at_toby201_$(MODEM_PROTOCOL).c
MODEM_DIR += at/$(MODEM_TARGET)/$(MODEM_PROTOCOL)
else ifeq ($(MODEM_TARGET),none)
# Special case for some tests using native networking
MODEM = none
else
$(error The MODEM_TARGET variable has an invalid value: $(MODEM_TARGET))
endif

vpath %.c  $(PROJ_ROOT)/lib/network: \
	$(PROJ_ROOT)/lib/network/at/core: \
	$(PROJ_ROOT)/lib/network/at/smscodec: \
	$(PROJ_ROOT)/lib/network/$(MODEM_DIR):


