/*++
 * Copyright (C) 2017 Sycamore Labs, LLC, All Rights Reserved
 *
 * bme-280.h
 *
 * interface for the sensor on the Beduin-v2 board
 *
 */

#ifndef __bme280_beduin__
#define __bme280_beduin__
#include <stdint.h>

typedef struct {
	uint8_t sz;		/* Number of bytes contained in the data buffer */
	uint8_t *bytes;		/* Pointer to the data buffer */
} array_t;

/*	\Brief: The function is used as I2C bus read
 *	\Return : Status of the I2C read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be read
 *	\param reg_data : This data read from the sensor, which is hold in an array
 *	\param cnt : The no of byte of data to be read
 */
int8_t BME280_I2C_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

 /*	\Brief: The function is used as I2C bus write
 *	\Return : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
int8_t BME280_I2C_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

/*	Brief : The delay routine
 *	\param : delay in ms
*/
void BME280_delay_msek(uint32_t msek);

void bme280_beduin_init(void);
int32_t bme280_beduin_read_sensor(array_t *buffer_struct);
int32_t bme280_beduin_wake(void);
int32_t bme280_beduin_sleep(void);



#endif // __bme280-beduin
