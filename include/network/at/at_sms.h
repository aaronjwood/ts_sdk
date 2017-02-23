/**
 * \file at_sms.h
 *
 * \brief API that uses AT commands to communicate through SMS
 *
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 *
 */

#ifndef AT_SMS_H
#define AT_SMS_H

#include <stdbool.h>
#include <stdint.h>
#include "smscodec.h"

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
 * 	True  - If the ACK was sent successfully
 * 	False - Failed sending the ACK
 */
bool at_sms_nack(void);

/*
 * Helper function to retrieve the current number associated with the SIM.
 *
 * Parameters:
 * 	num - Pointer to buffer that will contain the NULL terminated string
 * 	      representation of the number associated with the SIM. This buffer
 * 	      is assumed to be at least ADDR_SZ + 1 bytes long.
 *
 * Returns:
 * 	True  - Successfully retrieved the number associated with the SIM
 * 	False - Retrieval failed.
 */
bool at_sms_retrieve_num(char *num);

#endif
