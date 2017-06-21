#ifndef PAHO_MQTT_PORT_H
#define PAHO_MQTT_PORT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Timer {
	uint64_t end_time;
} Timer;

void TimerInit(Timer *timer);
char TimerIsExpired(Timer *timer);
void TimerCountdownMS(Timer *timer, unsigned int ms);
void TimerCountdown(Timer *timer, unsigned int sec);
int TimerLeftMS(Timer *timer);

typedef struct Network {
	int (*mqttread) (struct Network *net, unsigned char *buffer,
			int len, int timeout_ms);
	int (*mqttwrite) (struct Network *net, unsigned char *buffer,
			int len, int timeout_ms);
} Network;
#endif
