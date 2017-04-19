/* Copyright(C) 2016 Verizon. All rights reserved. */

/* Sensor module for the BMP180 temperature and pressure sensor */
#include <stm32f4xx_hal.h>
#include <stdbool.h>

#include "sensor_interface.h"
#include "sys.h"

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while(0)

#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */

static I2C_HandleTypeDef i2c_handle;

#define NUM_SENSORS		1
#define BMP180_ADDR		0x77	/* I2C device address for BMP180 */

/* Registers and data lengths internal to the sensor */
#define HEADER_SZ		0x01	/* Header size representing the length */
#define CALIB_ADDR		0xaa	/* Address of calib table in device */
#define CALIB_SZ		22	/* Calibration table is 22 bytes long */
#define MEASURE_CTL		0xf4	/* Measurement control register */
#define OUT_MSB			0xf6	/* Sensor output register */
#define TEMP_SZ			2	/* Size of temperature reading in bytes */
#define PRES_SZ			3	/* Size of pressure reading in bytes */

/* I2C Sensor Write */
static bool i2c_sw(uint8_t addr, uint8_t *data, uint16_t sz)
{
	HAL_StatusTypeDef s;
	s = HAL_I2C_Mem_Write(&i2c_handle, BMP180_ADDR << 1,
			addr, I2C_MEMADD_SIZE_8BIT,
			data, sz, I2C_TIMEOUT);
	return (s == HAL_OK);
}

/* I2C Sensor Read */
static bool i2c_sr(uint8_t addr, uint8_t *data, uint16_t sz)
{
	HAL_StatusTypeDef s;
	s = HAL_I2C_Mem_Read(&i2c_handle, BMP180_ADDR << 1,
			addr, I2C_MEMADD_SIZE_8BIT,
			data, sz, I2C_TIMEOUT);
	return (s == HAL_OK);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	GPIO_InitTypeDef i2c_pins;
	i2c_pins.Pin = GPIO_PIN_8 | GPIO_PIN_9;	/* PB8 = SCL; PB9 = SDA */
	i2c_pins.Mode = GPIO_MODE_AF_OD;
	i2c_pins.Pull = GPIO_PULLUP;
	i2c_pins.Speed = GPIO_SPEED_FAST;
	i2c_pins.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &i2c_pins);
}

bool si_init(void)
{
	/* Initialize the I2C bus on the processor */
	i2c_handle.Instance = I2C1;
	i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	i2c_handle.Init.ClockSpeed = 400000;
	i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_16_9;
	i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE ;
	i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	/* BMP180 does not need a special initialization sequence */
	return HAL_I2C_Init(&i2c_handle) == HAL_OK;
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (idx > NUM_SENSORS - 1)
		return false;
	if (max_sz < CALIB_SZ)
		return false;
	data->sz = CALIB_SZ;
	EOE(i2c_sr(CALIB_ADDR, data->bytes, CALIB_SZ));
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
	uint8_t temp_cmd = 0x2e;
	uint8_t pres_cmd = 0x34;
	if (max_sz < (PRES_SZ + TEMP_SZ))
		return false;
	data->sz = PRES_SZ + TEMP_SZ;
	EOE(i2c_sw(MEASURE_CTL, &temp_cmd, 1));
	sys_delay(5);
	EOE(i2c_sr(OUT_MSB, data->bytes, TEMP_SZ));
	EOE(i2c_sw(MEASURE_CTL, &pres_cmd, 1));
	sys_delay(5);
	EOE(i2c_sr(OUT_MSB, data->bytes + TEMP_SZ, PRES_SZ));
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}
