#include <stdio.h>

#include "paho_port_generic.h"
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

/* XXX: Move these to the application code and create an interface for them here */
#include "client-crt-1801.h"
#include "client-key-1801.h"

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
static int read_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	uint64_t now = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int recvd = mbedtls_net_recv(&ctx, &b[nbytes], (len - nbytes));
		if (recvd == 0)
			return 0;
		if (recvd < 0 && recvd != MBEDTLS_ERR_SSL_WANT_READ)
			return -1;
		if (recvd > 0)
			nbytes += recvd;
		now = sys_get_tick_ms();
	} while (nbytes < len && now - start_time < timeout_ms);
	return nbytes;
}*/

/*
 * Attempt to read 'len' bytes from the TCP/TLS stream into buffer 'b' within
 * 'timeout_ms' ms.
 */
static int read_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	uint64_t now = sys_get_tick_ms();
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
		now = sys_get_tick_ms();
	} while (nbytes < len && now - start_time < timeout_ms);
	return nbytes;
}

/*
static int write_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	uint64_t now = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int sent = mbedtls_net_send(&ctx, b, len);
		if (sent < 0 && sent != MBEDTLS_ERR_SSL_WANT_WRITE)
			return -1;
		if (sent >= 0)
			nbytes += sent;
		now = sys_get_tick_ms();
	} while (nbytes < len && now - start_time < timeout_ms);
	return nbytes;
}
*/

/*
 * Attempt writing to TCP/TLS stream a buffer 'b' of length 'len' within
 * 'timeout_ms' ms. Return -1 for an error, a positive number for the number of
 * bytes actually written.
 */
static int write_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	uint64_t now = sys_get_tick_ms();
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
		now = sys_get_tick_ms();
	} while (nbytes < len && now - start_time < timeout_ms);
	return nbytes;
}

static bool init_tls(void)
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
	dbg_printf("phase 0\n");
	if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				NULL, 0) != 0) {
		cleanup_mbedtls();
		return false;
	}
	dbg_printf("phase 1\n");

	/* Load the CA root certificate */
	if (mbedtls_x509_crt_parse(&cacert, cacert_buf,
					 sizeof(cacert_buf)) < 0) {
		cleanup_mbedtls();
		return false;
	}
	dbg_printf("phase 2\n");

	/* Load the client certificate */
	if (mbedtls_x509_crt_parse(&cl_cert, client_cert,
					 sizeof(client_cert)) < 0) {
		cleanup_mbedtls();
		return false;
	}
	dbg_printf("phase 3\n");

	/* Load the client key */
	if (mbedtls_pk_parse_key(&cl_key, client_key, sizeof(client_key),
			NULL, 0) != 0) {
		cleanup_mbedtls();
		return false;
	}
	dbg_printf("phase 4\n");

	/* Set up the TLS structures */
	if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
		cleanup_mbedtls();
		return false;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	if (mbedtls_ssl_conf_own_cert(&conf, &cl_cert, &cl_key) != 0) {
		cleanup_mbedtls();
		return false;
	}
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif
	return true;
}

bool paho_net_init(Network* n)
{
	mbedtls_net_init(&ctx);
	if (!init_tls())
		return false;
	n->mqttread = read_fn;
	n->mqttwrite = write_fn;
	return true;
}

int paho_net_connect(Network* n, char* addr, int port)
{
	char port_str[6];
	snprintf(port_str, 6, "%d", port);

	/* Connect to the cloud services over TCP */
	if (mbedtls_net_connect(&ctx, addr, port_str, MBEDTLS_NET_PROTO_TCP) < 0)
		return -1;

	/* Set up the SSL context */
	if (mbedtls_ssl_setup(&ssl, &conf) != 0) {
		mbedtls_net_free(&ctx);
		return -1;
	}

	mbedtls_ssl_set_bio(&ssl, &ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

	/*
	 * Set the server identity (hostname) that must be present in its
	 * certificate CN or SubjectAltName.
	 */
	if (mbedtls_ssl_set_hostname(&ssl, addr) != 0)
		return -1;

	/* Perform TLS handshake */
	int ret = mbedtls_ssl_handshake(&ssl);
	uint64_t start = sys_get_tick_ms();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			return -1;
		}
		if (sys_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			return -2;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}
	return 0;
}

bool paho_net_disconnect(Network* n)
{
	int s = mbedtls_ssl_close_notify(&ssl);
	mbedtls_ssl_free(&ssl);
	mbedtls_net_free(&ctx);
	if (s == 0)
		return true;
	else
		return false;
}
