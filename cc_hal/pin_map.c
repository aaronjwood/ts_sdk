/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stm32f4xx_hal.h>
#include "pin_map.h"

/*
 * The following tables map each pin to the possible peripherals along with the
 * alternate function ID used to connect to the peripheral.
 *
 * Note that while all possible mappings are listed here, only some of them
 * may be valid for a given board. Invalid mappings (i.e. pins already hardwired
 * to perform some other function) are commented out.
 *
 * Target board : Nucleo-F429ZI
 * Target MCU   : STM32F429ZIT
 */

#define DEFAULT_UART_PIN_CONF \
{ \
	.pull_mode = PP_PULL_UP, \
	.speed = SPEED_HIGH \
}

const pin_map_t uart_tx_map[] = {
	{PA0, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	/*{PA2, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},*/ /* Ethernet */
	/*{PA9, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* USB */
	{PB6, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},
	{PB10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PC6, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART6},
	{PC10, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	{PC10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PC12, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART5},
	{PD5, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PD8, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PE1, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART8},
	{PE8, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART7},
	{PF7, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART7},
	{PG14, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART6},
	END_OF_MAP
};

const pin_map_t uart_rx_map[] = {
	/*{PA1, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},*/ /* Ethernet */
	{PA3, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	/*{PA10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* USB */
	/*{PB7, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* LED2 */
	{PB11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PC7, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART6},
	{PC11, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	{PC11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PD2, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART5},
	{PD6, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PD9, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PE0, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART8},
	{PE7, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART7},
	{PF6, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART7},
	{PG9, DEFAULT_UART_PIN_CONF, AF6, (periph_t)USART6},
	END_OF_MAP
};

const pin_map_t uart_rts_map[] = {
	/*{PA1, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},*/ /* Ethernet */
	/*{PA12, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* USB */
	/*{PB14, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},*/ /* LED3 */
	{PD4, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PD12, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PG8, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART6},
	{PG12, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART6},
	END_OF_MAP
};

const pin_map_t uart_cts_map[] = {
	{PA0, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	/*{PA11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* USB */
	{PB13, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3}, /* With JP7 open */
	{PD3, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PD11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	/*{PG13, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART6},*/ /* Ethernet */
	{PG15, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART6},
	END_OF_MAP
};

#define DEFAULT_I2C_PIN_CONF \
{ \
	.pull_mode = OD_PULL_UP, \
	.speed = SPEED_HIGH \
}

const pin_map_t i2c_sda_map[] = {
	/*{PB7, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},*/ /* LED2 */
	{PB9, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	{PB11, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PC9, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C3},
	/*{PF0, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},*/ /* MCO */
	END_OF_MAP
};

const pin_map_t i2c_scl_map[] = {
	{PA8, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C3},
	{PB6, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	{PB8, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	{PB10, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PF1, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2}, /* No external clock */
	END_OF_MAP
};
