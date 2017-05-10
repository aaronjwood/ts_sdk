/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include <i2c_hal.h>
/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)


#define I2C_TIMEOUT          2000    /* 2000ms timeout for I2C response */
#define READ_MATCH_TIMEOUT   1000

struct array_t {
	uint8_t sz;      /* Number of bytes contained in the data buffer */
	uint8_t *bytes;  /* Pointer to the data buffer */
};

/* PowerDevice, Inhibit update when reading, 1 Hz update */
static uint8_t hts_init1 = 0x85;
/* ONE_SHOT bit */
static uint8_t hts_init2 = 0x01;


/* Registers and data lengths internal to the sensor */
#define HTS221_ADDR             0x5f
#define HTS221_CALIB_SZ         16
#define HTS_TEMP_SZ             2
#define HTS_HUMIDITY_SZ         2
#define HTS_CALIB               0xb0
#define HTS_CTRL_REG_1          0x20
#define HTS_CTRL_REG_2          0x21
#define HTS_STATUS_REG          0x27
#define HTS_TEMP_OUT_L          0xaa
#define HTS_HUMIDITY_OUT_L      0xa8
#define HTS_WHO_AM_I            0x0f

static bool read_until_matched_byte(periph_t i2c_handle,
		i2c_addr_t i2cDestAddr,  uint8_t mask, uint8_t expected)
{
	uint8_t value;
	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	do {
		EOE(i2c_read(i2c_handle, i2cDestAddr , 1, &value));
		if ((value & mask) == expected)
			return true;
		end = sys_get_tick_ms();
	} while (end - start < READ_MATCH_TIMEOUT);
	return false;
}


int main(int argc, char *argv[])
{
	sys_init();

	dbg_module_init();
	struct array_t data;
	i2c_addr_t i2cDestAddr;
	int i;
	uint8_t a[16] = {0};
	data.bytes = a;
	dbg_printf("Begin:\n");
	 /* PB8 = SCL; PB9 = SDA */
	periph_t i2c_handle =  i2c_init(PB8, PB9);

	i2cDestAddr.slave = HTS221_ADDR;
	i2cDestAddr.reg = HTS_WHO_AM_I;
	EOE(i2c_read(i2c_handle, i2cDestAddr, 1, data.bytes));
	if (*(data.bytes) == 0xBC)
		dbg_printf("Sensor identified as HTS221 Sensor\n");

	/* Calibrating the sensor -  Only HTS221 has a calibration table */
	i2cDestAddr.reg = HTS_CALIB;
	data.sz = HTS221_CALIB_SZ;
	EOE(i2c_read(i2c_handle, i2cDestAddr, data.sz, data.bytes));
	for (i = 0; i < 16 ; i++)
		dbg_printf("After calibration data[%d] = %x\n", i,\
						 data.bytes[i]);
	i2cDestAddr.reg = HTS_CTRL_REG_1;
	/*HTS221 - writing in to the  sensor*/
	dbg_printf("Initializing sensor..\n");
	EOE(i2c_write(i2c_handle, i2cDestAddr, 1 ,  &hts_init1));
	i2cDestAddr.reg = HTS_CTRL_REG_2;
	EOE(i2c_write(i2c_handle, i2cDestAddr, 1 , &hts_init2));

	dbg_printf("Reading the Temp Sensor.....\n");
	i2cDestAddr.reg = HTS_STATUS_REG;
	EOE(read_until_matched_byte(i2c_handle, i2cDestAddr, 0x01, 0x01));

	i2cDestAddr.reg = HTS_TEMP_OUT_L;
	data.sz = HTS_TEMP_SZ;
	/* Reading the sensor */
	EOE(i2c_read(i2c_handle, i2cDestAddr , data.sz, data.bytes));
	dbg_printf("Value of Temp Sensor = %2x%2x\n", *(data.bytes),\
						 *(data.bytes+1));
	dbg_printf("Reading the Humidity Sensor....\n");
	EOE(read_until_matched_byte(i2c_handle, i2cDestAddr, 0x02, 0x02));

	i2cDestAddr.reg = HTS_HUMIDITY_OUT_L;
	data.sz = HTS_HUMIDITY_SZ;
	/* Reading the sensor */

	EOE(i2c_read(i2c_handle, i2cDestAddr , data.sz, data.bytes));
	dbg_printf("Value of Humidity  Sensor = %2x%2x\n", *(data.bytes),\
							 *(data.bytes+1));
	return 0;
}
