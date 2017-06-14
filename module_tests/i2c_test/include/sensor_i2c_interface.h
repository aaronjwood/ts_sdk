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
}array_t;

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
bool si_i2c_init(void);


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
bool si_i2c_read_data(array_t *data);

#endif
