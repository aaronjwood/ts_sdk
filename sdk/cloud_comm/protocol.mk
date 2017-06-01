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
PROTOCOL_SRC =
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

vpath %.c $(PROTOCOLS_SRC_ROOT)/$(PROTOCOL_DIR):

# OTT and MQTT require the mbed TLS library
ifeq ($(PROTOCOL),$(filter $(PROTOCOL),OTT_PROTOCOL MQTT_PROTOCOL))
VENDOR_INC += -I $(SDK_ROOT)/vendor/mbedtls/include
VENDOR_LIB_DIRS += mbedtls
VENDOR_LIB_FLAGS += -L. -lmbedtls -lmbedx509 -lmbedcrypto
endif

# MQTT requires the Paho MQTT library
ifeq ($(PROTOCOL),MQTT_PROTOCOL)
VENDOR_INC += -I $(SDK_ROOT)/vendor/paho_mqtt/MQTTClient-C/src
VENDOR_INC += -DMQTTCLIENT_PLATFORM_HEADER=paho_port_generic.h \
	      -I $(SDK_ROOT)/vendor/paho_mqtt/MQTTClient-C/src/paho_port_generic
VENDOR_INC += -I $(SDK_ROOT)/vendor/paho_mqtt/MQTTPacket/src
VENDOR_LIB_DIRS += paho_mqtt
VENDOR_LIB_FLAGS += -L. -lpahomqtt
endif
