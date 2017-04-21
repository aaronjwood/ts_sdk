/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "uart_hal.h"
#include "ts_sdk_config.h"

#define MAX_IRQ_PRIORITY	15
#define CHECK_HANDLE(hdl, retval)	do { \
	if ((hdl) == NO_PERIPH) \
		return retval; \
} while(0)

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

static const IRQn_Type irq_vector[NUM_UART] = {
	USART1_IRQn,
	USART2_IRQn,
	USART3_IRQn,
	UART4_IRQn,
	UART5_IRQn,
	USART6_IRQn,
	UART7_IRQn,
	UART8_IRQn
};

static uart_rx_char_cb rx_char_cb[NUM_UART];
static UART_HandleTypeDef uart_stm32_handle[NUM_UART];
static bool uart_usage[NUM_UART];

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
	else if((USART_TypeDef *)hdl == USART6)
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
	if (!config)
		return false;

	/* XXX: For now, limit the baud rate to these speeds */
	if (config->baud != 9600 &&
			config->baud != 19200 &&
			config->baud != 38400 &&
			config->baud != 57600 &&
			config->baud != 115200)
		return false;

	if (config->data_width != 8 && config->data_width != 9)
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

static void enable_clock(const USART_TypeDef *inst)
{
	if (inst == USART1)
		__HAL_RCC_USART1_CLK_ENABLE();
	else if(inst == USART2)
		__HAL_RCC_USART2_CLK_ENABLE();
	else if(inst == USART3)
		__HAL_RCC_USART3_CLK_ENABLE();
	else if(inst == UART4)
		__HAL_RCC_UART4_CLK_ENABLE();
	else if(inst == UART5)
		__HAL_RCC_UART5_CLK_ENABLE();
	else if(inst == USART6)
		__HAL_RCC_USART6_CLK_ENABLE();
	else if(inst == UART7)
		__HAL_RCC_UART7_CLK_ENABLE();
	else if(inst == UART8)
		__HAL_RCC_UART8_CLK_ENABLE();
}

static bool init_uart_peripheral(periph_t hdl, const uart_config *config,
		bool hw_flow_ctrl, pin_name_t tx, pin_name_t rx)
{
	/* Peripheral currently under use */
	if (uart_usage[convert_hdl_to_id(hdl)])
		return false;

	enum uart_id uid = convert_hdl_to_id(hdl);

	USART_TypeDef *uart_instance = (USART_TypeDef *)hdl;
	enable_clock(uart_instance);

	uart_stm32_handle[uid].Instance = uart_instance;
	uart_stm32_handle[uid].Init.BaudRate = config->baud;

	uart_stm32_handle[uid].Init.WordLength =
		(config->data_width == 8) ? UART_WORDLENGTH_8B
		: (config->data_width == 9) ? UART_WORDLENGTH_9B
		: UART_WORDLENGTH_8B;

	uart_stm32_handle[uid].Init.StopBits =
		(config->stop_bits == 1) ? UART_STOPBITS_1
		: (config->stop_bits == 2) ? UART_STOPBITS_2
		: UART_STOPBITS_1;

	uart_stm32_handle[uid].Init.Parity =
		(config->parity == NONE) ? UART_PARITY_NONE
		: (config->parity == ODD) ? UART_PARITY_ODD
		: (config->parity == EVEN) ? UART_PARITY_EVEN
		: UART_PARITY_NONE;

	uart_stm32_handle[uid].Init.Mode =
		(tx != NC && rx != NC) ? UART_MODE_TX_RX
		: (tx == NC && rx != NC) ? UART_MODE_RX
		: (rx == NC && tx != NC) ? UART_MODE_TX
		: UART_MODE_TX_RX;

	uart_stm32_handle[uid].Init.HwFlowCtl =
		hw_flow_ctrl ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;

	/*
	 * Choose oversampling by 16 to increase tolerance of the receiver to
	 * noise on an asynchronous line.
	 */
	uart_stm32_handle[uid].Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&uart_stm32_handle[uid]) != HAL_OK)
		return false;

	/* Enable Error Interrupts: (Frame error, noise error, overrun error) */
	SET_BIT(uart_instance->CR3, USART_CR3_EIE);

	/* Enable the UART Parity Error and Data Register not empty Interrupts */
	SET_BIT(uart_instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

	if (rx != NC) {
		HAL_NVIC_SetPriority(irq_vector[uid], MODEM_UART_IRQ_PRIORITY, 0);
		HAL_NVIC_EnableIRQ(irq_vector[uid]);
	}
	uart_usage[convert_hdl_to_id(hdl)] = true;
	return true;
}

periph_t uart_init(const struct uart_pins *pins, const uart_config *config)
{
	if (!validate_config(config))
		return NO_PERIPH;

	/* Make sure at least one of RX / TX is specified */
	if (pins->tx == NC && pins->rx == NC)
		return NO_PERIPH;

	/* Determine if hardware flow control should be enabled */
	bool hw_fl_ctrl = pins->rts != NC && pins->cts != NC;

	/* Make sure all pins connect to the same peripheral */
	periph_t p1, p2, p3, p4;
	p1 = pp_get_peripheral(pins->tx, uart_tx_map);
	p2 = pp_get_peripheral(pins->rx, uart_rx_map);
	p3 = pp_get_peripheral(pins->rts, uart_rts_map);
	p4 = pp_get_peripheral(pins->cts, uart_cts_map);

	if (p1 != p2 || p2 != p3 || p3 != p4 || p1 == NO_PERIPH)
		return NO_PERIPH;

	/* Initialize the pins connected to the peripheral */
	if (!pp_peripheral_pin_init(pins->tx, uart_tx_map) ||
			!pp_peripheral_pin_init(pins->rx, uart_rx_map) ||
			!pp_peripheral_pin_init(pins->rts, uart_rts_map) ||
			!pp_peripheral_pin_init(pins->cts, uart_cts_map))
		return NO_PERIPH;

	/* Initialize the peripheral itself */
	return init_uart_peripheral(p1, config, hw_fl_ctrl, pins->tx, pins->rx) ?
		p1 : NO_PERIPH;
}

void uart_toggle_irq(periph_t hdl, bool state)
{
	CHECK_HANDLE(hdl, /* No return value */);
	if (state)
		HAL_NVIC_EnableIRQ(irq_vector[convert_hdl_to_id(hdl)]);
	else
		HAL_NVIC_DisableIRQ(irq_vector[convert_hdl_to_id(hdl)]);
}

void uart_set_rx_char_cb(periph_t hdl, uart_rx_char_cb cb)
{
	CHECK_HANDLE(hdl, /* No return value */);
	rx_char_cb[convert_hdl_to_id(hdl)] = cb;
}

void uart_irq_handler(periph_t hdl)
{
	CHECK_HANDLE(hdl, /* No return value */);
	uint32_t uart_sr_reg = ((USART_TypeDef *)hdl)->SR;
	uint32_t err_flags = uart_sr_reg & (uint32_t)(USART_SR_PE | USART_SR_FE
			| USART_SR_ORE | USART_SR_NE);

	/* If any errors, clear it by reading the RX data register. */
	if (err_flags) {
		volatile uint8_t data = ((USART_TypeDef *)hdl)->DR;
		UNUSED(data);
		return;
	}

	enum uart_id uid = convert_hdl_to_id(hdl);
	if (!rx_char_cb[uid])
		rx_char_cb[uid](((USART_TypeDef *)hdl)->DR);
}

bool uart_tx(periph_t hdl, uint8_t *data, uint16_t size, uint16_t timeout_ms)
{
	CHECK_HANDLE(hdl, false);
	if (!data)
		return false;

	if (HAL_UART_Transmit(&uart_stm32_handle[convert_hdl_to_id(hdl)], data,
				size, timeout_ms) != HAL_OK)
		return false;
	return true;
}
