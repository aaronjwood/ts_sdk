/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __HWRNG_H
#define __HWRNG_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Hardware Random Number Generator (RNG) driver.
 * Provide access to a hardware source of 32-bit randome numbers that can
 * be used as an entropy source.
 */

/*
 * Initialize the hardware random number generator.
 *
 * Parameters:
 *	None
 *
 * Returns:
 *	true - Generator was initialized and is ready to provide random numbers.
 *	false - Initialization failed.  Generator is not usable.
 */
bool hwrng_module_init(void);

/*
 * Read a random number from the generator, blocking until the number is 
 * available.
 *
 * Parameters:
 *	pnum - Pointer to a 32 bit variable to receive the number
 *
 * Returns:
 *	true - A number was returned.
 *	false - The generator reported an error.  No number was returned.
 */
bool hwrng_read_random(uint32_t *pnum);

#endif /* __HWRNG_H */
