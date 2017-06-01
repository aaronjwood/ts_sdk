#ifndef PAHO_PORT_GENERIC_H
#define PAHO_PORT_GENERIC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Timer {
	uint64_t end_time;	/* XXX 64-bit ms values can never wrap realistically */
} Timer;

void TimerInit(Timer* timer);
char TimerIsExpired(Timer* timer);
void TimerCountdownMS(Timer* timer, unsigned int ms);
void TimerCountdown(Timer* timer, unsigned int sec);
int TimerLeftMS(Timer* timer);

typedef struct Network {
	int (*mqttread) (struct Network*, unsigned char*, int, int);
	int (*mqttwrite) (struct Network*, unsigned char*, int, int);
} Network;

void paho_net_init(Network* n);
bool paho_net_connect(Network* n, char* addr, int port);
void paho_net_disconnect(Network* n);

#endif
