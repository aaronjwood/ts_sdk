/**
 * \file gps_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for GPS
 * \details This header defines a platform independent API
 * to read and write over the GPS. All sending operations
 * are blocking.
 */

#ifndef GPS_HAL_H
#define GPS_HAL_H

#include <stdbool.h>
#include <stdint.h>

struct parsed_nmea_t {
	uint8_t hour;
	uint8_t minute;
	uint8_t seconds;
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint16_t milliseconds;
	float latitude;
	float longitude;
	int32_t latitude_fixed;
	int32_t longitude_fixed;
	float latitude_degrees;
	float longitude_degrees;
	float geoid_height;
	float altitude;
	float speed;
	float angle;
	float mag_variation;
	float HDOP;
	char lat;
	char lon;
	char mag;
	bool fix;
	uint8_t fix_quality;
	uint8_t satellites;
};

/**
 * \brief Initialize the GPS module
 * \details Configure and initialize the required drivers for GPS
 * Setup NMEA protocol by sending NMEA messages/sentances.
 *
 * \retval true Initialization was successful.
 * \retval false Initialization failed.
 */
bool gps_module_init();

/**
 * \brief Receive and parse the GPS NMEA
 * \detail Receive and parse NMEA and store the parsed
 * values in the received structure format.
 *
 * \param[in] parsedNMEA  parsed values will be stored in this.
 *
 * \retval true NMEA is received successfully.
 * \retval false NMEA is not received successfully.
 */
bool gps_receive(struct parsed_nmea_t *parsedNEMA);

/**
 * \brief Stops GPS reporting.
 *
 * \retval true  sleep command successfull.
 * \retval false sleep command not successfull.
 *
 */
bool gps_sleep(void);

/**
 * \brief Restart GPS reporting.
 *
 * \retval true  wakeup command successfull.
 * \retval false wakeup command not successfull.
 *
 */
bool gps_wake(void);

#endif
