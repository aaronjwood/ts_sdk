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
 * \brief Initialize the UART peripheral.
 * \details The UART is initialized with the following settings: \n
 * Baud rate       : \b 115200 \b bps \n
 * Data width      : \b 8 \b bits \n
 * Parity bits     : \b None \n
 * Stop bits       : \b 1 \n
 * HW flow control : \b Configurable \n
 *
 * If either uart_pins.rts or uart_pins.cts is assigned the value 'NC', HW flow
 * control is disabled. Also, only one of uart_pins.tx or uart_pins.rx can be
 * 'NC'. This routine must be called once for every instance before attempting
 * to transmit data or set a receive callback.
 *
 * \param[in] pins Structure that contains names of the pins to use for the UART.
 * \param[in] priority Interrupt priority of this UART instance. The underlying
 * representation of the priority is determined by the implementation.
 *
 * \returns Handle to UART peripheral. If the combination of uart_pins turns
 * out to be invalid or pins do not connect to the same peripheral,
 * \ref NO_PERIPH is returned.
 */
periph_t uart_init(struct uart_pins pins, uint32_t priority);

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

#endif
