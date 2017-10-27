/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * XXX: Cortex M4 has hardware support for single precision floating point
 * numbers, i.e. "float" and numbers such as "2.0f". Usage of "2.0" or "double"
 * will cause the compiler to do software emulation.
 */

#include <stdbool.h>
#include "hmc5883l_interpret.h"
#include <stdlib.h>

#define CALIB_TABLE_SZ			16
#define HMC5883L_RAW_DATA_SZ		6

bool hmc5883l_get_readings(uint8_t sz, const uint8_t *raw_data, hmc5883l_reading_t *val)
{
	if (sz != HMC5883L_RAW_DATA_SZ || raw_data == NULL || val == NULL)
		return false;

	int16_t x_val = raw_data[1] << 8 | raw_data[0];
	int16_t y_val = raw_data[3] << 8 | raw_data[2];
	int16_t z_val = raw_data[5] << 8 | raw_data[4];
	val->magnetometer.x = x_val * 0.061f;
	val->magnetometer.y = y_val * 0.061f;
	val->magnetometer.z = z_val * 0.061f;

	return true;
}
