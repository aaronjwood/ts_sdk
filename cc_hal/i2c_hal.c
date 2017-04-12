#include "i2c_hal.h"
#include <stm32f4xx_hal.h>

/* MCU specific implementation. Target: STM32F429 */

static I2C_HandleTypeDef i2c_handle1, i2c_handle2;
static bool i2c1_used, i2c2_used;
#define I2C_TIMEOUT_MS	2000

static bool init_i2c_peripheral(I2C_TypeDef *i2c)
{
	I2C_HandleTypeDef *i2c_handle;
	if (i2c == I2C1 && !i2c1_used) {
		__HAL_RCC_I2C1_CLK_ENABLE();
		i2c_handle = &i2c_handle1;
		i2c1_used = true;
	} else if (i2c == I2C2 && !i2c2_used) {
		__HAL_RCC_I2C2_CLK_ENABLE();
		i2c_handle = &i2c_handle2;
		i2c2_used = true;
	} else {
		return false;
	}

	i2c_handle->Instance = i2c;
	i2c_handle->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	i2c_handle->Init.ClockSpeed = 400000;
	i2c_handle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2c_handle->Init.DutyCycle = I2C_DUTYCYCLE_16_9;
	i2c_handle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2c_handle->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

	return HAL_I2C_Init(i2c_handle) == HAL_OK;
}

periph_t i2c_init(pin_name_t scl, pin_name_t sda)
{
	/* Make sure both pins map to the same peripheral */
	periph_t p1, p2;
	p1 = pp_get_peripheral(scl, i2c_scl_map);
	p2 = pp_get_peripheral(sda, i2c_sda_map);
	if (p1 != p2 || p1 == NC || p2 == NC)
		return NC;

	/* Initialize the pins connected to the peripheral */
	if (!pp_peripheral_pin_init(scl, i2c_scl_map) ||
			!pp_peripheral_pin_init(sda, i2c_sda_map))
		return NC;

	/* Initialize the peripheral itself */
	return init_i2c_peripheral((I2C_TypeDef *)p1) ? p1 : NC;
}

bool i2c_read(periph_t hdl, uint8_t slave, uint8_t reg, uint8_t len, uint8_t *buf)
{
	I2C_HandleTypeDef *i2c_handle;
	HAL_StatusTypeDef s;

	if ((I2C_TypeDef *)hdl == I2C1)
		i2c_handle = &i2c_handle1;
	else if ((I2C_TypeDef *)hdl == I2C2)
		i2c_handle = &i2c_handle2;
	else
		return false;

	s = HAL_I2C_Mem_Read(i2c_handle, slave << 1, reg, I2C_MEMADD_SIZE_8BIT,
			buf, len, I2C_TIMEOUT_MS);

	return s == HAL_OK;
}

bool i2c_write(periph_t hdl, uint8_t slave, uint8_t reg, uint8_t len, uint8_t *buf)
{
	I2C_HandleTypeDef *i2c_handle;
	HAL_StatusTypeDef s;

	if ((I2C_TypeDef *)hdl == I2C1)
		i2c_handle = &i2c_handle1;
	else if ((I2C_TypeDef *)hdl == I2C2)
		i2c_handle = &i2c_handle2;
	else
		return false;

	s = HAL_I2C_Mem_Write(i2c_handle, slave << 1, reg, I2C_MEMADD_SIZE_8BIT,
			buf, len, I2C_TIMEOUT_MS);

	return s == HAL_OK;
}

void i2c_pwr(periph_t hdl, bool state)
{
	/* XXX: Stub for now */
}
