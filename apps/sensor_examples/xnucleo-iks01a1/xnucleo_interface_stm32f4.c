/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/*
 * Sensor module for the following sensors:
 * 1) HTS221 (Temperature and humidity)
 * 2) LSM6DS0 (Accelerometer)
 * 3) LPS25HB (Temperature and pressure)
 * 4) LIS3MDL (Magnetometer)
 */

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

#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */
periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;

#define READ_MATCH_TIMEOUT	1000   /* 1000ms timeout - read until matched*/

enum sensors {
	HTS221,
	LSM6DS0,
	LPS25HB,
	LIS3MDL,
	NUM_SENSORS
};

/* Registers and data lengths internal to the sensor */
#define HTS221_ADDR		0x5f
#define HTS221_CALIB_SZ		16
#define HTS_TEMP_SZ		2
#define HTS_HUMIDITY_SZ		2
#define HTS_CALIB		0xb0
#define HTS_CTRL_REG_1		0x20
#define HTS_CTRL_REG_2		0x21
#define HTS_STATUS_REG		0x27
#define HTS_TEMP_OUT_L		0xaa
#define HTS_HUMIDITY_OUT_L	0xa8

/* PowerDevice, Inhibit update when reading, 1 Hz update */
static uint8_t hts_init1 = 0x85;
/* ONE_SHOT bit */
static uint8_t hts_init2 = 0x01;

#define LSM6DS0_ADDR		0x6b
#define LSM_XLIN_SZ		0x02
#define LSM_YLIN_SZ		0x02
#define LSM_ZLIN_SZ		0x02
#define LSM_XA_SZ		0x02
#define LSM_YA_SZ		0x02
#define LSM_ZA_SZ		0x02
#define LSM_CTRL_REG1_G		0x10
#define LSM_CTRL_REG2_G		0x11
#define LSM_CTRL_REG3_G		0x12
#define LSM_CTRL_REG4		0x1e
#define LSM_CTRL_REG5_XL	0x1f
#define LSM_CTRL_REG6_XL	0x20
#define LSM_OUT_X_XL		0x28
#define LSM_OUT_Y_XL		0x2a
#define LSM_OUT_Z_XL		0x2c
#define LSM_OUT_X_G		0x18
#define LSM_OUT_Y_G		0x1a
#define LSM_OUT_Z_G		0x1c

static uint8_t lsm_init[6] = {
	0x78,			/* ODR<011> FS<11> */
	0x02,			/* OUT_SEL<10> */
	(0x40 | 0x00),		/* HP_EN<1> | HPCF_G<0000> */
	0x38,			/* Zen_G | Yen_G | Xen_G */
	0x60,			/* ODR_XL1 | ODR_XL0 */
	0x38			/* Zen_XL | Yen_XL | Xen_XL */
};

#define LPS25HB_ADDR		0x5d
#define LPS_PRES_SZ		0x03
#define LPS_TEMP_SZ		0x02
#define LPS_CTRL_REG1		0x20
#define LPS_CTRL_REG2		0x21
#define LPS_STATUS_REG		0x27
#define LPS_PRESS_OUT_XL	0xa8
#define LPS_TEMP_OUT_L		0xab

static uint8_t lps_action[] = {
	(0x80 | 0x04),		/* PD | BDU Default: 1-shot */
	0x01			/* ONESHOT Default:FIFO+Autozero disable */
};

#define LIS3MDL_ADDR		0x1e
#define LIS_DATA_SZ		0x06
#define LIS_CTRL_REG3		0x22
#define LIS_CTRL_REG5		0x24
#define LIS_STATUS_REG		0x27
#define LIS_OUT_X_L		0x28

static uint8_t lis_init[] = {
	0x00,			/* MD<00> Continuous conversion */
	0x40			/* BDU<1> */
};


static bool init_sensors(void)
{
	/* HTS221 */
	i2c_dest_addr.slave = HTS221_ADDR;
	i2c_dest_addr.reg = HTS_CTRL_REG_1;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &hts_init1));
	i2c_dest_addr.slave = HTS221_ADDR;
	i2c_dest_addr.reg = HTS_CTRL_REG_2;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 , &hts_init2));

	/* LSM6DS0 */
	i2c_dest_addr.slave = LSM6DS0_ADDR;
	i2c_dest_addr.reg = LSM_CTRL_REG1_G;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[0]));
	i2c_dest_addr.reg = LSM_CTRL_REG2_G;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[1]));
	i2c_dest_addr.reg = LSM_CTRL_REG3_G;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[2]));
	i2c_dest_addr.reg = LSM_CTRL_REG4;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[3]));
	i2c_dest_addr.reg = LSM_CTRL_REG6_XL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[4]));
	i2c_dest_addr.reg = LSM_CTRL_REG5_XL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lsm_init[5]));

	/* LPS25HB - No initialization needed */

	/* LIS3MDL */
	i2c_dest_addr.slave = LIS3MDL_ADDR;
	i2c_dest_addr.reg = LIS_CTRL_REG3;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lis_init[0]));
	i2c_dest_addr.reg = LIS_CTRL_REG5;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &lis_init[1]));

	return true;
}

bool si_init(void)
{
	/* Initialize the I2C bus on the processor */
	i2c_handle =  i2c_init(PB8, PB9);

	if (i2c_handle != NO_PERIPH)
		return init_sensors();

	return false;
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	int i;
	/* Only HTS221 has a calibration table */
	if (idx != HTS221)
		return false;
	if (max_sz < HTS221_CALIB_SZ)
		return false;
	data->sz = HTS221_CALIB_SZ;
	i2c_dest_addr.slave = HTS221_ADDR;
	i2c_dest_addr.reg = HTS_CALIB;

	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz,  data->bytes));
	for (i = 0; i < HTS221_CALIB_SZ ; i++)
		dbg_printf("After calibration data[%d] = %x\n", i, \
				data->bytes[i]);
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}

/*
 * Wait until the value of the register 'reg' of I2C device 'dev' masked by
 * 'mask' reads the expected value. In case it takes too long, timeout.
 */
static bool read_until_matched_byte(uint8_t dev, uint8_t reg,
		uint8_t mask, uint8_t expected)
{
	uint8_t value;
	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	do {
		i2c_dest_addr.slave = dev;
		i2c_dest_addr.reg = reg;
		EOE(i2c_read(i2c_handle, i2c_dest_addr, 1, &value));
		if ((value & mask) == expected)
			return true;
		end = sys_get_tick_ms();
	} while (end - start < READ_MATCH_TIMEOUT);
	return false;
}

static bool read_hts221(uint16_t max_sz, array_t *data)
{
	if (max_sz < HTS_TEMP_SZ + HTS_HUMIDITY_SZ)
		return false;
	data->sz = HTS_TEMP_SZ + HTS_HUMIDITY_SZ;
	EOE(read_until_matched_byte(HTS221_ADDR, HTS_STATUS_REG, 0x01, 0x01));
	i2c_dest_addr.slave = HTS221_ADDR;
	i2c_dest_addr.reg = HTS_TEMP_OUT_L;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, HTS_TEMP_SZ, data->bytes));
	dbg_printf("APPS: Value of Temperature  Sensor = %2x%2x\n",\
					 *(data->bytes), *((data->bytes)+1));
	EOE(read_until_matched_byte(HTS221_ADDR, HTS_STATUS_REG, 0x02, 0x02));
	i2c_dest_addr.reg = HTS_HUMIDITY_OUT_L;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, HTS_HUMIDITY_SZ,\
						 data->bytes + HTS_TEMP_SZ));
	dbg_printf("APPS: Value of Humidity  Sensor = %2x%2x\n",\
	*((data->bytes) + HTS_TEMP_SZ) , *((data->bytes)+HTS_TEMP_SZ+1));

	return true;
}

static bool read_lsm6ds0(uint16_t max_sz, array_t *data)
{
	if (max_sz < LSM_XLIN_SZ + LSM_YLIN_SZ + LSM_ZLIN_SZ + LSM_XA_SZ +
			LSM_YA_SZ + LSM_ZA_SZ)
		return false;
	data->sz = LSM_XLIN_SZ + LSM_YLIN_SZ + LSM_ZLIN_SZ + LSM_XA_SZ +
		LSM_YA_SZ + LSM_ZA_SZ;
	i2c_dest_addr.slave = LSM6DS0_ADDR;
	i2c_dest_addr.reg = LSM_OUT_X_XL;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_XLIN_SZ, data->bytes));
	uint8_t offset = LSM_XLIN_SZ;
	i2c_dest_addr.reg = LSM_OUT_Y_XL;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_YLIN_SZ,\
					 data->bytes + offset));
	offset += LSM_YLIN_SZ;
	i2c_dest_addr.reg = LSM_OUT_Z_XL;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_ZLIN_SZ, \
					data->bytes + offset));
	offset += LSM_ZLIN_SZ;

	i2c_dest_addr.reg = LSM_OUT_X_G;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_XA_SZ, \
					data->bytes + offset));
	offset += LSM_XA_SZ;
	i2c_dest_addr.reg = LSM_OUT_Y_G;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_YA_SZ, \
					data->bytes + offset));
	offset += LSM_YA_SZ;
	i2c_dest_addr.reg = LSM_OUT_Z_G;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LSM_ZA_SZ, \
					data->bytes + offset));
	return true;
}

static bool read_lps25hb(uint16_t max_sz, array_t *data)
{
	if (max_sz < LPS_PRES_SZ + LPS_TEMP_SZ)
		return false;
	data->sz = LPS_PRES_SZ + LPS_TEMP_SZ;
	i2c_dest_addr.slave = LPS25HB_ADDR;
	i2c_dest_addr.reg = LPS_CTRL_REG1;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &lps_action[0]));
	i2c_dest_addr.reg = LPS_CTRL_REG2;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &lps_action[1]));

	EOE(read_until_matched_byte(LPS25HB_ADDR, LPS_STATUS_REG, 0x02, 0x02));
	i2c_dest_addr.slave = LPS25HB_ADDR;
	i2c_dest_addr.reg = LPS_PRESS_OUT_XL;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LPS_PRES_SZ, data->bytes));
	EOE(read_until_matched_byte(LPS25HB_ADDR, LPS_STATUS_REG, 0x01, 0x01));
	i2c_dest_addr.reg = LPS_TEMP_OUT_L;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, LPS_TEMP_SZ,\
					 data->bytes + LPS_PRES_SZ));

	return true;
}

static bool read_lis3mdl(uint16_t max_sz, array_t *data)
{
	if (max_sz < LIS_DATA_SZ)
		return false;
	data->sz = LIS_DATA_SZ;
	EOE(read_until_matched_byte(LIS3MDL_ADDR, LIS_STATUS_REG, 0x04, 0x04));
	i2c_dest_addr.slave = LIS3MDL_ADDR;
	i2c_dest_addr.reg = 0x80 | LIS_OUT_X_L;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, LIS_DATA_SZ , data->bytes));
	return true;
}

bool si_read_data(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (idx > NUM_SENSORS - 1)
		return false;
	switch (idx) {
	case HTS221:
		EOE(read_hts221(max_sz, data));
		break;
	case LSM6DS0:
		EOE(read_lsm6ds0(max_sz, data));
		break;
	case LPS25HB:
		EOE(read_lps25hb(max_sz, data));
		break;
	case LIS3MDL:
		EOE(read_lis3mdl(max_sz, data));
		break;
	default:
		return false;
	}
	return true;
}

bool si_sleep(void)
{
	/* None of the sensors have a sleep sequence */
	return true;
}

bool si_wakeup(void)
{
	/* None of the sensors have a wakeup sequence */
	return true;
}
