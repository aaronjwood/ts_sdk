/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/* Sensor module for the BMP180 temperature and pressure sensor */
#include <stdbool.h>

#include "i2c_hal.h"
#include "sensor_interface.h"
#include "sys.h"
#include "dbg.h"

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)

periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;

#define NUM_SENSORS	1
#define BMP180_ADDR	0x77	/* I2C device address for BMP180 */

/* Registers and data lengths internal to the sensor */
#define HEADER_SZ	0x01	/* Header size representing the length */
#define CALIB_ADDR	0xaa	/* Address of calib table in device */
#define CALIB_SZ	22	/* Calibration table is 22 bytes long */
#define MEASURE_CTL	0xf4	/* Measurement control register */
#define OUT_MSB		0xf6	/* Sensor output register */
#define TEMP_SZ		2	/* Size of temperature reading in bytes */
#define PRES_SZ		3	/* Size of pressure reading in bytes */
#define TEMP_CTL	0x2e	/* Temperature control register */
#define PRES_CTL	0x34	/* Pressure control register */

bool si_init(void)
{
	/* Initialize the I2C bus on the processor */
	i2c_handle =  i2c_init(PB8, PB9);

	if (i2c_handle == NO_PERIPH)
		return false;

	return true;
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (idx > NUM_SENSORS - 1)
		return false;
	if (max_sz < CALIB_SZ)
		return false;
	data->sz = CALIB_SZ;
	i2c_dest_addr.slave = BMP180_ADDR;
	i2c_dest_addr.reg = CALIB_ADDR;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz,  data->bytes));
	for (int i = 0; i < CALIB_SZ ; i++)
		dbg_printf("After calibration data[%d] = %x\n", i, \
							data->bytes[i]);
	return true;
}

bool si_sleep(void)
{
	/* BMP180 does not have a sleep sequence */
	return true;
}

bool si_wakeup(void)
{
	/* BMP180 does not have a wakeup sequence */
	return true;
}

bool si_read_data(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (idx > NUM_SENSORS - 1)
		return false;
	/* Read temperature and pressure off the BMP180 sensor */
	uint8_t temp_cmd = TEMP_CTL;
	uint8_t pres_cmd = PRES_CTL;
	if (max_sz < (PRES_SZ + TEMP_SZ))
		return false;
	data->sz = PRES_SZ + TEMP_SZ;
	i2c_dest_addr.slave = BMP180_ADDR;
	i2c_dest_addr.reg = MEASURE_CTL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &temp_cmd));
	sys_delay(5);

	i2c_dest_addr.reg = OUT_MSB;
	data->sz = TEMP_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz, data->bytes));
	dbg_printf("Apps-bmp180: Temperature Value = %2x%2x\n",\
					*(data->bytes), *((data->bytes)+1));
	i2c_dest_addr.reg = MEASURE_CTL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &pres_cmd));
	sys_delay(5);

	data->sz = PRES_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz,\
					 ((data->bytes)+TEMP_SZ)));
	dbg_printf("Apps-bmp180: Pressure Value = %2x%2x%2x\n",\
					*((data->bytes) + TEMP_SZ), \
					*((data->bytes) + TEMP_SZ + 1),	\
					*((data->bytes) + TEMP_SZ + 2));
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}
