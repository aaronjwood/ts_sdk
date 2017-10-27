/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef PROTO_INIT_H
#define PROTO_INIT_H

#if defined (OTT_PROTOCOL)
#define SEND_DATA_SZ    22
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
#define SEND_DATA_SZ   22
#else
#error "define valid protocol options from OTT_PROTOCOL \
	SMSNAS_PROTOCOL"
#endif

#endif
