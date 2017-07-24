/**
 * \file utils.h
 *
 * \brief Helper utility APIs, may be optional for some platforms
 *
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief	Retrieves unique device id i.e. mac address/ imei etc...
 * \param[out] id:	 valid buffer to store retrieved id.
 * \param[in] len:	 buffer length.
 * \param[in] interface: Optional null terminated string describing the interface
 *                       to fetch the device id, for example eth0 for raspberry
 *                       pi devices which fetches mac address as the unique
 *                       device id.
 * \return    True if success and id contains valid value or false otherwise.
 */
bool utils_get_device_id(char *id, uint8_t len, const char *interface);

/**
 * \brief	Retrieves ip address
 * \param[out] ipaddr:	valid buffer to store retrieved id.
 * \param[in] len:	buffer length.
 * \param[in] interface: Optional null terminated string describing the interface
 *                       to fetch the device id, for example eth0 for raspberry
 *                       pi devices.
 * \return    True if success and ipaddr contains valid value or false otherwise.
 */
bool utils_get_ip_addr(char *ipaddr, uint8_t len, const char *interface);

#endif