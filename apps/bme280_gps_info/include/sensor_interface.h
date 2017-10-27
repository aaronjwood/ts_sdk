/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t sz;		/* Number of bytes contained in the data buffer */
	uint8_t *bytes;		/* Pointer to the data buffer */
} array_t;

/*
 * Initialize bme280 sensor along with the underlying hardware used
 * to communicate with the sensor. This function will be called
 * at most once.
 *
 * Parameters:
 *      None
 *
 * Returns:
 *      True  : Initialization was successful
 *      False : Initialization failed
 */
bool si_init(void);

/*
 * Read sensor data into the buffer provided.
 *
 * Parameters:
 *      data   : Pointer to an array_t type variable that will hold the raw bytes
 *               read from the sensor.
 *
 * Returns:
 *      True  : Sensor data read successfully into the buffer
 *      False : Failed to read sensor data
 */
bool si_read_data(array_t *buffer_struct);

/*
 * Wake up the sensor for a read.
 *
 * Returns:
 *      True  : The sensor was successfully woken up
 *      False : Failed to wake up the sensor
 */
int32_t si_wakeup(void);

/*
 * Put the sensor to sleep in order to conserve power.
 *
 * Returns:
 *      True  : The sensor was put into sleep mode
 *      False : Failed to put the sensor into sleep mode
 */
int32_t si_sleep(void);

#endif //SENSOR_INTERFACE_H
