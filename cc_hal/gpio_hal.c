/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "gpio_hal.h"
#include <stm32f4xx_hal.h>

/* Chipset specific implementation. Target: STM32F4xx */

bool gpio_init(pin_name_t pin_name, const gpio_config_t *config)
{
	return pp_gpio_pin_init(pin_name, config);
}

bit_value_t gpio_read(pin_name_t pin_name)
{
	uint32_t pin = pd_map_drv_pin(pin_name);
	GPIO_TypeDef *port = (GPIO_TypeDef *)pd_map_drv_port(pin_name);
	GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin);
	return (pin_state == GPIO_PIN_RESET) ? PIN_LOW : PIN_HIGH;
}

void gpio_write(pin_name_t pin_name, bit_value_t bit)
{
	GPIO_PinState pin_state = (bit == PIN_LOW) ? GPIO_PIN_RESET : GPIO_PIN_SET;
	uint32_t pin = pd_map_drv_pin(pin_name);
	GPIO_TypeDef *port = (GPIO_TypeDef *)pd_map_drv_port(pin_name);
	HAL_GPIO_WritePin(port, pin, pin_state);
}

void gpio_pwr(port_id_t port, bool state)
{
	/* XXX: Stub for now */
}
