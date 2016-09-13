/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __DBG_H
#define __DBG_H

#include <stdio.h>
#include <stdbool.h>

/*
 * Debug module driver.
 * This core library component is used to print debug messages through one of
 * the UART ports on the board.
 * To compile without debug messages, define "NO_DEBUG" before including this
 * header.
 */

/*
 * Initialize the debug interface. This sets up the underlying UART hardware
 * and related GPIO pins to output debug messages to the host machine.
 * Avoid calling this function directly. It is meant to be accessed through the
 * associated macro instead.
 *
 * Returns:
 * 	True - when initialization was successful.
 * 	False - when initialization failed.
 *
 * Paramters:
 * 	None
 */
bool __dbg_module_init(void);

#ifdef NO_DEBUG
#define dbg_module_init()
#else
#define dbg_module_init()	__dbg_module_init()
#endif

/*
 * Print debug messages. This function is defined as an alias to the standard
 * C library function printf.
 *
 * Returns:
 * 	Error number if return value is negative. Otherwise, total number of
 * 	characters written.
 *
 * Parameters:
 * 	Format string and variable number of arguments
 */
#ifdef NO_DEBUG
#define dbg_printf(args...)
#else
#define dbg_printf(args...)	printf(args)
#endif

/*
 * Block execution and toggle the onboard LED - LD2 (Blue).
 *
 * Returns:
 * 	None
 *
 * Parameters:
 * 	None
 */
void raise_err(void);

/*
 * Print the given message, block execution and toggle the onboard LED.
 * if NO_DEBUG is defined, the message will not be printed but the LED will be
 * toggled.
 *
 * Returns:
 * 	None
 *
 * Parameters:
 * 	Format string and variable number of arguments
 */
#define fatal_err(args...) do { dbg_printf(args); raise_err(); } while(0)

/*
 * Assert an expression. Use the debug port to print the filename and the line
 * number and toggle the onboard LED if the expression evaluates to "false".
 * If NO_DEBUG is defined, the filename and line number will not be printed but
 * the LED will be toggled.
 *
 * Returns:
 * 	None
 *
 * Parameters:
 * 	x - The expression expected to evaluate to "true".
 */
#define ASSERT(x) do { \
	if (!((x))) { \
		dbg_printf("Assertion failed in %s : %d\n", __FILE__, __LINE__); \
		raise_err(); \
	} \
} while(0)

#endif
