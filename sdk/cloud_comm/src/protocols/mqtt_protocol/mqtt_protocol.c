/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>

#include "mqtt_protocol.h"
#include "sys.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/pk.h"

#ifdef MBEDTLS_DEBUG_C
#include "dbg.h"
#include "mbedtls/debug.h"
#endif

/* Certificate used for the TLS connection */
#include "simpm-ea-iwk-server.h"

#ifdef MBEDTLS_DEBUG_C
static void my_debug(void *ctx, int level,
                     const char *file, int line,
	     const char *str)
{
	(void)(level);
	dbg_printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}
#endif

#define TIMEOUT_MS			5000

#ifdef CALC_TLS_OVRHD_BYTES
#include "dbg.h"
extern bool ovrhd_profile_flag;
extern uint32_t num_bytes_recvd;
extern uint32_t num_bytes_sent;
#define START_CALC_OVRHD_BYTES()	ovrhd_profile_flag = true
#define STOP_CALC_OVRHD_BYTES()		ovrhd_profile_flag = false
#define PRINT_OVRHD_SENT()		dbg_printf("OVS:%"PRIu32"\n", num_bytes_sent);
#define PRINT_OVRHD_RECVD()		dbg_printf("OVR:%"PRIu32"\n", num_bytes_recvd);
#else
#define START_CALC_OVRHD_BYTES()
#define STOP_CALC_OVRHD_BYTES()
#define PRINT_OVRHD_SENT()
#define PRINT_OVRHD_RECVD()
#endif	/* CALC_TLS_OVRHD_BYTES */

/* mbedTLS specific variables */
static mbedtls_net_context ctx;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
static mbedtls_x509_crt cacert;
static mbedtls_x509_crt cl_cert;
static mbedtls_pk_context cl_key;

static inline void cleanup_mbedtls(void)
{
	/* Free network interface resources. */
	mbedtls_net_free(&ctx);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_x509_crt_free(&cl_cert);
	mbedtls_pk_free(&cl_key);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

void TimerInit(Timer *timer)
{
	timer->end_time = 0;
}

char TimerIsExpired(Timer *timer)
{
	uint64_t now = sys_get_tick_ms();
	return timer->end_time <= now;
}

void TimerCountdownMS(Timer *timer, unsigned int ms)
{
	uint64_t now = sys_get_tick_ms();
	timer->end_time = now + ms;
}

void TimerCountdown(Timer *timer, unsigned int sec)
{
	uint64_t now = sys_get_tick_ms();
	timer->end_time = now + sec * 1000;
}

int TimerLeftMS(Timer *timer)
{
	uint64_t now = sys_get_tick_ms();
	return timer->end_time <= now ? 0 : (int)(timer->end_time - now);
}

/*
 * Attempt to read 'len' bytes from the TCP/TLS stream into buffer 'b' within
 * 'timeout_ms' ms.
 */
static int read_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int recvd = mbedtls_ssl_read(&ssl, b + nbytes, len - nbytes);
		if (recvd == 0)
			return 0;
		if (recvd < 0 && recvd != MBEDTLS_ERR_SSL_WANT_READ &&
				recvd != MBEDTLS_ERR_SSL_WANT_WRITE)
			return -1;
		if (recvd > 0)
			nbytes += recvd;
	} while (nbytes < len && sys_get_tick_ms() - start_time < timeout_ms);
	return nbytes;
}

/*
 * Attempt writing to TCP/TLS stream a buffer 'b' of length 'len' within
 * 'timeout_ms' ms. Return -1 for an error, a positive number for the number of
 * bytes actually written.
 */
static int write_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int ret = mbedtls_ssl_write(&ssl, b + nbytes, len - nbytes);
		if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return -1;
		}
		if (ret >= 0)
			nbytes += ret;
	} while (nbytes < len && sys_get_tick_ms() - start_time < timeout_ms);
	return nbytes;
}

static const char pers[] = "mqtt_ts_sdk";
static bool init_tls(const auth_creds *creds)
{
#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif

	/* Initialize TLS structures */
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_x509_crt_init(&cl_cert);
	mbedtls_pk_init(&cl_key);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	/* Seed the RNG */
	mbedtls_entropy_init(&entropy);
	if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				(const unsigned char *)pers, strlen(pers)) != 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the CA root certificate */
	if (mbedtls_x509_crt_parse_der(&cacert, cacert_buf,
					 sizeof(cacert_buf)) < 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the client certificate */
	if (mbedtls_x509_crt_parse_der(&cl_cert, creds->cl_cert,
					 creds->cl_cert_len) < 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the client key */
	if (mbedtls_pk_parse_key(&cl_key, creds->cl_key, creds->cl_key_len,
			NULL, 0) != 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Set up the TLS structures */
	if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
		cleanup_mbedtls();
		return false;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif

	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	if (mbedtls_ssl_conf_own_cert(&conf, &cl_cert, &cl_key) != 0) {
		cleanup_mbedtls();
		return false;
	}
	return true;
}

bool mqtt_net_init(Network *net, const auth_creds *creds)
{
	mbedtls_net_init(&ctx);
	if (!init_tls(creds))
		return false;
	net->mqttread = read_fn;
	net->mqttwrite = write_fn;
	return true;
}

int mqtt_net_connect(const char *addr, uint16_t port)
{
	char port_str[6];
	snprintf(port_str, 6, "%d", port);

	int ret = 0;
	START_CALC_OVRHD_BYTES();
	/* Connect to the cloud services over TCP */
	if (mbedtls_net_connect(&ctx, addr, port_str, MBEDTLS_NET_PROTO_TCP) < 0) {
		ret = -1;
		goto exit_func;
	}

	/* Set up the SSL context */
	if (mbedtls_ssl_setup(&ssl, &conf) != 0) {
		mbedtls_net_free(&ctx);
		ret = -1;
		goto exit_func;
	}

	/*
	 * Set the server identity (hostname) that must be present in its
	 * certificate CN or SubjectAltName.
	 */
	if (mbedtls_ssl_set_hostname(&ssl, addr) != 0) {
		ret = -1;
		goto exit_func;
	}

	mbedtls_ssl_set_bio(&ssl, &ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	uint64_t start = sys_get_tick_ms();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			ret = -1;
			goto exit_func;
		}
		if (sys_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			ret = -2;
			goto exit_func;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}

exit_func:
	STOP_CALC_OVRHD_BYTES();
	PRINT_OVRHD_SENT();
	PRINT_OVRHD_RECVD();
	return ret;
}

bool mqtt_net_disconnect(void)
{
	int s = mbedtls_ssl_close_notify(&ssl);
	mbedtls_ssl_free(&ssl);
	mbedtls_net_free(&ctx);
	if (s == 0)
		return true;
	else
		return false;
}
