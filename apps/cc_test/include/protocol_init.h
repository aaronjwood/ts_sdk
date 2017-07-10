/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef PROTO_INIT_H
#define PROTO_INIT_H

/* Enable this macro to test concatenated sms send */
/*#define CONCAT_SMS*/

#if defined (OTT_PROTOCOL)
#define REMOTE_HOST	"iwk.ott.thingspace.verizon.com:443"
#define STAT_SZ_BYTES	10
#include "dev_creds.h"
/* Certificate that is used with the OTT services */
#include "verizon_ott_ca.h"
static const uint8_t *cl_cred = d_ID;
static const uint8_t *cl_sec_key = d_sec;
static const uint8_t *cacert = cacert_der;

#define CL_CRED_SZ	sizeof(d_ID)
#define CL_SEC_KEY_SZ	sizeof(d_sec)
#define CA_CRED_SZ	sizeof(cacert_der)

#elif defined (SMSNAS_PROTOCOL)
#define REMOTE_HOST	"+12345678912"
static const uint8_t *cl_cred;
static const uint8_t *cl_sec_key;
static const uint8_t *cacert;

#define CL_CRED_SZ	0
#define CL_SEC_KEY_SZ	0
#define CA_CRED_SZ	0

#if defined (CONCAT_SMS)
/* Approximate large size to send concatnated size */
#define STAT_SZ_BYTES	500
#define FIRST_SEG_SZ	130
#define SEC_SEG_SZ	134
#define THIRD_SEG_SZ	134
#define FOURTH_SEG_SZ	(STAT_SZ_BYTES - (FIRST_SEG_SZ + SEC_SEG_SZ + THIRD_SEG_SZ))
#else
#define STAT_SZ_BYTES	10
#endif

#elif defined (MQTT_PROTOCOL)

#define REMOTE_HOST	"simpm-ea-iwk.thingspace.verizon.com:8883"
#define STAT_SZ_BYTES	10
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
#error "define valid protocol options from OTT_PROTOCOL/SMSNAS_PROTOCOL/MQTT_PROTOCOL"
#endif

#endif
