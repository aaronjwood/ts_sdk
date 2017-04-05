/**
 * \file pin_map.h
 * \brief Define tables that map pins to peripherals
 * \details These structures contain all possible pin mappings for a given MCU &
 * board combination. Each entry is of type \ref pin_t and has all the information
 * needed to connect and initialize a specific pin to work with an MCU peripheral.
 * For example, uart_tx_map maps all pins that are capable of transmitting serial
 * data with their respective UART / USART hardware peripherals.
 */
#ifndef PIN_MAP_H
#define PIN_MAP_H

#include "pin_defs.h"

/**
 * \brief UART TX pin mappings
 */
extern const pin_t uart_tx_map[];

/**
 * \brief UART RX pin mappings
 */
extern const pin_t uart_rx_map[];

/**
 * \brief UART RTS pin mappings
 */
extern const pin_t uart_rts_map[];

/**
 * \brief UART CTS pin mappings
 */
extern const pin_t uart_cts_map[];

/**
 * \brief I2C SDA pin mappings
 */
extern const pin_t i2c_sda_map[];

/**
 * \brief I2C SCL pin mappings
 */
extern const pin_t i2c_scl_map[];

#endif
