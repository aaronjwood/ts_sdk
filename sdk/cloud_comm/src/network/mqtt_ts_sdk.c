#include <stdio.h>

#include "paho_port_generic.h"
#include "sys.h"

#include "mbedtls/net.h"

static mbedtls_net_context ctx;

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
}

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

void paho_net_init(Network* n)
{
	mbedtls_net_init(&ctx);
	n->mqttread = read_fn;
	n->mqttwrite = write_fn;
}

bool paho_net_connect(Network* n, char* addr, int port)
{
	char port_str[6];
	snprintf(port_str, 6, "%d", port);
	int r = mbedtls_net_connect(&ctx, (const char *)addr, port_str,
			MBEDTLS_NET_PROTO_TCP);
	return r >= 0;
}

void paho_net_disconnect(Network* n)
{
	mbedtls_net_free(&ctx);
}
