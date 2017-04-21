/**
 * \file at_core.h
 *
 * \brief Functions that aid in communicating with a generic modem's AT interface
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 */

#ifndef AT_CORE_H
#define AT_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "uart_util.h"

typedef enum at_return_codes {
        AT_SUCCESS = 0,
        AT_RSP_TIMEOUT,
        AT_FAILURE,
        AT_TX_FAILURE,
        AT_WRONG_RSP,
        AT_RECHECK_MODEM
} at_ret_code;

#ifdef MODEM_TOBY201
/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define AT_COMM_DELAY_MS	20
#define CHECK_MODEM_DELAY	5000	/* in mili seconds, polling for modem */
#endif

#define MAX_RSP_LINE		2	/* Some command send response plus OK */
#define AT_CORE_INV_PARAM	UART_INV_PARAM	/* Invalid parameter. */

/* Enable to debug wrong response, this prints expected vs received buffer in
 * raw format
 */
/*#define DEBUG_WRONG_RSP*/

/* Enable this macro to display messages, error will alway be reported if this
 * macro is enabled while V2 and V1 will depend on debug_level setting
 */
/*#define DEBUG_AT_LIB*/

/* level v2 is normally for extensive debugging need, for example tracing
 * function calls
 */
#ifdef DEBUG_AT_LIB
static int __attribute__((unused)) debug_level;
#define DEBUG_V2(...)	\
                        do { \
                                if (debug_level >= 2) \
                                        printf(__VA_ARGS__); \
                        } while (0)

/* V1 is normaly used for variables, states which are internal to functions */
#define DEBUG_V1(...) \
                        do { \
                                if (debug_level >= 1) \
                                        printf(__VA_ARGS__); \
                        } while (0)

#define DEBUG_V0(...)	printf(__VA_ARGS__)
#else
#define DEBUG_V0(...)
#define DEBUG_V1(...)
#define DEBUG_V2(...)

#endif

/*#define DBG_STATE*/

#ifdef DBG_STATE
#define DEBUG_STATE(...) printf("%s: line %d, state: %u\n",\
                                __func__, __LINE__, (uint32_t)state)
#else
#define DEBUG_STATE(...)
#endif

#define CHECK_NULL(x, y) do { \
                                if (!((x))) { \
                                        printf("%s: Fail at line: %d\n", \
                                                __func__, __LINE__); \
                                        return ((y)); \
                                } \
                         } while (0)

#define CHECK_SUCCESS(x, y, z)	\
                        do { \
                                if ((x) != (y)) { \
                                        printf("%s: Fail at line: %d\n", \
                                                __func__, __LINE__); \
                                        return (z); \
                                } \
                        } while (0)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

/** response descriptor */
typedef struct _at_rsp_desc {
        /** Hard coded expected response for the given command */
        const char *rsp;
        /** Response handler if any */
        void (*rsp_handler)(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);
        /** private data */
        void *data;
} at_rsp_desc;

/** Command description */
typedef struct _at_command_desc {
        /** skeleton for the command */
        const char *comm_sketch;
        /** final command after processing command skeleton */
        char *comm;
        /** command timeout, indicates bound time to receive response */
        uint32_t comm_timeout;
        at_rsp_desc rsp_desc[MAX_RSP_LINE];
        /** Expected command error */
        const char *err;
} at_command_desc;

/*
 * Serial data receive callback.
 *
 * Parameters:
 *
 * Returns:
 *	None
 */
typedef void (*at_rx_callback)(void);

/*
 * URC handler.
 *
 * Parameters:
 * 	urc - Pointer to the buffer containing the URC
 *
 * Returns:
 * 	None
 */
typedef void (*at_urc_callback)(const char *urc);

/*
 * Initialize the AT core module
 *
 * Parameters:
 * 	rx_cb  - A pointer to the serial data receive callback
 * 	urc_cb - A pointer to the URC handler
 *
 * Returns:
 * 	True  - The AT core module was successfully initialized
 * 	False - There was an error initializing the module
 */
bool at_core_init(at_rx_callback rx_cb, at_urc_callback urc_cb);

/*
 * Reset the modem.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	AT_RSP_TIMEOUT - Timed out trying to reset modem
 * 	AT_TX_FAILURE  - Could not establish a link over the UART
 * 	AT_FAILURE     - Failed to restart the modem
 * 	AT_SUCCESS     - Successfully reset the modem
 * 	AT_WRONG_RSP   - Received a wrong response from the modem during reset
 */
at_ret_code at_core_modem_reset(void);

/*
 * Write a set of bytes into the UART connecting the MCU and the modem.
 *
 * Parameters:
 * 	buf - Pointer to buffer containing the data to be written
 * 	len - Length of the data contained in the buffer
 *
 * Returns:
 * 	True  - If the data was sent out successfully
 * 	False - If there was an error in sending the data
 */
bool at_core_write(uint8_t *buf, uint16_t len);

/*
 * Retrieve a set of bytes from the UART's read buffer. The maximum number of
 * bytes that can be retrieved is given by the return value of 'at_core_rx_available'.
 *
 * Parameters:
 * 	buf - Pointer to the buffer where the data will be read into.
 * 	sz  - Maximum size the supplied buffer can store.
 *
 * Returns:
 * 	Number of bytes actually read into the buffer.
 * 	AT_CORE_INV_PARAM if null pointer is provided for buffer.
 */
int at_core_read(uint8_t *buf, buf_sz sz);

/*
 * Utility function to scan the internal UART ring buffer to find the pattern or
 * substring.
 *
 * Parameters:
 * 	start_idx - starting index to being scan for the pattern, -1 is special
 *		    value where if supplied, it starts with begining of the ring
 *		    buffer
 * 	pattern   - Null terminated string.
 *	nlen      - length of the pattern to be matched
 *
 * Returns:
 * 	-1 if no such string is found inside the read buffer or parameters are
 *	wrong
 * 	On success, returns the starting position within the ring buffer of
 *	matched pattern
 */
int at_core_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen);

/*
 * Return the number of unread bytes present in the buffer.
 *
 * Paramters:
 * 	None
 *
 * Returns:
 * 	Number of unread bytes in the buffer.
 */
buf_sz at_core_rx_available(void);

/*
 * Clear the receive buffer associated with the UART link between the MCU and
 * the modem.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	None
 */
void at_core_clear_rx(void);

/*
 * Write an AT command over the UART link to the modem and receive a response.
 * This routine is not valid for commands that return a prompt and expect
 * additional input.
 *
 * Parameters:
 * 	desc      - Command to be sent
 * 	read_line - Expect the response to be contained in a single line
 * 	            delimited by <CR><LF> before and after the line.
 *
 * Returns:
 * 	AT_SUCCESS     - Executed the command and received the expected response
 * 	AT_WRONG_RSP   - Executed the command and received a wrong response
 * 	AT_RSP_TIMEOUT - Timed out waiting for the response
 * 	AT_FAILURE     - Failed to execute the command
 */
at_ret_code at_core_wcmd(const at_command_desc *desc, bool read_line);

/*
 * Process any pending URCs.
 *
 * Parameters:
 * 	True  - If the function is being called outside the interrupt and UART
 * 	        callback context.
 * 	False - If the function is being called from within an interrupt or UART
 * 	        callback context.
 *
 * Returns:
 * 	None
 */
void at_core_process_urc(bool mode);

/*
 * Check if this module is processing a URC outside the interrupt / callback
 * context.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	When the core AT module is processing a URC outside the interrupt /
 * 	callback context, return true. Otherwise, false.
 */
bool at_core_is_proc_urc(void);

/*
 * Check if this module is processing a response.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  - The core AT module is processing a response
 * 	False - The core AT module is not processing a response
 */
bool at_core_is_proc_rsp(void);

/*
 * Check if the modem is registered to the network.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  - The modem is registered to the network
 * 	False - The modem is not correctly registered to the network
 */
bool at_core_query_netw_reg(void);
#endif /* at_core.h */
