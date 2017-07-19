/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "i2c_hal.h"
#include <stm32f4xx_hal.h>

#define _CAT(a, ...)    a ## __VA_ARGS__
#define CAT(a, ...)     _CAT(a, __VA_ARGS__)
#define I2C_TIMEOUT_MS 2000
#define I2C_CLOCKSPEED 400000
#define READ_MATCH_TIMEOUT 1000

/*
 * Table mapping I2C peripherals to their IDs. The format of the table
 * is: ID, followed by the corresponding I2C peripheral.
 */
#define I2C_TABLE(X) \
		X(I1, I2C1) \
		X(I2, I2C2) \
		X(I3, I2C3)

#define CONV_HDL_TO_ID(a, b)    do {\
	if ((I2C_TypeDef *)hdl == b) \
		return a; \
} while (0);

#define ENABLE_CLOCKS(a, b) \
	if (inst == b) { \
		CAT(__HAL_RCC_, b##_CLK_ENABLE()); \
		return;     }

#define GET_IDS(a, b)		a,

enum i2c_id {
	I2C_TABLE(GET_IDS)
	NUM_I2C,
	UI		/*Invalid ID */
};

static I2C_HandleTypeDef i2c_stm32_handle[NUM_I2C];
static bool i2c_usage[NUM_I2C];
static uint32_t i2c_timeout_ms;

static enum i2c_id convert_hdl_to_id(periph_t hdl)
{
	I2C_TABLE(CONV_HDL_TO_ID);
	return UI;
}

static void enable_clock(const I2C_TypeDef *inst)
{
	I2C_TABLE(ENABLE_CLOCKS);
}

static bool init_i2c_peripheral(periph_t hdl)
{
	enum i2c_id iid = convert_hdl_to_id(hdl);

	/*Peripheral under usage*/
	if (i2c_usage[iid])
		return false;
	I2C_TypeDef *i2c_instance = (I2C_TypeDef *)hdl;
	enable_clock(i2c_instance);

	i2c_stm32_handle[iid].Instance = i2c_instance;
	i2c_stm32_handle[iid].Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	i2c_stm32_handle[iid].Init.ClockSpeed = I2C_CLOCKSPEED;
	i2c_stm32_handle[iid].Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2c_stm32_handle[iid].Init.DutyCycle = I2C_DUTYCYCLE_16_9;
	i2c_stm32_handle[iid].Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2c_stm32_handle[iid].Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&i2c_stm32_handle[iid]) != HAL_OK)
		return false;

	i2c_usage[iid] = true;
	return true;
}

periph_t i2c_init(pin_name_t scl, pin_name_t sda, uint32_t timeout_ms)
{
	/* Mapping scl, sda pins */
	periph_t p1, p2;
	p1 = pp_get_peripheral(scl, i2c_scl_map);
	p2 = pp_get_peripheral(sda, i2c_sda_map);
	if (p1 != p2 || p1 == NO_PERIPH || p2 == NO_PERIPH)
		return NO_PERIPH;
	/* Pin Initialization */
	if (!pp_peripheral_pin_init(scl, i2c_scl_map) || \
			!pp_peripheral_pin_init(sda, i2c_sda_map))
		return NO_PERIPH;

	if ((init_i2c_peripheral(p1)) == true) {
		if (!timeout_ms)
			i2c_timeout_ms = I2C_TIMEOUT_MS;
		else
			i2c_timeout_ms = timeout_ms;

		return p1;
		}
	else
		return NO_PERIPH;
}

bool i2c_write(periph_t hdl, i2c_addr_t addr, uint8_t len, const uint8_t *buf)
{
	if ((!buf) || (len == 0))
		return false;

	if (hdl == NO_PERIPH)
		return false;

	if (HAL_I2C_Mem_Write(&i2c_stm32_handle[convert_hdl_to_id(hdl)],\
		 addr.slave << 1 , addr.reg, I2C_MEMADD_SIZE_8BIT,\
			 (uint8_t *) buf , len, i2c_timeout_ms) != HAL_OK) {
		return false;
	}
	return true;
}

bool i2c_read(periph_t hdl, i2c_addr_t addr, uint8_t len, uint8_t *buf)
{
	if ((!buf) || (len == 0))
		return false;

	if (hdl == NO_PERIPH)
		return false;

	if (HAL_I2C_Mem_Read(&i2c_stm32_handle[convert_hdl_to_id(hdl)],\
		 addr.slave << 1 , addr.reg, I2C_MEMADD_SIZE_8BIT, buf ,\
			 len, i2c_timeout_ms) != HAL_OK){
		return false;
	}
	return true;
}

void i2c_pwr(periph_t hdl, bool state)
{
	/* XXX: Stub for now */
}
