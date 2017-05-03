/**
 * \file hwrng_hal.h
 * \copyright Copyright (c) 2016, 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for a hardware RNG.
 * \details Provide access to a hardware source of 32-bit random numbers that
 * can be used as an entropy source.
 */

#ifndef __HWRNG_HAL_H
#define __HWRNG_HAL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Initialize the hardware random number generator.
 * \retval true Generator was initialized and is ready to provide random numbers.
 * \retval false Initialization failed. Generator is not usable.
 */
bool hwrng_module_init(void);

/**
 * \brief Read a random number from the generator, blocking until the number is
 * available.
 * \param[out] pnum Pointer to a 32 bit variable to receive the number.
 * \retval true A number was returned.
 * \retval false The generator reported an error. No number was returned.
 * \pre \ref hwrng_module_init must be called before retrieving a random number.
 */
bool hwrng_read_random(uint32_t *pnum);

#endif /* __HWRNG_HAL_H */
