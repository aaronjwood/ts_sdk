/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "uart_hal.h"
#include "ts_sdk_board_config.h"

#define MAX_IRQ_PRIORITY		15
#define CHECK_HANDLE(hdl, retval)	do { \
	if ((hdl) == NO_PERIPH) \
		return retval; \
} while (0)

#define _CAT(a, ...)	a ## __VA_ARGS__
#define CAT(a, ...)	_CAT(a, __VA_ARGS__)

/*
 * Table mapping UARTs / USARTs peripherals to their IDs.
 * The format of the table  is: ID, followed by the
 * corresponding UART / USART peripheral.
 */
#define UART_TABLE(X) \
	X(U1, USART1) \
	X(U2, USART2) \
	X(U3, USART3) \
	X(U4, UART4) \
	X(U5, UART5) \
	X(U6, USART6)

/*
 * Define operations on the table above. These are used to generate repetitive
 * blocks / boilerplate code.
 */
#define GET_IDS(a, b)			a,
#define GET_IRQ_VECS(a, b)		b##_IRQn,

#define DEF_IRQ_HANDLERS(a, b)	\
	void b##_IRQHandler(void) { uart_irq_handler((periph_t) b); }

#define CONV_HDL_TO_ID(a, b)	\
	if ((USART_TypeDef *)hdl == b) return a;

#define ENABLE_CLOCKS(a, b) \
	if (inst == b) { CAT(__HAL_RCC_, b##_CLK_ENABLE()); return; }

enum uart_id {
	UART_TABLE(GET_IDS)
	NUM_UART,
	UI			/* Invalid ID - Unlikely */
};

static uart_rx_char_cb rx_char_cb[NUM_UART];
static UART_HandleTypeDef uart_stm32_handle[NUM_UART];
static bool uart_usage[NUM_UART];

/*
 * Defines UART IRQ Handlers that call uart_irq_handler with the appropriate
 * peripheral as the parameter.
 */
UART_TABLE(DEF_IRQ_HANDLERS);

static const IRQn_Type irq_vec[] = {
	UART_TABLE(GET_IRQ_VECS)
};

static enum uart_id convert_hdl_to_id(periph_t hdl)
{
	UART_TABLE(CONV_HDL_TO_ID);
	return UI;
}

static void enable_clock(const USART_TypeDef *inst)
{
	UART_TABLE(ENABLE_CLOCKS);
}

static bool validate_config(const uart_config *config)
{
	if (config == NULL)
		return false;

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

static bool init_peripheral(periph_t hdl, const uart_config *config,
		bool hw_flow_ctrl, pin_name_t tx, pin_name_t rx)
{
	enum uart_id uid = convert_hdl_to_id(hdl);

	/* Peripheral currently under use */
	if (uart_usage[uid])
		return false;

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

	/* Enable the UART Parity Error and Data Register
	 * not empty Interrupts
	 */
	SET_BIT(uart_instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

	if ((rx != NC) && (rx != GPS_RX_PIN)) {
		HAL_NVIC_SetPriority(irq_vec[uid], MODEM_UART_IRQ_PRIORITY, 0);
		HAL_NVIC_EnableIRQ(irq_vec[uid]);
	}
	uart_usage[uid] = true;
	return true;
}

/*
 * Make sure all pins connect to the same peripheral. Some pins of the UART
 * are optional so they are expected to connect to NO_PERIPH.
 */
static periph_t find_common_periph(const struct uart_pins *pins)
{
	const periph_t p[] = {
		pp_get_peripheral(pins->tx, uart_tx_map),
		pp_get_peripheral(pins->rx, uart_rx_map),
		pp_get_peripheral(pins->rts, uart_rts_map),
		pp_get_peripheral(pins->cts, uart_cts_map)
	};

	const bool used[] = {
		pins->tx != NC,
		pins->rx != NC,
		pins->rts != NC,
		pins->cts != NC
	};

	periph_t p_common = NO_PERIPH;
	for (uint8_t i = 0; i < 4; i++) {
		if (!used[i])
			continue;
		if (p[i] == NO_PERIPH)
			return NO_PERIPH;
		if (p_common == NO_PERIPH)
			p_common = p[i];
		if (p[i] != p_common)
			return NO_PERIPH;
	}

	return p_common;
}

#define PIN_INIT_RET_ON_ERROR(x)	do {\
	if (pins->x != NC) \
		if (!pp_peripheral_pin_init(pins->x, uart_##x##_map)) \
			return NO_PERIPH; \
} while (0)

periph_t uart_init(const struct uart_pins *pins, const uart_config *config)
{
	if (!validate_config(config) || pins == NULL)
		return NO_PERIPH;

	/* Make sure at least one of RX / TX is specified */
	if (pins->tx == NC && pins->rx == NC)
		return NO_PERIPH;

	/* Determine if hardware flow control should be enabled */
	bool hw_fl_ctrl = pins->rts != NC && pins->cts != NC;

	/* Find a common peripheral among the pins */
	periph_t periph = find_common_periph(pins);
	if (periph == NO_PERIPH)
		return NO_PERIPH;

	/* Initialize the pins connected to the peripheral */
	PIN_INIT_RET_ON_ERROR(tx);
	PIN_INIT_RET_ON_ERROR(rx);
	PIN_INIT_RET_ON_ERROR(rts);
	PIN_INIT_RET_ON_ERROR(cts);

	/* Initialize the peripheral itself */
	return init_peripheral(periph, config, hw_fl_ctrl, pins->tx, pins->rx) ?
		periph : NO_PERIPH;
}

void uart_irq_on(periph_t hdl)
{
	CHECK_HANDLE(hdl, /* No return value */);
	HAL_NVIC_EnableIRQ(irq_vec[convert_hdl_to_id(hdl)]);
}

void uart_irq_off(periph_t hdl)
{
	CHECK_HANDLE(hdl, /* No return value */);
	HAL_NVIC_DisableIRQ(irq_vec[convert_hdl_to_id(hdl)]);
}

void uart_set_rx_char_cb(periph_t hdl, uart_rx_char_cb cb)
{
	CHECK_HANDLE(hdl, /* No return value */);
	if (cb != NULL)
		rx_char_cb[convert_hdl_to_id(hdl)] = cb;
}

void uart_irq_handler(periph_t hdl)
{
	CHECK_HANDLE(hdl, /* No return value */);
	enum uart_id uid = convert_hdl_to_id(hdl);

	if (!rx_char_cb[uid])
		return;

	USART_TypeDef *uart_instance = (USART_TypeDef *)hdl;
	uint32_t uart_sr_reg = uart_instance->SR;
	uint32_t err_flags = uart_sr_reg & (uint32_t)(USART_SR_PE | USART_SR_FE
			| USART_SR_ORE | USART_SR_NE);

	/* If any errors, clear it by reading the RX data register. */
	if (err_flags) {
		volatile uint8_t data = uart_instance->DR;
		UNUSED(data);
		return;
	}

	uint8_t data = uart_instance->DR;
	rx_char_cb[uid](data);
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

bool uart_rx(periph_t hdl, uint8_t *data, uint16_t size, uint16_t timeout_ms)
{
	CHECK_HANDLE(hdl, false);

	if (!data)
		return false;
	if (HAL_UART_Receive(&uart_stm32_handle[convert_hdl_to_id(hdl)], data,
				size, timeout_ms) != HAL_OK)
		return false;
	return true;
}
