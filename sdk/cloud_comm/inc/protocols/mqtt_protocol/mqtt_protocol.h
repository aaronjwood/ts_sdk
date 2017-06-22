/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef MQTT_PROTOCOL_H
#define MQTT_PROTOCOL_H

#include "paho_mqtt_port.h"

typedef struct auth_creds {
	size_t cl_cert_len;
	const unsigned char *cl_cert;
	size_t cl_key_len;
	const unsigned char *cl_key;
} auth_creds;

bool mqtt_net_init(Network *net, const auth_creds *creds);
int mqtt_net_connect(const char *addr, uint16_t port);
bool mqtt_net_disconnect(void);

#endif
