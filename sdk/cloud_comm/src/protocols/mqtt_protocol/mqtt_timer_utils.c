/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "paho_mqtt_port.h"
#include "sys.h"

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
