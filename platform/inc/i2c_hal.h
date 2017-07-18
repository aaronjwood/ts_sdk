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
 * \brief Defines an I2C address.
 * \details This type stores the slave's 7-bit address along with the address of
 * the register inside the slave.
 */
typedef struct {
	uint8_t slave;	/**< 7-bit slave address */
	uint8_t reg;	/**< 8-bit register address */
} i2c_addr_t;

/**
 * \brief Initialize the I2C peripheral and associated pins.
 * \details Initializes the pins \b scl and \b sda to function as the clock and
 * data pins, respectively, of the I2C peripheral they are connected to. This
 * routine must be called once before using the I2C peripheral.
 *
 * \param[in] scl I2C peripheral's clock pin
 * \param[in] sda I2C peripheral's data pin
 * \param[in] I2C timeout duration in milli seconds
 *
 * \returns Handle to I2C peripheral. If \b scl and \b sda cannot be configured,
 * \ref NO_PERIPH is returned. Possible causes can be:
 * \arg At least one of the pins is already being used.
 * \arg Pins do not connect to the same I2C peripheral.
 */
periph_t i2c_init(pin_name_t scl, pin_name_t sda, uint32_t timeout_ms);

/**
 * \brief Read bytes from the I2C peripheral.
 * \details This routine performs a blocking read on the I2C peripheral
 * referenced by the handle. It can read a maximum of 255 bytes of data from the
 * slave on the I2C bus. Each byte on this bus is 8-bits wide.
 *
 * \param[in] hdl Handle of the I2C peripheral to be read.
 * \param[in] addr Destination address (\ref i2c_addr_t) on the I2C bus.
 * \param[in] len Length of the data to read into the buffer.
 * \param[out] buf Pointer to the buffer that stores the data read in.
 *
 * \retval true Data was successfully read from the I2C bus
 * \retval false Failed to read data into the buffer
 *
 * \pre \ref i2c_init must be called to retrieve a valid handle.
 */
bool i2c_read(periph_t hdl, i2c_addr_t addr, uint8_t len, uint8_t *buf);

/**
 * \brief Write bytes to the I2C peripheral.
 * \details This routine performs a blocking write on the I2C peripheral
 * referenced by the handle. It can write a maximum of 255 bytes of data to the
 * slave on the I2C bus. Each byte on this bus is 8-bits wide.
 *
 * \param[in] hdl Handle of the I2C peripheral to be written to.
 * \param[in] addr Destination address (\ref i2c_addr_t) on the I2C bus.
 * \param[in] len Length of the data contained in the write buffer.
 * \param[in] buf Pointer to the buffer that stores the data to be written.
 *
 * \retval true Data was successfully written to the I2C bus
 * \retval false Failed to write data onto the bus
 *
 * \pre \ref i2c_init must be called to retrieve a valid handle.
 */
bool i2c_write(periph_t hdl, i2c_addr_t addr, uint8_t len, const uint8_t *buf);

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
