# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# modem and protocol is selected.

ifeq ($(PROTOCOL),OTT_PROTOCOL)
MODEM_PROTOCOL = tcp
NET_OS = at
else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
MODEM_PROTOCOL = sms
else ifeq ($(PROTOCOL),MQTT_PROTOCOL)
MODEM_PROTOCOL = tcp
ifeq ($(DEV_BOARD),$(filter $(DEV_BOARD),raspberry_pi3 virtual))
NET_OS = linux
else
NET_OS = at
endif
else ifeq ($(PROTOCOL),NO_PROTOCOL)
# Some lower-level test programs bypass the protocol layer.
# They may need to set MODEM_PROTOCOL and other variables.
else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

MODEM_INC_ROOT = $(SDK_ROOT)/inc/network/at
MODEM_SRC_ROOT = $(SDK_ROOT)/src/network

ifneq ($(MODEM_TARGET),none)
# Currently, all modems supporting TCP and SMS do so via AT commands.
NET_OS = at
ifeq ($(MODEM_PROTOCOL),$(filter $(MODEM_PROTOCOL),tcp sms))
MODEM_SRC += at_core.c uart_util.c at_modem.c
MODEM_INC += -I $(MODEM_INC_ROOT)
MODEM_INC += -I $(MODEM_SRC_ROOT)/at -I $(MODEM_SRC_ROOT)/at/core
endif

ifeq ($(MODEM_PROTOCOL),sms)
MODEM_SRC += smscodec.c
MODEM_INC += -I $(MODEM_SRC_ROOT)/at/smscodec
endif
endif

# All TCP currently uses "TCP over AT commands" for TLS.
# Provide the network abstraction module used by the TLS library.
# Only relevant if the vendor library "mbedtls" is included.
ifeq ($(MODEM_PROTOCOL),tcp)
ifneq (,$(findstring mbedtls,$(VENDOR_LIB_DIRS)))
MODEM_SRC += net_mbedtls_$(NET_OS).c
endif
endif

#
# Support for specific modems.
#
ifeq ($(MODEM_TARGET),toby201)
MODEM = MODEM_TOBY201
MODEM_SRC += at_toby201_$(MODEM_PROTOCOL).c
MODEM_DIR += at/$(MODEM_TARGET)/$(MODEM_PROTOCOL)
else ifeq ($(MODEM_TARGET),sqmonarch)
MODEM = MODEM_SQMONARCH
MODEM_SRC += at_sqmonarch_$(MODEM_PROTOCOL).c
MODEM_SRC += rbuf.c
MODEM_DIR += at/$(MODEM_TARGET)/$(MODEM_PROTOCOL)
else ifeq ($(MODEM_TARGET),none)
# Special case for some tests using native networking
MODEM = MODEM_NONE
else
$(error The MODEM_TARGET variable has an invalid value: $(MODEM_TARGET))
endif

vpath %.c  $(MODEM_SRC_ROOT): \
	$(MODEM_SRC_ROOT)/at/core: \
	$(MODEM_SRC_ROOT)/at/smscodec: \
	$(MODEM_SRC_ROOT)/$(MODEM_DIR): \
	$(MODEM_SRC_ROOT)/at/$(MODEM_TARGET):
