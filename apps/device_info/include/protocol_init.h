/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef PROTO_INIT_H
#define PROTO_INIT_H

#if defined (MQTT_PROTOCOL)

#define REMOTE_HOST	"68.128.212.248:8883"
#include "client-crt-1801.h"
#include "client-key-1801.h"
#include "simpm-ea-iwk-server.h"

static const uint8_t *cl_cred = client_cert;
static const uint8_t *cl_sec_key = client_key;
static const uint8_t *cacert = cacert_buf;

#define CL_CRED_SZ	sizeof(client_cert)
#define CL_SEC_KEY_SZ	sizeof(client_key)
#define CA_CRED_SZ	sizeof(cacert_buf)
#else
#error "Define MQTT_PROTOCOL as a protocol option"
#endif

#endif
