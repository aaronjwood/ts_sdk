/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "uart_hal.h"

#define MAX_IRQ_PRIORITY	15

/* List of supported UARTs */
enum uart_id {
	U1,
	U2,
	U3,
	U4,
	U5,
	U6,
	U7,
	U8,
	NUM_UART,
	UI		/* Invalid UART ID */
};

/* Tables of callbacks, usage counters and STM32 HAL handles */
static uart_rx_char_cb rx_char_cb[NUM_UART];
static UART_HandleTypeDef uart_handle[NUM_UART];
statc bool uart_usage[NUM_UART];

static enum uart_id convert_hdl_to_id(periph_t hdl)
{
	if ((USART_TypeDef *)hdl == USART1)
		return U1;
	else if((USART_TypeDef *)hdl == USART2)
		return U2;
	else if((USART_TypeDef *)hdl == USART3)
		return U3;
	else if((USART_TypeDef *)hdl == UART4)
		return U4;
	else if((USART_TypeDef *)hdl == UART5)
		return U5;
	else if((USART_TypeDef *)hdl == USART5)
		return U6;
	else if((USART_TypeDef *)hdl == UART7)
		return U7;
	else if((USART_TypeDef *)hdl == UART8)
		return U8;
	else
		return UI;
}

static bool validate_config(const uart_config *config)
{
	/* XXX: For now, limit the baud rate to these speeds */
	if (config->baud != 9600 &&
			config->baud != 19200 &&
			config->baud != 38400 &&
			config->baud != 57600 &&
			config->baud != 115200)
		return false;

	if (config->data != 8 && config->data_width != 9)
		return false;

	if (config->parity != NONE &&
			config->parity != ODD &&
			config->parity != EVEN)
		return false;

	if (config->stop_bits != 1 && config->stop_bits != 2)
		return false;

	if (config->priority > MAX_IRQ_PRIORITY)
		return false;

	return true;
}

static bool init_uart_peripheral(UART_HandleTypeDef *uart)
{
	/* TODO */
	return true;
}

periph_t uart_init(struct uart_pins pins, uart_config config)
{
	if (!validate_config(&config))
		return NO_PERIPH;

	/* Make sure at least one of RX / TX is specified */
	if (pins.tx == NC && pins.rx == NC)
		return NO_PERIPH;

	/* Determine if hardware flow control should be enabled */
	bool hw_flow_control = pins.rts != NC && pins.cts != NC;

	/* Make sure all pins connect to the same peripheral */
	periph_t p1, p2, p3, p4;
	p1 = pp_get_peripheral(pins.tx, uart_tx_map);
	p2 = pp_get_peripheral(pins.rx, uart_rx_map);
	p3 = pp_get_peripheral(pins.rts, uart_rts_map);
	p4 = pp_get_peripheral(pins.cts, uart_cts_map);

	if (p1 != p2 || p2 != p3 || p3 != p4 || p1 == NO_PERIPH)
		return NO_PERIPH;

	/* Initialize the pins connected to the peripheral */
	if (!pp_peripheral_pin_init(pins.tx, uart_tx_map) ||
			!pp_peripheral_pin_init(pins.rx, uart_rx_map) ||
			!pp_peripheral_pin_init(pins.rts, uart_rts_map) ||
			!pp_peripheral_pin_init(pins.cts, uart_cts_map))
		return NO_PERIPH;

	/* Initialize the peripheral itself */
	return init_uart_peripheral((USART_TypeDef *)p1) ? p1 : NO_PERIPH;
}
