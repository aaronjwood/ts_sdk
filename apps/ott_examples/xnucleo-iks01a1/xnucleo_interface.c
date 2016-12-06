/* Copyright(C) 2016 Verizon. All rights reserved. */

/*
 * Read the following sensors
 * 1) HTS221
 * 2) LSM6DS0
 * 3) LPS25HB
 * 4) LIS3MDL
 */

#include <stm32f4xx_hal.h>
#include <stdbool.h>

#include "sensor_interface.h"
#include "platform.h"

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while(0)

#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */
static I2C_HandleTypeDef i2c_handle;

#define READ_MATCH_TIMEOUT	1000	/* 1000ms timeout for read until matched */

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

/* I2C Sensor Write */
static bool i2c_sw(uint8_t dev, uint8_t addr, uint8_t *data, uint16_t sz)
{
	HAL_StatusTypeDef s;
	s = HAL_I2C_Mem_Write(&i2c_handle, dev << 1,
			addr, I2C_MEMADD_SIZE_8BIT,
			data, sz, I2C_TIMEOUT);
	return (s == HAL_OK);
}

/* I2C Sensor Read */
static bool i2c_sr(uint8_t dev, uint8_t addr, uint8_t *data, uint16_t sz)
{
	HAL_StatusTypeDef s;
	s = HAL_I2C_Mem_Read(&i2c_handle, dev << 1,
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

static bool init_sensors(void)
{
	/* HTS221 */
	EOE(i2c_sw(HTS221_ADDR, HTS_CTRL_REG_1, &hts_init1, 1));
	EOE(i2c_sw(HTS221_ADDR, HTS_CTRL_REG_2, &hts_init2, 1));

	/* LSM6DS0 */
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG1_G, &lsm_init[0], 1));
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG2_G, &lsm_init[1], 1));
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG3_G, &lsm_init[2], 1));
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG4, &lsm_init[3], 1));
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG6_XL, &lsm_init[4], 1));
	EOE(i2c_sw(LSM6DS0_ADDR, LSM_CTRL_REG5_XL, &lsm_init[5], 1));

	/* LPS25HB - No initialization needed */

	/* LIS3MDL */
	EOE(i2c_sw(LIS3MDL_ADDR, LIS_CTRL_REG3, &lis_init[0], 1));
	EOE(i2c_sw(LPS25HB_ADDR, LIS_CTRL_REG5, &lis_init[1], 1));

	return true;
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
	if (HAL_I2C_Init(&i2c_handle) != HAL_OK)
		return false;

	return init_sensors();
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	/* Only HTS221 has a calibration table */
	if (idx > HTS221)
		return false;
	if (max_sz < HTS221_CALIB_SZ)
		return false;
	data->sz = HTS221_CALIB_SZ;
	EOE(i2c_sr(HTS221_ADDR, HTS_CALIB, data->bytes, HTS221_CALIB_SZ));
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}

/*
 * Wait until the value of the register 'reg', of I2C device 'dev',  masked by
 * 'mask' reads the expected value. In case it takes too long, timeout.
 */
static bool read_until_matched_byte(uint8_t dev, uint8_t reg,
		uint8_t mask, uint8_t expected)
{
	uint8_t value;
	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	do {
		EOE(i2c_sr(dev, reg, &value, 1));
		if ((value & mask) == expected)
			return true;
	} while(end - start < READ_MATCH_TIMEOUT);
	return false;
}

static bool read_hts221(uint16_t max_sz, array_t *data)
{
	if (max_sz < HTS_TEMP_SZ + HTS_HUMIDITY_SZ)
		return false;
	data->sz = HTS_TEMP_SZ + HTS_HUMIDITY_SZ;
	EOE(read_until_matched_byte(HTS221_ADDR, HTS_STATUS_REG, 0x01, 0x01));
	EOE(i2c_sr(HTS221_ADDR, HTS_TEMP_OUT_L, data->bytes, HTS_TEMP_SZ));
	EOE(read_until_matched_byte(HTS221_ADDR, HTS_STATUS_REG, 0x02, 0x02));
	EOE(i2c_sr(HTS221_ADDR, HTS_HUMIDITY_OUT_L,
				data->bytes + HTS_TEMP_SZ, HTS_HUMIDITY_SZ));
	return true;
}

static bool read_lsm6ds0(uint16_t max_sz, array_t *data)
{
	if (max_sz < LSM_XLIN_SZ + LSM_YLIN_SZ + LSM_ZLIN_SZ + LSM_XA_SZ +
			LSM_YA_SZ + LSM_ZA_SZ)
		return false;
	data->sz = LSM_XLIN_SZ + LSM_YLIN_SZ + LSM_ZLIN_SZ + LSM_XA_SZ +
		LSM_YA_SZ + LSM_ZA_SZ;
	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_X_XL, data->bytes, LSM_XLIN_SZ));
	uint8_t offset = LSM_XLIN_SZ;
	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_Y_XL, data->bytes + offset, LSM_YLIN_SZ));
	offset += LSM_YLIN_SZ;
	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_Z_XL, data->bytes + offset, LSM_ZLIN_SZ));
	offset += LSM_ZLIN_SZ;

	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_X_G, data->bytes + offset, LSM_XA_SZ));
	offset += LSM_XA_SZ;
	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_Y_G, data->bytes + offset, LSM_YA_SZ));
	offset += LSM_YA_SZ;
	EOE(i2c_sr(LSM6DS0_ADDR, LSM_OUT_Z_G, data->bytes + offset, LSM_ZA_SZ));
	return true;
}

static bool read_lps25hb(uint16_t max_sz, array_t *data)
{
	if (max_sz < LPS_PRES_SZ + LPS_TEMP_SZ)
		return false;
	data->sz = LPS_PRES_SZ + LPS_TEMP_SZ;
	EOE(i2c_sw(LPS25HB_ADDR, LPS_CTRL_REG1, &lps_action[0], 1));
	EOE(i2c_sw(LPS25HB_ADDR, LPS_CTRL_REG2, &lps_action[1], 1));
	EOE(read_until_matched_byte(LPS25HB_ADDR, LPS_STATUS_REG, 0x02, 0x02));
	EOE(i2c_sr(LPS25HB_ADDR, LPS_PRESS_OUT_XL, data->bytes, LPS_PRES_SZ));
	EOE(read_until_matched_byte(LPS25HB_ADDR, LPS_STATUS_REG, 0x01, 0x01));
	EOE(i2c_sr(LPS25HB_ADDR, LPS_TEMP_OUT_L,
				data->bytes + LPS_PRES_SZ, LPS_TEMP_SZ));
	return true;
}

static bool read_lis3mdl(uint16_t max_sz, array_t *data)
{
	if (max_sz < LIS_DATA_SZ)
		return false;
	data->sz = LIS_DATA_SZ;
	EOE(read_until_matched_byte(LIS3MDL_ADDR, LIS_STATUS_REG, 0x04, 0x04));
	EOE(i2c_sr(LIS3MDL_ADDR, 0x80 | LIS_OUT_X_L, data->bytes, LIS_DATA_SZ));
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
