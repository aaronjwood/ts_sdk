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
 * Target board : Nucleo-L476RG
 * Target MCU   : STM32F476RGT
 */

#define END_OF_MAP	{NC, {0}, 0, 0}

#define DEFAULT_UART_PIN_CONF \
{ \
	.pull_mode = PP_PULL_UP, \
	.speed = SPEED_HIGH \
}

const pin_map_t uart_tx_map[] = {
	{PA3, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2}, /* Ethernet */
	{PC4, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PA9, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1}, /* USB */
	{PC10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB6, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},
	{PC1, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART1},
	{PA0, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	{PC10, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	{PC12, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART5},
	END_OF_MAP
};

const pin_map_t uart_rx_map[] = {
	/*{PA1, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},*/ /* Ethernet */
	{PA2, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	/*{PA10, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* USB */
	/*{PB7, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},*/ /* LED2 */
	{PC5, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB11, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART3},
	{PA10, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART1},
	{PC11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB7, DEFAULT_UART_PIN_CONF, AF8, (periph_t)USART1},
	{PC0, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART1},
	{PA1, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART4},
	{PC11, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART4},
	{PD2, DEFAULT_UART_PIN_CONF, AF8, (periph_t)UART5},
	END_OF_MAP
};

const pin_map_t uart_rts_map[] = {
	{PA0, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PA6, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB13, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PA11, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1}, /* USB */
	{PD2, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB3, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},
	{PA15, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART4},
	{PB4, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART5},
	END_OF_MAP
};

const pin_map_t uart_cts_map[] = {
	{PA1, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART2},
	{PB1, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PB14, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART3},
	{PA12, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1}, /* USB */
	{PB4, DEFAULT_UART_PIN_CONF, AF7, (periph_t)USART1},
	{PB5, DEFAULT_UART_PIN_CONF, AF7, (periph_t)UART5},
	END_OF_MAP
};

#define DEFAULT_I2C_PIN_CONF \
{ \
	.pull_mode = OD_PULL_UP, \
	.speed = SPEED_HIGH \
}

const pin_map_t i2c_sda_map[] = {
	{PC1, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C3},
	{PB11, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PB14, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PB7, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	{PB9, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	END_OF_MAP
};

const pin_map_t i2c_scl_map[] = {
	{PC0, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C3},
	{PB10, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PB13, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C2},
	{PB6, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	{PB8, DEFAULT_I2C_PIN_CONF, AF4, (periph_t)I2C1},
	END_OF_MAP
};
