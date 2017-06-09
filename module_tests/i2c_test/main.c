/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "i2c_hal.h"

#define BME280_BEDUIN
/* #define HTS221_NUCLEO */

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)


#define READ_MATCH_TIMEOUT   1000

struct array_t {
	uint8_t sz;      /* Number of bytes contained in the data buffer */
	uint8_t *bytes;  /* Pointer to the data buffer */
};

#ifdef BME280_BEDUIN
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

#endif

#ifdef HTS221_NUCLEO
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
#define HTS_DEV_IDENTIFIER       0xBC

static bool read_until_matched_byte(periph_t i2c_handle,
		i2c_addr_t i2c_dest_addr,  uint8_t mask, uint8_t expected)
{
	uint8_t value;
	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	do {
		EOE(i2c_read(i2c_handle, i2c_dest_addr , 1, &value));
		if ((value & mask) == expected)
			return true;
		end = sys_get_tick_ms();
	} while (end - start < READ_MATCH_TIMEOUT);
	return false;
}
#endif

int main()
{
	sys_init();

	dbg_module_init();
	struct array_t data;
	i2c_addr_t i2c_dest_addr;

	#ifdef BME280_BEDUIN
	uint8_t a[BME280_CALIB_SZ] = {0};
	data.bytes = a;
	dbg_printf("Begin:\n");
	 /* PB6= SCL; PB7 = SDA */
	periph_t i2c_handle =  i2c_init(PB6, PB7);
	ASSERT(!(i2c_handle == NO_PERIPH));
	i2c_dest_addr.slave = BME280_ADDR;
	i2c_dest_addr.reg = BME280_ID;
	dbg_printf("Calling i2c_read\n");
	EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, data.bytes));
	dbg_printf("data_bytes = %d\n", *(data.bytes));
	if (*(data.bytes) == BME280_DEV_IDENTIFIER)
		dbg_printf("Sensor identified as BME280 Sensor\n");
	else
		dbg_printf("Wrong Sensor !!!\n");

	i2c_dest_addr.reg = BME280_CALIB_ADDR1;
	data.sz = BME280_CALIB_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data.sz, data.bytes));
	for (int i = 0; i < BME280_CALIB_SZ ; i++)
		dbg_printf("BME280 calibration table data[%d] = 0x%x\n", i,\
							data.bytes[i]);
	while (1) {
		i2c_dest_addr.reg = BME280_CTRL_REG;
		/*BME280 - writing in to the  sensor*/
		dbg_printf("Setting Power mode to normal mode..\n");
		EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 , &bme_value));
		dbg_printf("Reading the BME280 Temp value.....\n");
		i2c_dest_addr.reg = BME280_TEMP_MSB;
		data.sz = BME280_TEMP_SZ;
		/* Reading the sensor */
		EOE(i2c_read(i2c_handle, i2c_dest_addr, data.sz, data.bytes));
		dbg_printf("Value of BME280 Temp Sensor = %2x%2x%2x\n",\
			*(data.bytes),  *(data.bytes+1), *(data.bytes+2));
	}
	#endif

	#ifdef HTS221_NUCLEO
	uint8_t a[HTS221_CALIB_SZ] = {0};
	data.bytes = a;
	dbg_printf("Begin:\n");
	/* PB8 = SCL; PB9 = SDA */
	periph_t i2c_handle =  i2c_init(PB8, PB9);
	ASSERT(!(i2c_handle == NO_PERIPH));
	i2c_dest_addr.slave = HTS221_ADDR;
	i2c_dest_addr.reg = HTS_WHO_AM_I;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, data.bytes));
	if (*(data.bytes) == HTS_DEV_IDENTIFIER)
			dbg_printf("Sensor identified as HTS221 Sensor\n");

	/* Retrieving the calibration table
	Only HTS221 has a calibration table */
	i2c_dest_addr.reg = HTS_CALIB;
	data.sz = HTS221_CALIB_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data.sz, data.bytes));
	for (int i = 0; i < HTS221_CALIB_SZ ; i++)
		dbg_printf("HTS221 calibration table data[%d] = %x\n", i,\
							data.bytes[i]);

	while (1) {
		i2c_dest_addr.reg = HTS_CTRL_REG_1;
		/*HTS221 - writing in to the  sensor*/
		dbg_printf("Initializing HTS221 sensor..\n");
		EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &hts_init1));
		i2c_dest_addr.reg = HTS_CTRL_REG_2;
		EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 , &hts_init2));

		dbg_printf("Reading the HTS221 Temp value.....\n");
		i2c_dest_addr.reg = HTS_STATUS_REG;
		EOE(read_until_matched_byte(i2c_handle, i2c_dest_addr,\
								 0x01, 0x01));
		i2c_dest_addr.reg = HTS_TEMP_OUT_L;
		data.sz = HTS_TEMP_SZ;
		/* Reading the sensor */
		EOE(i2c_read(i2c_handle, i2c_dest_addr , data.sz, data.bytes));
		dbg_printf("Value of HTS221 Temp Sensor = %2x%2x\n",\
						*(data.bytes), *(data.bytes+1));
		dbg_printf("Reading the HTS221 Humidity value....\n");
		EOE(read_until_matched_byte(i2c_handle, i2c_dest_addr,\
								 0x02, 0x02));
		i2c_dest_addr.reg = HTS_HUMIDITY_OUT_L;
		data.sz = HTS_HUMIDITY_SZ;
		/* Reading the sensor */

		EOE(i2c_read(i2c_handle, i2c_dest_addr , data.sz, data.bytes));
		dbg_printf("Value of HTS221 Humidity  Sensor = %2x%2x\n",\
					 *(data.bytes),	 *(data.bytes+1));
		sys_delay(1000);
	}
	#endif
	return 0;
}
