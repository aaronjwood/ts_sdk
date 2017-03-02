# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# modem and protocol is selected.

ifeq ($(PROTOCOL),OTT_PROTOCOL)
MODEM_PROTOCOL = tcp
else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
MODEM_PROTOCOL = sms
else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

# Currently, all modems supporting OTT and SMSNAS do so via AT commands.
ifeq ($(PROTOCOL),$(filter $(PROTOCOL),OTT_PROTOCOL SMSNAS_PROTOCOL))
MODEM_SRC += at_core.c 
INC += -I $(PROJ_ROOT)/include/network/at
INC += -I $(PROJ_ROOT)/lib/network/at -I $(PROJ_ROOT)/lib/network/at/core
endif

# All OTT implementions currently use "TCP over AT commands" for TLS.
# Provide the network abstraction library used by the TLS library.
ifeq ($(PROTOCOL),OTT_PROTOCOL)
MODEM_SRC += net_mbedtls_at.c
endif

ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
MODEM_SRC += smscodec.c
INC += $(PROJ_ROOT)/lib/network/at/smscodec
endif

#
# Support for specific modems.
#
ifeq ($(MODEM_TARGET),toby201)
	MODEM = MODEM_TOBY201
	MODEM_SRC += at_toby201_$(MODEM_PROTOCOL).c
	MODEM_DIR += at/$(MODEM_TARGET)/$(MODEM_PROTOCOL)
else
$(error The MODEM_TARGET variable has an invalid value: $(MODEM_TARGET))
endif

vpath %.c  $(PROJ_ROOT)/lib/network: \
	$(PROJ_ROOT)/lib/network/at/core: \
	$(PROJ_ROOT)/lib/network/$(MODEM_DIR):


