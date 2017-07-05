/* Copyright(C) 2017 Verizon. All rights reserved. */

#define _POSIX_C_SOURCE 199309
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* IP address and mac address related includes */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>

#define MS_NS_MULT	     1000000
#define MS_SEC_MULT          1000

#define NETWORK_INTERFACE    "eth0"
#define MAX_CLIENT_ID_SZ     23

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

void sys_reset_modem(uint16_t pulse_width_ms)
{
        /* stub */
}

bool sys_get_mac_addr(char *id, uint8_t len)
{
        uint8_t macadd[6];
        int fd;
        struct ifreq ifr;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
                memset(&ifr, 0, sizeof(struct ifreq));
                memcpy(ifr.ifr_name, NETWORK_INTERFACE,
                        strlen(NETWORK_INTERFACE));
                if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
                        dbg_printf("%s:%d:ioctl failed, can not retrieve mac "
                                "address\n", __func__, __LINE__);
                        close(fd);
                        return false;
                }
                memcpy(macadd, ifr.ifr_hwaddr.sa_data, 6);
                close(fd);
        } else {
                dbg_printf("%s:%d:Socket create failed, can not retrieve mac "
                        "address\n", __func__, __LINE__);
                return false;
        }
        uint8_t length = (len <= 6) ? len : 6;
        snprintf(id, length, "%u", macadd);
        dbg_printf("Device MAC Addr: %s\n", id);
        return true;
}

#if 0
void ts_get_ip(char *ipaddr, unsigned int length)
{
        int fd;
        struct ifreq ifr;
        char* buffer = NULL;
        const char*no_ip_str = {"no_ip"};

        if (ipaddr == NULL) {
        IOT_ERROR("ThingSpace : Invalid ip buffer!");
        return;
        }
        strncpy(ipaddr, no_ip_str, length);

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
        ifr.ifr_addr.sa_family = AF_INET;
        memcpy(ifr.ifr_name, NETWORK_INTERFACE, strlen(NETWORK_INTERFACE) + 1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        buffer = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        strncpy(ipaddr, buffer, length);
        close(fd);
        }
        else {
        IOT_ERROR("ThingSpace : Socket creation error!\n");
        }
}
#endif

bool sys_get_device_id(const char *id, uint8_t len)
{
        if (!id || len == 0)
                return false;
        return sys_get_mac_addr(id, len);
}

void dsb()
{
	/* stub */
}
