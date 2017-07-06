/**
 * \file sys.h
 *
 * \brief APIs to address platform related init and other utillities
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief	Retrieves unique device id i.e. mac address/ imei etc...
 * \param[in] id:	valid buffer to store retrieved id.
 * \param[in] len:	buffer length.
 *
 */
bool utils_get_device_id(char *id, uint8_t len);

/**
 * \brief	Retrieves ip address
 * \param[in] ipaddr:	valid buffer to store retrieved id.
 * \param[in] len:	buffer length.
 *
 */
bool utils_get_ip_addr(char *ipaddr, uint8_t len);

#endif
