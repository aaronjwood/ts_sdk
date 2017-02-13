/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef __SMSCODEC_H
#define __SMSCODEC_H

#include <stdint.h>
#include <stdbool.h>
#include "smsdef.h"

/*
 * Encode a single segment of the message to an outgoing SMS PDU.
 *
 * Parameters:
 * 	msg_to_send - A struct that contains the segment of the SMS to be encoded
 * 	              along with necessary metadata.
 *
 * 	pdu         - A NULL terminated character buffer containing the encoded
 * 	              PDU. The maximum size of this buffer is MAX_PDU_SZ bytes,
 * 	              not accounting for the NULL character.
 *
 * Returns:
 * 	0 if the encoding operation failed. On success, a positive number
 * 	representing the size of the encoded PDU as a hex string in bytes
 * 	excluding the NULL termination character.
 */
uint16_t smscodec_encode(const sms_t *msg_to_send, char *pdu);

/*
 * Decode the incoming PDU string and populate the message structure accordingly.
 *
 * Parameters:
 * 	len      - Length of the PDU hex string in bytes
 * 	pdu      - PDU hex string
 * 	recv_msg - A struct that holds the received segment along with metadata
 * 	           needed to reconstruct the entire message.
 *
 * Returns:
 * 	True  - If the decoding operation succeeded
 * 	False - If the decoding operation failed
 */
bool smscodec_decode(uint8_t len, const char *pdu, sms_t *recv_msg);

#endif
