# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which networking
# module is being built.

ifeq ($(NET_TYPE),at)
# Build with networking via LTE modem and AT commands to support mbedtls.
CORELIB_SRC += net_mbedtls_at.c $(MODEM_SRC)

INC += -I $(PROJ_ROOT)/include/network/at
INC += -I $(PROJ_ROOT)/include/network

vpath %.c $(PROJ_ROOT)/lib/network: \
	$(PROJ_ROOT)/lib/network/at/$(MODEM_DIR):
endif
