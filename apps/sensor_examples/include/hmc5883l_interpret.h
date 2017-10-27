/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef HMC5883L_INTERPRET_H
#define HMC5883L_INTERPRET_H

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"

typedef struct {
	struct {
		float x;
		float y;
		float z;
	} magnetometer;
} hmc5883l_reading_t;

/*
 * Obtain the values of magnetometer readings from the HMC5883L
 * sensor.
 *
 * Parameters:
 * 	sz       - Size of the raw data in bytes.
 * 	raw_data - Pointer to an array of bytes representing
 * 	           magnetometer data (X, Y, Z axes)
 *
 * Returns:
 * 	true  - A valid reading was obtained.
 * 	false - Failed to obtain a valid reading.
 */
bool hmc5883l_get_readings(uint8_t sz, const uint8_t *raw_data, hmc5883l_reading_t *val);

void send_json_payload(cc_buffer_desc *send_buffer, cc_data_sz *send_sz);

#endif
