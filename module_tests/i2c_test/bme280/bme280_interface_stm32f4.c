/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "i2c_hal.h"
#include "sensor_i2c_interface.h"

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)


#define READ_MATCH_TIMEOUT   1000
periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;

/* Setting Powermode for the Sensor */
static uint8_t bme_value = 0x03;
#define BME280_ADDR             0x77
#define BME280_CTRL_REG         0xF4
#define BME280_STATUS_REG       0xF3
#define BME280_TEMP_XLSB        0xFC
#define BME280_TEMP_LSB         0xFB
#define BME280_TEMP_MSB         0xFA
#define BME280_TEMP_SZ          3
#define BME280_CALIB_ADDR1      0x88
#define BME280_CALIB_SZ         26
#define BME280_ID               0xD0
#define BME280_DEV_IDENTIFIER   0x60

bool si_i2c_init(void)
{
        /* Initialize the I2C bus on the processor */
        i2c_handle =  i2c_init(PB6, PB7);

        if (i2c_handle != NO_PERIPH)
                return true;
	else
 	       return false;
}
bool si_i2c_read_data(array_t *data)
{
	uint8_t a[BME280_CALIB_SZ] = {0};
	data->bytes = a;
	i2c_dest_addr.slave = BME280_ADDR;
	i2c_dest_addr.reg = BME280_ID;
	
	EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, data->bytes));
	dbg_printf("data_bytes = %d\n", *(data->bytes));
	if (*(data->bytes) == BME280_DEV_IDENTIFIER)
		dbg_printf("Sensor identified as BME280 Sensor\n");
	else
		dbg_printf("Wrong Sensor !!!\n");

	i2c_dest_addr.reg = BME280_CALIB_ADDR1;
	data->sz = BME280_CALIB_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz, data->bytes));
	for (int i = 0; i < BME280_CALIB_SZ ; i++)
		dbg_printf("BME280 calibration table data[%d] = 0x%x\n", i,\
							data->bytes[i]);
	while (1) {
		i2c_dest_addr.reg = BME280_CTRL_REG;
		/*BME280 - writing in to the  sensor*/
		dbg_printf("Setting Power mode to normal mode..\n");
		EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 , &bme_value));		
		dbg_printf("Reading the BME280 Temp value.....\n");
		i2c_dest_addr.reg = BME280_TEMP_MSB;
		data->sz = BME280_TEMP_SZ;
		/* Reading the sensor */
		EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz, data->bytes));
		dbg_printf("Value of BME280 Temp Sensor = %2x%2x%2x\n",\
			*(data->bytes),  *(data->bytes+1), *(data->bytes+2));
	}
	return 0;
}
