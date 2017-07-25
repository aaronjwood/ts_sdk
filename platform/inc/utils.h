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

/**
 * \brief	Retrieves system physical RAM information.
 * \param[out] free_ram:	valid pointer to store ram information.
 * \param[out] avail_ram:	valid pointer to store ram information.
 */
void utils_get_ram_info(uint32_t *free_ram, uint32_t *avail_ram);

/**
 * \brief	Retrieves os version
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_os_version(char *value, uint32_t len);

/**
 * \brief	Retrieves kernel version
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_kernel_version(char *value, uint32_t len);

/**
 * \brief	Retrieves current calendar date information.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_local_time(char *value, uint32_t len);

/**
 * \brief	Retrieves current time zone information.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_time_zone(char *value, uint32_t len);

/**
 * \brief	Retrieves up time of the device.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_uptime(char *value, uint32_t len);

/**
 * \brief	Retrieves os version
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_manufacturer(char *value, uint32_t len);

/**
 * \brief	Retrieves device model.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_dev_model(char *value, uint32_t len);

/**
 * \brief	Retrieves chipset.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_chipset(char *value, uint32_t len);

/**
 * \brief	Retrieves iccid information.
 * \param[out] value:	valid buffer to store retrieved null formatted string.
 * \param[in] len:	buffer length.
 * \return    True if success and value contains valid value or false otherwise.
 */
bool utils_get_iccid(char *value, uint32_t len);

#endif
