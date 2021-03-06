# Copyright(C) 2016,2017 Verizon. All rights reserved.

# Makefile variables to select the necessary files depending on which
# protocol is selected.

ifeq ($(PROTOCOL),OTT_PROTOCOL)
PROTOCOL_SRC = ott_protocol.c
PROTOCOL_DIR = ott_protocol
PROTOCOL_INC_DIR = ott_protocol

else ifeq ($(PROTOCOL),SMSNAS_PROTOCOL)
PROTOCOL_SRC = smsnas_protocol.c
PROTOCOL_DIR = smsnas_protocol
PROTOCOL_INC_DIR = smsnas_protocol

else ifeq ($(PROTOCOL),MQTT_PROTOCOL)
PROTOCOL_SRC = mqtt_protocol.c
PROTOCOL_SRC += mqtt_timer_utils.c
PROTOCOL_DIR = mqtt_protocol
PROTOCOL_INC_DIR = mqtt_protocol

else ifeq ($(PROTOCOL),NO_PROTOCOL)
# Some lower-level test programs bypass the protocol layer.
# They may need to set additional variables.

else
$(error The PROTOCOL variable has an invalid value: $(PROTOCOL))
endif

PROTOCOLS_INC_ROOT = $(SDK_ROOT)/inc/protocols
PROTOCOLS_SRC_ROOT = $(SDK_ROOT)/src/protocols

PROTOCOL_INC += -I $(PROTOCOLS_INC_ROOT)
PROTOCOL_INC += -I $(PROTOCOLS_INC_ROOT)/$(PROTOCOL_INC_DIR)
PROTOCOL_INC += -I $(PROTOCOLS_SRC_ROOT)/$(PROTOCOL_DIR)

vpath %.c $(PROTOCOLS_SRC_ROOT)/$(PROTOCOL_DIR):

# OTT and MQTT require the mbed TLS library
ifeq ($(PROTOCOL),$(filter $(PROTOCOL),OTT_PROTOCOL MQTT_PROTOCOL))
VENDOR_INC += -I $(SDK_ROOT)/vendor/mbedtls/include
# raspberry_pi3 and linux based virtual devices use same code base
ifeq ($(DEV_BOARD),$(filter $(DEV_BOARD),raspberry_pi3 virtual))
DEV_BOARD_MOD = raspberry_pi3
else
DEV_BOARD_MOD = $(DEV_BOARD)
endif
VENDOR_INC += -DMBEDTLS_CONFIG_FILE="\"mbedtls/config_$(DEV_BOARD_MOD).h\""
VENDOR_LIB_DIRS += mbedtls
VENDOR_LIB_FLAGS += -L. -lmbedtls -lmbedx509 -lmbedcrypto
endif

# MQTT requires the Paho MQTT library. It uses -isystem instead of -I to avoid
# checking them with -Wall and -Werror.
ifeq ($(PROTOCOL),MQTT_PROTOCOL)
VENDOR_INC += -isystem $(SDK_ROOT)/vendor/paho_mqtt/MQTTClient-C/src
VENDOR_INC += -DMQTTCLIENT_PLATFORM_HEADER=paho_mqtt_port.h
VENDOR_INC += -isystem $(SDK_ROOT)/vendor/paho_mqtt/MQTTPacket/src
VENDOR_LIB_DIRS += paho_mqtt
VENDOR_LIB_FLAGS += -L. -lpahomqtt
endif

#ifeq ($(PROTOCOL),$(filter $(PROTOCOL),OTT_PROTOCOL MQTT_PROTOCOL))
# add cjson dependency
VENDOR_INC += -I $(SDK_ROOT)/vendor/cJSON/
VENDOR_LIB_DIRS += cJSON
VENDOR_LIB_FLAGS += -L. -lcjson

# add cbor dependency
VENDOR_INC += -I $(SDK_ROOT)/vendor/tinycbor/src
VENDOR_LIB_DIRS += tinycbor
VENDOR_LIB_FLAGS += -L. -ltinycbor
#endif
