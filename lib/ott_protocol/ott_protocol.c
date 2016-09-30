/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "uart.h"
#include "ott_protocol.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"

#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

static uint8_t msg_buf[MAX_MSG_SZ];	/* Buffer that holds received messages. */
static bool message_in_buffer;		/* Set when a message is received. */

static mbedtls_net_context server_fd;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config ssl_config;
static mbedtls_x509_crt cacert;
static const char *pers_data = "ott_protocol_test";

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

	/* Replace the net layer with our own implementation. */
	//mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&ssl_config);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	int ret;
	ott_status s = OTT_OK;
	dbg_printf("Seeding the RNG\n");
	mbedtls_entropy_init(&entropy);

	//if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
	//				 &entropy, (const unsigned char *)pers_data,
	//				 strlen(pers_data))) != 0) {
	//	dbg_printf("Failed: mbedtls_ctr_drbg_seed returned %d\n", ret);
	//	s = OTT_ERROR;
	//	goto cleanup;
	//}

	//if ((ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)
	//				  mbedtls_test_cas_pem,
	//				  mbedtls_test_cas_pem_len)) < 0) {
	//	dbg_printf("Failed: mbedtls_x509_crt_parse returned %d\n", ret);
	//	s = OTT_ERROR;
	//	goto cleanup;
	//}

cleanup://
	///* Free network interface resources. */
	////mbedtls_net_free(&server_fd);
	//mbedtls_ssl_free(&ssl);
	//mbedtls_ssl_config_free(&ssl_config);
	//mbedtls_ctr_drbg_free(&ctr_drbg);
	//mbedtls_entropy_free(&entropy);
	return s;
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

static inline bool flags_valid(c_flags_t c_flags)
{
	/* Return "true" if the combination of flags are valid. */
	if (c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK))
		return false;
	return true;
}

ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uuid_t dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if (c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK))
		return OTT_ERROR;

	/*
	 * All data must fit within the remainder of the
	 * message after accounting for overhead bytes.
	 */
	if (dev_sec_sz + UUID_SZ > MAX_DATA_SZ)
		return OTT_ERROR;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if (c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK))
		return OTT_ERROR;

	/*
	 * All data must fit within the remainder of the
	 * message after accounting for overhead bytes.
	 */
	if (status_sz > MAX_DATA_SZ)
		return OTT_ERROR;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_ctrl_msg(c_flags_t c_flags)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if (c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK))
		return OTT_ERROR;

	/* The pending flag has no meaning for this type of message. */
	if (c_flags == CF_PENDING)
		return OTT_ERROR;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_retrieve_msg(msg_t *msg)
{
	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
	return 0;
}
