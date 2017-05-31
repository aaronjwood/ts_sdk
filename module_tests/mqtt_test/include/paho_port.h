#ifndef PAHO_PORT_H
#define PAHO_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "mbedtls/net.h"

typedef struct Timer {
	uint64_t end_time;	/* XXX 64-bit ms values can never wrap realistically */
} Timer;

void TimerInit(Timer* timer);
bool TimerIsExpired(Timer* timer);
void TimerCountdownMS(Timer* timer, unsigned int ms);
void TimerCountdown(Timer* timer, unsigned int sec);
int TimerLeftMS(Timer* timer);

typedef struct Network {
	mbedtls_net_context ctx;
	int (*mqttread) (struct Network*, unsigned char*, int, int);
	int (*mqttwrite) (struct Network*, unsigned char*, int, int);
} Network;

void paho_net_init(Network* n);
bool paho_net_connect(Network* n, char* addr, int port);
void paho_net_disconnect(Network* n);

#endif
