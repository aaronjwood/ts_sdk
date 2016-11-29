/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

#include <stdint.h>

/*
 * This module defines an interface to a group of sensors. Individual sensors
 * can be addressed through a bit mask. A value of '1' in the bitmask allows
 * access to the sensor; a value of '0' ignores the sensor for the current
 * call.
 */

/* Bitmasks are currently 16 bit wide, i.e. able to accommodate 16 sensors. */
typedef uint16_t mask_t;
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
 * 	mask   : Bitmask that specifies which sensors are active and which are not
 *
 * Returns:
 * 	A mask whose bits specify the success / failure of the operation on the
 * 	individual sensors. A bit value of '1' implies success for the particular
 * 	sensor; a bit value of '0' implies failure.
 * 	Ideally, this value should match the mask passed to this function.
 */
mask_t si_init(mask_t mask);

/*
 * Read the calibration table(s) into the buffer provided.
 *
 * Parameters:
 * 	mask   : Bitmask that specifies which sensors are active and which are not
 * 	max_sz : Maximum size of the buffer being passed.
 * 	data   : Pointer to an array_t type variable that will hold the raw bytes
 * 	         read from the sensor.
 *
 * Returns:
 * 	A mask whose bits specify the success / failure of the operation on the
 * 	individual sensors. A bit value of '1' implies success for the particular
 * 	sensor; a bit value of '0' implies failure.
 * 	Ideally, this value should match the mask passed to this function.
 */
mask_t si_read_calib(mask_t mask, uint16_t max_sz, array_t *data);

/*
 * Put the sensor(s) to sleep in order to conserve power.
 *
 * Parameters:
 * 	mask : Bitmask that specifies which sensors are active and which are not
 *
 * Returns:
 * 	A mask whose bits specify the success / failure of the operation on the
 * 	individual sensors. A bit value of '1' implies success for the particular
 * 	sensor; a bit value of '0' implies failure.
 * 	Ideally, this value should match the mask passed to this function.
 */
mask_t si_sleep(mask_t mask);

/*
 * Wake up the sensor(s) for a read.
 *
 * Parameters:
 * 	mask : Bitmask that specifies which sensors are active and which are not
 *
 * Returns:
 * 	A mask whose bits specify the success / failure of the operation on the
 * 	individual sensors. A bit value of '1' implies success for the particular
 * 	sensor; a bit value of '0' implies failure.
 * 	Ideally, this value should match the mask passed to this function.
 */
mask_t si_wakeup(mask_t mask);

/*
 * Read raw readings from the sensor(s).
 *
 * Parameters:
 * 	mask   : Bitmask that specifies which sensors are active and which are not
 * 	max_sz : Maximum size of the buffer being passed.
 * 	data   : Pointer to an array_t type variable that will hold the raw bytes
 * 	         read from the sensor.
 *
 * Returns:
 * 	A mask whose bits specify the success / failure of the operation on the
 * 	individual sensors. A bit value of '1' implies success for the particular
 * 	sensor; a bit value of '0' implies failure.
 * 	Ideally, this value should match the mask passed to this function.
 */
mask_t si_read_data(mask_t mask, uint16_t max_sz, array_t *data);

#endif
