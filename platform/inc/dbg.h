/**
 * \file dbg.h
 * \copyright Copyright (c) 2016, 2017 Verizon. All rights reserved.
 * \brief Give access to the debug port.
 * \details This component is used to print debug messages through one of the
 * UART ports on the board. To compile without debug messages, define "NO_DEBUG"
 * before including this header.
 */

#ifndef __DBG_H
#define __DBG_H

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

/**
 * \brief Initialize the debug interface.
 * \details This sets up the underlying UART hardware and related GPIO pins to
 * output debug messages to the host machine. Avoid calling this function
 * directly. It is meant to be accessed through the associated macro (\ref
 * dbg_module_init) instead.
 *
 * \retval true Initialization was successful.
 * \retval false Initialization failed.
 */
bool __dbg_module_init(void);

/**
 * \brief Macro to initialize the debug module.
 */
#ifdef NO_DEBUG
#define dbg_module_init()
#else
#define dbg_module_init()	__dbg_module_init()
#endif

/**
 * \brief Macro to print debug messages.
 * \details This macro is defined as an alias to the standard C library
 * function printf.
 * \param[in] args Format string and variable number of arguments
 * \returns Error number if return value is negative. Otherwise, total number of
 * characters written.
 */
#ifdef NO_DEBUG
#define dbg_printf(args...)
#else
#define dbg_printf(args...)	printf(args)
#endif

/**
 * \brief Block execution and optionally toggle an onboard LED.
 */
void raise_err(void);

/**
 * \brief Print the given message, block execution and toggle the onboard LED.
 * \details If NO_DEBUG is defined, the message will not be printed but the LED
 * will be toggled.
 * \param[in] args Format string and variable number of arguments
 */
#define fatal_err(args...) do { dbg_printf(args); raise_err(); } while(0)

/**
 * \brief Assert an expression.
 * \details Use the debug port to print the filename and the line number and
 * toggle the onboard LED if the expression evaluates to "false". If NO_DEBUG
 * is defined, the filename and line number will not be printed but the LED
 * will be toggled.
 *
 * \param[in] x The expression expected to evaluate to "true".
 */
#define ASSERT(x) do { \
	if (!((x))) { \
		dbg_printf("Assertion failed in %s : %d\n", __FILE__, __LINE__); \
		raise_err(); \
	} \
} while(0)

#endif
