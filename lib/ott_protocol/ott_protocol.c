/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "uart.h"
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

static uint8_t msg_buf[MAX_MSG_SZ];	/* Buffer that holds received messages. */
static bool message_in_buffer;		/* Set when a message is received. */

/* XXX: Will reside in the AT layer. */
#define SEND_TIMEOUT_MS		2000
#define IDLE_CHARS		5
static void rx_cb(callback_event event)
{
	/* XXX: Will reside in the AT layer. */
}

ott_status ott_protocol_init(void)
{
	message_in_buffer = false;

	/* XXX: Will reside in the AT layer. */
	ASSERT(uart_module_init(UART_EN_HW_CTRL, IDLE_CHARS) == true);
	uart_set_rx_callback(rx_cb);

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_initiate_connection(void)
{
	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_close_connection(void)
{
	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

static bool flags_valid(c_flags_t c_flags)
{
	/* Return "true" if the combination of flags are valid. */
	return true;
}

ott_status ott_send_auth_to_cloud(c_flags_t c_flags,
                                  const uuid_t dev_id,
				  const bytes_t *dev_sec)
{
	if (!flags_valid(c_flags))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_status_to_cloud(c_flags_t c_flags, const bytes_t *status)
{
	if (!flags_valid(c_flags))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_ctrl_msg(c_flags_t c_flags)
{
	if (!flags_valid(c_flags))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_retrieve_msg(msg_t *msg)
{
	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}
