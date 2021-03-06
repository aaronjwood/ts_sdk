# Copyright(C) 2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# protocol is selected.

include $(SDK_ROOT)/protocol.mk
include $(SDK_ROOT)/modem.mk

SDK_INC += $(PROTOCOL_INC)
SDK_INC += $(MODEM_INC)

# Header files for the cloud_comm API
SDK_INC += -I $(SDK_ROOT)/api

# Header files for services
SDK_INC += -I $(SDK_ROOT)/inc
SDK_INC += -I $(SDK_ROOT)/inc/services

# Header files for vendor libraries
SDK_INC += $(VENDOR_INC)

# Source for the main cloud API
CLOUD_COMM_SRC ?= cloud_comm.c

# Source for the standard services.
# An application may append to this variable if it uses additional services.
SERVICES_SRC ?= cc_basic_service.c cc_control_service.c

SDK_SRC += $(MODEM_SRC) $(PROTOCOL_SRC) $(CLOUD_COMM_SRC) $(SERVICES_SRC)

CFLAGS_SDK += $(MODEM_CFLAGS) $(PROTOCOL_CFLAGS)
export CFLAGS_SDK
export SDK_SRC
export SDK_INC

# Search paths for device driver sources
vpath %.c $(SDK_ROOT)/src/cloud_comm_api: \
	$(SDK_ROOT)/src/services:
