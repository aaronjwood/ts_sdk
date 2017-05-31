#include <stdio.h>

#include "paho_port.h"
#include "sys.h"

#ifdef MBEDTLS_DEBUG_C
#include "mbedtls/debug.h"
#endif

void TimerInit(Timer *timer)
{
	timer->end_time = 0;
}

bool TimerIsExpired(Timer *timer)
{
	uint64_t now = sys_get_tick_ms();
	return timer->end_time < now;
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
	return timer->end_time < now ? 0 : (int)(timer->end_time - now);
}

static int read_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	return mbedtls_net_recv_timeout(&n->ctx, b, len, timeout_ms);
}

static int write_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t now = sys_get_tick_ms();
	uint64_t end_time = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int ret = mbedtls_net_send(&n->ctx, b, len);
		if (ret > 0)
			nbytes += ret;
		end_time = sys_get_tick_ms();
	} while (nbytes != len && end_time - now < timeout_ms);
	return nbytes;
}

void paho_net_init(Network* n)
{
	mbedtls_net_init(&n->ctx);
	n->mqttread = read_fn;
	n->mqttwrite = write_fn;
}

bool paho_net_connect(Network* n, char* addr, int port)
{
	char port_str[6];
	snprintf(port_str, 6, "%d", port);
	int r = mbedtls_net_connect(&n->ctx, (const char *)addr, port_str,
			MBEDTLS_NET_PROTO_TCP);
	return r >= 0;
}

void paho_net_disconnect(Network* n)
{
	mbedtls_net_free(&n->ctx);
}
