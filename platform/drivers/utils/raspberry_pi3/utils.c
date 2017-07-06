/* Copyright(C) 2017 Verizon. All rights reserved. */
#define _BSD_SOURCE
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

#include "dbg.h"

#define NETWORK_INTERFACE    "eth0"
#define MAX_CLIENT_ID_SZ     23

bool utils_get_mac_addr(char *id, uint8_t len)
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
        snprintf(id, len * 2, "%x%x%x%x%x%x", macadd[0], macadd[1], macadd[2],
                macadd[3] ,macadd[4] ,macadd[5]);
        return true;
}

bool utils_get_ip_addr(char *ipaddr, uint8_t length)
{
        if (ipaddr == NULL)
                return false;
        int fd;
        struct ifreq ifr;
        char *buffer = NULL;

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd != -1) {
                ifr.ifr_addr.sa_family = AF_INET;
                memcpy(ifr.ifr_name, NETWORK_INTERFACE,
                        strlen(NETWORK_INTERFACE) + 1);
                ioctl(fd, SIOCGIFADDR, &ifr);
                buffer = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
                strncpy(ipaddr, buffer, length);
                close(fd);
        } else {
                dbg_printf("%s:%d: Socket creation error!\n", __func__, __LINE__);
                return false;
        }
        return true;
}

bool utils_get_device_id(char *id, uint8_t len)
{
        if (!id || len == 0)
                return false;
        return utils_get_mac_addr(id, len);
}
