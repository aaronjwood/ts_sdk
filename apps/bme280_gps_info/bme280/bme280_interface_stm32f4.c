/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include "sensor_interface.h"
#include <stdint.h>
#include "bme280.h"
#include "sys.h"
#include "i2c_hal.h"
#include "board_interface.h"
#include "dbg.h"

#define	I2C_BUFFER_LEN	28
#define I2C_TIMEOUT	5000
#define NUM_SENSOR_DATA	3

static struct bme280_t bme280;
static periph_t i2c_handle;
static i2c_addr_t i2c_dest_addr;

static void int_to_buffer(char *buffer, int32_t n)
{
	buffer[0] = (n >> 24) & 0xFF;
	buffer[1] = (n >> 16) & 0xFF;
	buffer[2] = (n >> 8) & 0xFF;
	buffer[3] = n & 0xFF;
}
/*
 * Write the data on the register through i2c api.
 *
 * Parameters:
 *      dev_addr : The device address of the sensor
 *      reg_addr : Address of the first register,
 *			where the data is going to be written
 *	reg_data : It is a value hold in the array,
 *			will be used for write the value into the register
 *	cnt	 : The no of byte of data to be write
 *
 * Returns:
 *      True  : I2C write was successful
 *      False : I2C write failed
 */
static s8 bme280_write(uint8_t dev_addr, uint8_t reg_addr,
			 uint8_t *reg_data, uint8_t cnt)
{
	int32_t error = BME280_INIT_VALUE;
	i2c_dest_addr.slave = dev_addr;
	i2c_dest_addr.reg = reg_addr;


	if (!(i2c_write(i2c_handle, i2c_dest_addr, cnt , reg_data))) {
		dbg_printf("i2c_write failed\n");
		error = (-1);
	}
	return (int8_t)error;
}

/*
 * Read the data from the register through i2c api.
 *
 * Parameters:
 *      dev_addr : The device address of the sensor
 *      reg_addr : Address of the first register,
 *                 from where the data will be read
 *      reg_data : It is a value hold in the array,
 *                 which is read from the register
 *      cnt      : The no of byte of data to be read
 *
 * Returns:
 *      True  : I2C write was successful
 *      False : I2C write failed
 */
static s8 bme280_read(uint8_t dev_addr, uint8_t reg_addr,
			 uint8_t *reg_data, uint8_t cnt)
{
	int32_t error = BME280_INIT_VALUE;
	uint8_t array[I2C_BUFFER_LEN] = {BME280_INIT_VALUE};
	uint8_t stringpos = BME280_INIT_VALUE;
	array[BME280_INIT_VALUE] = reg_addr;

	i2c_dest_addr.slave = dev_addr;
	i2c_dest_addr.reg = reg_addr;

	if (!(i2c_read(i2c_handle, i2c_dest_addr, cnt , array))) {
		dbg_printf("i2c_read failed\n");
		error = (-1);
	}

	for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++)
		*(reg_data + stringpos) = array[stringpos];

	return (int8_t)error;
}

/* The delay routine
 *
 * Parameters :
 *	ms : Delay in milliseconds
 *
*/
static void bme280_delay(uint32_t ms)
{
	sys_delay(10*ms);
}

static int32_t bme280_beduin_set_operational_mode(void)
{
	int32_t result = E_ERROR;

	/* Functions to talk to the I2C */
	bme280.bus_write = bme280_write;
	bme280.bus_read = bme280_read;

	/* BME280 on beduin is strapped for 0x77 */
	bme280.dev_addr = BME280_I2C_ADDRESS2;
	bme280.delay_msec = bme280_delay;

	/* init the bme280 struct */
	result = bme280_init(&bme280);

	/* For initialization it is required to set the mode of
	 * the sensor as "NORMAL"
	 * data acquisition/read/write is possible in this mode.*/

	/* Set the power mode as NORMAL*/
	result += bme280_set_power_mode(BME280_NORMAL_MODE);

	/* For reading the pressure, humidity and temperature data
	 * it is required to  set the OSS setting of humidity,
	 * pressure and temperature. The "BME280_CTRLHUM_REG_OSRSH" register
	 * sets the humidity data acquisition options of the device.
	 * Changes to this registers only become effective after a write
	 * operation to "BME280_CTRLMEAS_REG" register.
	 * In the code automated reading and writing of
	 * "BME280_CTRLHUM_REG_OSRSH" register first set the
	 * "BME280_CTRLHUM_REG_OSRSH" and then read and write
	 * the "BME280_CTRLMEAS_REG" register in the function
	*/

	result += bme280_set_oversamp_humidity(BME280_OVERSAMP_16X);

	/* Set the pressure oversampling*/
	result += bme280_set_oversamp_pressure(BME280_OVERSAMP_16X);

	/* Set the temperature oversampling*/
	result += bme280_set_oversamp_temperature(BME280_OVERSAMP_1X);

	/* This API used to write the standby time of the sensor for the
	 * given value. Normal mode comprises an automated perpetual
	 * cycling between an (active) Measurement period and an (inactive)
	 * standby period.
	 * The standby time is determined by the contents of the register t_sb.
	 * Standby time can be set using BME280_STANDBYTIME_125_MS.*/
	result += bme280_set_standby_durn(BME280_STANDBY_TIME_125_MS);

	return result;
}

bool si_init(void)
{
	i2c_handle =  i2c_init(I2C_SCL, I2C_SDA, I2C_TIMEOUT);
	ASSERT(!(i2c_handle == NO_PERIPH));
	int32_t result = E_ERROR;
	result = bme280_beduin_set_operational_mode();
	if (result != E_ERROR)
		return true;
	else
		return false;
}

bool si_read_data(array_t *buffer_struct)
{
	int32_t v_data_temp[2] = {BME280_INIT_VALUE, BME280_INIT_VALUE};
	uint32_t v_data_pres[2] = {BME280_INIT_VALUE, BME280_INIT_VALUE};
	uint32_t v_data_hum[2] = {BME280_INIT_VALUE, BME280_INIT_VALUE};
	int32_t result = E_ERROR;

	/* Wake up the sensor */
	result = si_wakeup();

	/* Read the sensor data */
	char *buffer = (char *)buffer_struct->bytes;
	result += bme280_read_pressure_temperature_humidity(
	&v_data_pres[1], &v_data_temp[1], &v_data_hum[1]);

	int_to_buffer(&buffer[0], (int32_t)v_data_pres[1]);
	int_to_buffer(&buffer[4], v_data_temp[1]);
	int_to_buffer(&buffer[8], (int32_t)v_data_hum[1]);

	/* Move the sensor to sleep state */
	result = si_sleep();

	buffer_struct->sz = sizeof(int) * NUM_SENSOR_DATA;
	if (result != E_ERROR)
		return true;
	else
		return false;
}

int32_t si_wakeup(void)
{
	int32_t result = E_ERROR;
	result +=  bme280_set_power_mode(BME280_NORMAL_MODE);
	return result;
}

int32_t si_sleep(void)
{

	int32_t result = E_ERROR;
	result += bme280_set_power_mode(BME280_SLEEP_MODE);
	return result;
}
