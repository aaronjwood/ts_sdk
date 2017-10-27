/* Copyright(C) 2017 Verizon. All rights reserved. */

#define _POSIX_C_SOURCE 199309
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define MS_NS_MULT	     1000000
#define MS_SEC_MULT          1000

void sys_init(void)
{
        /* nothing to do here */
}

void sys_delay(uint32_t delay_ms)
{
        struct timespec req, remain;
        req.tv_sec = delay_ms / MS_SEC_MULT;
        req.tv_nsec = (delay_ms % MS_SEC_MULT) * MS_NS_MULT;
        int res;
        for (;;) {
                res = nanosleep(&req, &remain);
                if (res == -1 && errno == EINVAL) {
                        printf("%s:%d: invalid input parameters\n",
                                __func__, __LINE__);
                        return;
                }
                if (res == -1 && errno == EFAULT) {
                        printf("%s:%d: problem copying userspace information\n",
                                __func__, __LINE__);
                        return;
                }
                if (res == -1 && errno == EINTR) {
                        printf("%s:%d: delay interrupted\n", __func__, __LINE__);
                        exit(1);
                }
                if (res == 0)
                        break; /* nanosleep() completed */
                req = remain; /* Next sleep is with remaining time */
        }
}

uint64_t sys_get_tick_ms(void)
{
        struct timespec start;
        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
                printf("%s:%d: failed to get time\n", __func__, __LINE__);
                return 0;
        }
        uint64_t ms = start.tv_nsec / MS_NS_MULT;
        return (start.tv_sec * MS_SEC_MULT) + ms;
}

uint32_t sys_sleep_ms(uint32_t sleep)
{
	if (sleep == 0)
		return 0;

        struct timespec req, remain;
        req.tv_sec = sleep / MS_SEC_MULT;
        req.tv_nsec = (sleep % MS_SEC_MULT) * MS_NS_MULT;
        int res = nanosleep(&req, &remain);

        if (res == 0)
                return 0;
        if (res == -1 && errno == EINVAL) {
                printf("%s:%d: invalid input parameters\n", __func__, __LINE__);
                return 0;
        }
        if (res == -1 && errno == EFAULT) {
                printf("%s:%d: problem copying userspace information\n",
                        __func__, __LINE__);
                return 0;
        }

        if (remain.tv_sec == 0)
                return (remain.tv_nsec / MS_NS_MULT);
        else
                return (remain.tv_sec * MS_SEC_MULT) +
                                (remain.tv_nsec / MS_NS_MULT);
}

void dsb()
{
	/* stub */
}
