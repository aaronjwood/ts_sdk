/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * This module defines an interface to a group of sensors.
 */

typedef struct {
	uint8_t sz;		/* Number of bytes contained in the data buffer */
	uint8_t *bytes;		/* Pointer to the data buffer */
} array_t;

/*
 * Initialize the sensor(s) specified in the mask along with the underlying
 * hardware used to communicate with the sensor. This function will be called
 * at most once.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  : Initialization was successful
 * 	False : Initialization failed
 */
bool si_init(void);

/*
 * Read the calibration table(s) into the buffer provided.
 *
 * Parameters:
 * 	idx    : Index of the sensor to be read.
 * 	max_sz : Maximum size of the buffer being passed.
 * 	data   : Pointer to an array_t type variable that will hold the raw bytes
 * 	         read from the sensor.
 *
 * Returns:
 * 	True  : Calibration data read successfully into the buffer
 * 	False : Failed to read calibration data
 */
bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data);

/*
 * Put the sensor(s) to sleep in order to conserve power.
 *
 * Parameters:
 * 	mask : Bitmask that specifies which sensors are active and which are not
 *
 * Returns:
 * 	True  : The sensor was put into sleep mode
 * 	False : Failed to put the sensor into sleep mode
 */
bool si_sleep(void);

/*
 * Wake up the sensor(s) for a read.
 *
 * Parameters:
 * 	mask : Bitmask that specifies which sensors are active and which are not
 *
 * Returns:
 * 	True  : The sensor was successfully woken up
 * 	False : Failed to wake up the sensor
 */
bool si_wakeup(void);

/*
 * Read raw readings from the sensor(s).
 *
 * Parameters:
 * 	idx    : Index of the sensor to be read.
 * 	max_sz : Maximum size of the buffer being passed.
 * 	data   : Pointer to an array_t type variable that will hold the raw bytes
 * 	         read from the sensor.
 *
 * Returns:
 * 	True  : Sensor data was successfully read in to the buffer
 * 	False : Failed to read sensor data
 */
bool si_read_data(uint8_t idx, uint16_t max_sz, array_t *data);

/*
 * Return the number of sensors handled by this module.
 *
 * Parameter:
 * 	None
 *
 * Returns:
 * 	Number of sensors handled by this module. This value will always be
 * 	greater than 0.
 */
uint8_t si_get_num_sensors(void);

#endif
