/* Copyright(C) 2017 Verizon. All rights reserved. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>

/* IP address and mac address related includes */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "dbg.h"

static bool utils_get_mac_addr(char *id, uint8_t len, const char *interface)
{
        uint8_t macadd[6];
        int fd;
        struct ifreq ifr;
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
                memset(&ifr, 0, sizeof(struct ifreq));
                memcpy(ifr.ifr_name, interface, strlen(interface));
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
        snprintf(id, len, "%X%X%X%X%X%X", macadd[0], macadd[1], macadd[2],
                        macadd[3] ,macadd[4] ,macadd[5]);
        return true;
}

bool utils_get_ip_addr(char *ipaddr, uint8_t length, const char *interface)
{
        if (!ipaddr || !interface || length == 0) {
                dbg_printf("%s:%d: invalid parameters\n", __func__, __LINE__);
                return false;
        }

        struct ifaddrs *ifaddr, *ifa;
        int s;
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) != 0) {
                dbg_printf("%s:%d: getifaddrs function failed\n",
                        __func__, __LINE__);
                return false;
        }


        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                        continue;

                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                                NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if ((strcmp(ifa->ifa_name, interface) == 0) &&
                        (ifa->ifa_addr->sa_family == AF_INET)) {
                        if (s != 0) {
                                printf("%s:%d: getnameinfo() failed: %s\n",
                                        __func__, __LINE__, gai_strerror(s));
                                return false;
                        }
                        snprintf(ipaddr, length, "%s", host);
                        freeifaddrs(ifaddr);
                        return true;
                }
        }
        dbg_printf("%s:%d: no interface found to fetch ip address\n",
                __func__, __LINE__);
        return false;
}

bool utils_get_device_id(char *id, uint8_t len, char *interface)
{
        if (!id || !interface || len == 0)
                return false;
        return utils_get_mac_addr(id, len, interface);
}

void utils_get_ram_info(uint32_t *free_ram, uint32_t *avail_ram)
{
        if (!free_ram || !avail_ram)
                return;
        *free_ram = 0;
        *avail_ram = 0;
        struct sysinfo mem_info;
        sysinfo(&mem_info);
        *free_ram = mem_info.freeram;
        *avail_ram = mem_info.totalram - mem_info.freeram;
}

bool utils_get_os_version(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        struct utsname u;
        uname(&u);
        snprintf(value, len, "%s", u.sysname);
        return true;
}

bool utils_get_kernel_version(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        struct utsname u;
        uname(&u);
        snprintf(value, len, "%s", u.release);
        return true;
}

bool utils_get_manufacturer(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        snprintf(value, len, "%s", "raspberry pi");
        return true;
}

bool utils_get_dev_model(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        snprintf(value, len, "%s", "raspberry pi 3");
        return true;
}

bool utils_get_chipset(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        snprintf(value, len, "%s", "Broadcom BCM2837");
        return true;
}

bool utils_get_iccid(char *value, uint32_t len)
{
        return false;
}

bool utils_get_local_time(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        time_t rawtime;
        struct tm *timeinfo;

        if (time(&rawtime) < 0)
                return false;
        timeinfo = localtime(&rawtime);
        if (!timeinfo)
                return false;
        snprintf(value, len, "%s", asctime(timeinfo));

        char *pos;
        if ((pos = strchr(value, '\n')) != NULL)
                *pos = '\0';
        return true;
}

bool utils_get_time_zone(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        time_t t = time(NULL);
        struct tm lt = {0};
        localtime_r(&t, &lt);
        snprintf(value, len, "%s", lt.tm_zone);
        return true;
}

bool utils_get_uptime(char *value, uint32_t len)
{
        if (!value || (len == 0))
                return false;
        const long minute = 60;
        const long hour = minute * 60;
        const long day = hour * 24;

        struct sysinfo si;
        sysinfo(&si);
        snprintf(value, len, "%ld:%02ld:%02ld:%02ld", si.uptime / day,
                (si.uptime % day) / hour, (si.uptime % hour) / minute,
                si.uptime % minute);
        return true;
}
