/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "ott_protocol.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#define MAX_MSG_SZ		512
#define VER_SZ			1
#define CMD_SZ			1
#define LEN_SZ			2
#define MAX_DATA_SZ		(MAX_MSG_SZ - VER_SZ - CMD_SZ - LEN_SZ)

static uint8_t msg_buf[MAX_MSG_SZ];
static bool message_in_buffer;

ott_status ott_protocol_init(void)
{
	message_in_buffer = false;
	return OTT_OK;
}
