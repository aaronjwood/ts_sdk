/* Copyright(C) 2017 Verizon. All rights reserved. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
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
                        strncpy(ipaddr, host, length);
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
