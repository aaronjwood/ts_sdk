/**
 * \file uart_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for UART
 * \details This header defines a platform independent API to read and write over
 * the UART port. All sending operations are blocking while data is received
 * byte-by-byte via a receive callback.
 */
#ifndef UART_HAL_H
#define UART_HAL_H

#include "port_pin_api.h"

/**
 * \brief UART receive character callback.
 * \details Every time the UART receives a character, this callback is invoked
 * with the received character as a parameter.
 */
typedef void (*uart_rx_char_cb)(uint8_t ch);

/**
 * \brief Structure that defines the pins to use for the UART peripheral.
 */
struct uart_pins {
	pin_name_t tx;	/**< UART transmit pin */
	pin_name_t rx;	/**< UART receive pin */
	pin_name_t rts;	/**< UART RTS pin */
	pin_name_t cts;	/**< UART CTS pin */
};

/**
 * \brief Type of parity required on the UART peripheral.
 */
typedef enum {
	NONE,	/**< No parity bit will be computed */
	ODD,	/**< Odd parity bit will be computed */
	EVEN	/**< Even parity bit will be computed */
} parity_t;

/**
 * \brief Type defining the UART configuration.
 * \details Acceptable values for the following fields are hardware / implementation
 * dependent.
 */
typedef struct {
	uint32_t baud;		/**< Baud rate in bits per second */
	uint8_t data_width;	/**< Width (in bits) of the unit of data */
	parity_t parity;	/**< Type of parity */
	uint8_t stop_bits;	/**< Number of stop bits */
	uint32_t priority;	/**< Interrupt priority of the UART's IRQ Handler */
} uart_config;

/**
 * \brief Initialize the UART peripheral.
 * \details The UART is configured according to the \ref uart_config structure
 * passed to this routine. \n
 * If either uart_pins.rts or uart_pins.cts is assigned the value 'NC', HW flow
 * control is disabled. Also, only one of uart_pins.tx or uart_pins.rx can be
 * 'NC'. This routine must be called once for every instance before attempting
 * to transmit data or set a receive callback. \n
 * On initialization, IRQs are enabled by default.
 *
 * \param[in] pins Structure that contains names of the pins to use for the UART.
 * \param[in] config Defines the configuration of the UART peripheral.
 *
 * \returns Handle to UART peripheral. \ref NO_PERIPH is returned if the
 * combination of uart_pins turns out to be invalid, pins do not connect to the
 * same peripheral or configuration parameters are invalid.
 */
periph_t uart_init(struct uart_pins pins, uart_config config);

/**
 * \brief Set the UART receive character callback.
 * \details The callback is invoked with the received character as a parameter.
 * \param[in] hdl Handle of the UART peripheral.
 * \param[in] cb Pointer to the callback routine.
 * \pre \ref uart_init must be called to retrieve a valid handle.
 */
void uart_set_rx_char_cb(periph_t hdl, uart_rx_char_cb cb);

/**
 * \brief Send data over the UART.
 * \details This is a blocking send. In case the receiver blocks the flow via
 * hardware flow control, the call will block for at most 'timeout_ms'
 * milliseconds.
 *
 * \param[in] hdl Handle of the UART peripheral to write.
 * \param[in] data Pointer to the data to be sent.
 * \param[in] size Number of bytes to send
 * \param[in] timeout_ms Total number of milliseconds to wait before giving up
 * in case of a busy channel.
 *
 * \retval true Data was sent out successfully.
 * \retval false Send aborted due to timeout or null pointer was provided for
 * the data.
 *
 * \pre \ref uart_init must be called to retrieve a valid handle.
 */
bool uart_tx(periph_t hdl, uint8_t *data, uint16_t size, uint16_t timeout_ms);

/**
 * \brief Call the IRQ handler of the UART peripheral instance.
 * \details In most cases each peripheral has its own interrupt vector. For
 * this scenario, the handler is invoked from within the driver implicitly and
 * the user need not bother with this routine.
 * However sometimes two peripherals may share the interrupt vector. In such
 * cases, this function can be used to explicitly call the IRQ handler from
 * outside the driver.
 *
 * \param[in] hdl Handle of the UART peripheral.
 * \pre \ref uart_init must be called to retrieve a valid handle.
 */
void uart_irq_handler(periph_t hdl);

/**
 * \brief Enable / disable the UART interrupt.
 * \details Call this function to enable or disable the interrupt service
 * handler of a specific UART instance.
 *
 * \param[in] hdl Handle to the UART instance
 * \param[in] state Boolean value that determines if the UART's interrupt should
 * be disabled. Set to 'true' to enable the IRQ, 'false' to disable it.
 */
void uart_toggle_irq(bool state);

#endif
