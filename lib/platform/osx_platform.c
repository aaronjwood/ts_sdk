/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <mach/mach_time.h>

#define MS_NS_MULT	1000000
#define MS_US_MULT	1000

static mach_timebase_info_data_t time_base;

void platform_init()
{
	/* Get the time base for mach_absolute_time() */
	mach_timebase_info(&time_base);
}

void platform_delay(uint32_t delay_ms)
{
	usleep(delay_ms * MS_US_MULT);
}

uint32_t platform_get_tick_ms(void)
{
	/*
	 * XXX: On POSIX compatible systems, use:
	 * clock_gettime(CLOCK_MONOTONIC, ..)
	 *
	 * While technically mach_absolute_time() is not a monotonic timer, it
	 * should be okay for small (millisecond range) time durations.
	 */
	uint64_t time_count = mach_absolute_time();
	uint64_t time_ns = time_count * time_base.numer / time_base.denom;
	return (time_ns / MS_NS_MULT);
}
