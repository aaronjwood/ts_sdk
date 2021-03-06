/* Copyright (C) 2017 Verizon. All rights reserved. */

/*
 * This API is used to send and receive SMS messages via the modem's AT interface.
 */

#ifndef AT_SMS_H
#define AT_SMS_H

#include <stdbool.h>
#include <stdint.h>
#include "smscodec.h"

/* Max address length in bytes for the remote host */
#define MAX_HOST_LEN		ADDR_SZ

/* Max SMS size in bytes without user data header (TP-UDH) */
#define MAX_SMS_PL_SZ		MAX_DATA_SZ

/* Max concatenated sms size in bytes
 */
#define CONCT_SMS_SZ            MAX_CONCAT_DATA_SZ

/* Upper limit for the concatenated sms reference number */
#define MAX_SMS_REF_NUMBER		256

/* Type describing the SMS segment */
typedef sms_t at_msg_t;

/* Callback that will be invoked when a URC is received */
typedef void (*at_sms_cb)(const at_msg_t *sms_seg);

/*
 * Initialize the AT interface for sending and receiving SMS
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  - If initialization was a success
 * 	False - If initialization failed
 */
bool at_init(void);

/*
 * Send an SMS segment.
 *
 * Parameters:
 * 	sms_seg - Pointer to the struct representing an SMS segment
 *
 * Returns:
 * 	True  - If message was sent successfully and an ACK was received
 * 	False - Sending the message failed
 */
bool at_sms_send(const at_msg_t *sms_seg);

/*
 * Set the receive callback that will be invoked when a SMS segment is received
 * in a URC.
 *
 * Parameters:
 * 	cb - Pointer to callback function
 *
 * Returns:
 * 	None
 */
void at_sms_set_rcv_cb(at_sms_cb cb);

/*
 * Send a RP-ACK acknowledging the message segment most recently received.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  - If the ACK was sent successfully
 * 	False - Failed sending the ACK
 */
bool at_sms_ack(void);

/*
 * Send a RP-ERROR, letting the sender the know the message was not stored in
 * the receiver.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	True  - If the NACK was sent successfully
 * 	False - Failed sending the NACK
 */
bool at_sms_nack(void);
#endif
