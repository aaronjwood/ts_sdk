/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include <stdbool.h>

#include "sensor_interface.h"
#include "platform.h"

/* Exit on error macro */
#define EOE(func, mask, dev_bit) \
	do { \
		if (!(func)) \
			return (mask & ~(1 << (dev_bit - 1))); \
	} while(0)

#define BMP180_BIT		0x01	/* Bit position of this sensor in the mask */
#define BMP180_ADDR		0x77	/* I2C device address for BMP180 */
#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */

/* Registers and data lengths internal to the sensor */
#define CALIB_ADDR		0xaa	/* Address of calib table in device */
#define CALIB_SZ		22	/* Calibration table is 22 bytes long */
#define MEASURE_CTL		0xf4	/* Measurement control register */
#define OUT_MSB			0xf6	/* Sensor output register */
#define TEMP_SZ			2	/* Size of temperature reading in bytes */
#define PRES_SZ			3	/* Size of pressure reading in bytes */

static I2C_HandleTypeDef i2c_handle;

static bool i2c_sensor_write(uint8_t addr, uint8_t *data, uint16_t sz)
{
	HAL_StatusTypeDef s;
	s = HAL_I2C_Mem_Write(&i2c_handle, BMP180_ADDR << 1,
			addr, I2C_MEMADD_SIZE_8BIT,
			data, sz, I2C_TIMEOUT);
	return (s == HAL_OK);
}

static bool i2c_sensor_read(uint8_t addr, uint8_t *data, uint16_t sz)
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

mask_t si_init(mask_t mask)
{
	/* Initialize the I2C bus on the processor */
	i2c_handle.Instance = I2C1;
	i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	i2c_handle.Init.ClockSpeed = 400000;
	i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_16_9;
	i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE ;
	i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	EOE((HAL_I2C_Init(&i2c_handle) == HAL_OK), mask, BMP180_BIT);

	/* BMP180 does not need a special initialization sequence */
	return mask;
}

mask_t si_read_calib(mask_t mask, uint16_t max_sz, array_t *data)
{
	if (max_sz < CALIB_SZ)
		return ~mask;
	data->sz = CALIB_SZ;
	EOE(i2c_sensor_read(CALIB_ADDR, data->bytes, CALIB_SZ), mask, BMP180_BIT);
	return mask;
}

mask_t si_sleep(mask_t mask)
{
	/* BMP180 does not have a sleep sequence */
	return mask;
}

mask_t si_wakeup(mask_t mask)
{
	/* BMP180 does not have a wakeup sequence */
	return mask;
}

mask_t si_read_data(mask_t mask, uint16_t max_sz, array_t *data)
{
	static uint8_t temp_cmd = 0x2e;
	static uint8_t pres_cmd = 0x34;
	if (max_sz < (PRES_SZ + TEMP_SZ))
		return ~mask;
	data->sz = PRES_SZ + TEMP_SZ;
	EOE(i2c_sensor_write(MEASURE_CTL, &temp_cmd, 1), mask, BMP180_BIT);
	platform_delay(5);
	EOE(i2c_sensor_read(OUT_MSB, data->bytes, TEMP_SZ), mask, BMP180_BIT);
	EOE(i2c_sensor_write(MEASURE_CTL, &pres_cmd, 1), mask, BMP180_BIT);
	platform_delay(5);
	EOE(i2c_sensor_read(OUT_MSB, data->bytes + TEMP_SZ, PRES_SZ), mask,
			BMP180_BIT);
	return mask;
}
