/**
 * \file i2c_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for I2C peripherals
 * \details This header defines a platform independent API to access I2C
 * peripherals on the target. All transactions of the I2C bus are assumed to be
 * blocking and the unit of transaction is an 8-bit byte. All slave addresses
 * are assumed to be 7-bits wide.
 */
#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>
#include "port_pin_api.h"

/**
 * \brief Initialize the I2C peripheral and associated pins.
 * \details Initializes the pins \b scl and \b sda to function as the clock and
 * data pins, respectively, of the I2C peripheral they are connected to. This
 * routine must be called once before using the I2C peripheral.
 *
 * \param[in] scl I2C peripheral's clock pin
 * \param[in] sda I2C peripheral's data pin
 *
 * \returns Handle to I2C peripheral. If \b scl and \b sda cannot be configured,
 * \b NC is returned. Possible causes can be:
 * 	\arg At least one of the pins is already being used.
 * 	\arg Pins do not connect to the same I2C peripheral.
 */
periph_t i2c_init(pin_name_t scl, pin_name_t sda);

/**
 * \brief Read bytes from the I2C peripheral.
 * \details This routine performs a blocking read on the I2C peripheral
 * referenced by the handle. It can read a maximum of 255 bytes of data from the
 * slave on the I2C bus. Each byte on this bus is 8-bits wide.
 *
 * \param[in] hdl Handle of the I2C peripheral to be read.
 * \param[in] slave 7-bit slave address on the I2C bus.
 * \param[in] reg 8-bit register address inside the slave.
 * \param[in] len Length of the data read into the buffer.
 * \param[out] buf Pointer to the buffer that stores the data read in.
 *
 * \retval true Data was successfully read from the I2C bus
 * \retval false Failed to read data into the buffer
 *
 * \pre \ref i2c_init must be called to retrieve a valid handle.
 */
bool i2c_read(periph_t hdl, uint8_t slave, uint8_t reg, uint8_t len, uint8_t *buf);

/**
 * \brief Write bytes to the I2C peripheral.
 * \details This routine performs a blocking write on the I2C peripheral
 * referenced by the handle. It can write a maximum of 255 bytes of data to the
 * slave on the I2C bus. Each byte on this bus is 8-bits wide.
 *
 * \param[in] hdl Handle of the I2C peripheral to be written to.
 * \param[in] slave 7-bit slave address on the I2C bus.
 * \param[in] reg 8-bit register address inside the slave.
 * \param[in] len Length of the data contained in the write buffer.
 * \param[in] buf Pointer to the buffer that stores the data to be written.
 *
 * \retval true Data was successfully written to the I2C bus
 * \retval false Failed to write data onto the bus
 *
 * \pre \ref i2c_init must be called to retrieve a valid handle.
 */
bool i2c_write(periph_t hdl, uint8_t slave, uint8_t reg, uint8_t len, uint8_t *buf);

/**
 * \brief Turn on or turn off the power to the I2C peripheral.
 *
 * \param[in] hdl Handle of the I2C peripheral.
 * \param[in] state \arg Set to \b true to turn on the power
 *                  \arg Set to \b false to turn off the power
 *
 * \pre \ref i2c_init must be called to retrieve a valid handle.
 */
void i2c_pwr(periph_t hdl, bool state);
#endif
