/* Copyright (C) 2017 Verizon. All rights reserved. */
#ifndef __SMSCODEC_H
#define __SMSCODEC_H

#include <stdint.h>
#include <stdbool.h>

/*
 * All sizes are in bytes taken up in the final PDU. This means '4A' occupies
 * two bytes
 */
#define ADDR_SZ				12
#define ENC_ADDR_SZ			14
#define PDU_OVR_HD			(12 + ENC_ADDR_SZ)
#define UD_SZ				(140*2)	/* Valid only for 8-bit encoding */
#define MAX_PDU_SZ			(PDU_OVR_HD + UD_SZ)

/*
 * This structure encodes a single segment of the SMS.
 * In case of a single SMS, num_seg is 1 and concat_ref_no and seq_no are
 * "don't care" values.
 * Segments belonging to a concatenated SMS must have the same concat_ref_no.
 * num_seg must take a value of 2 or greater and seq_no must not be greater than
 * num_seg and must be greater than 0.
 * The address is an international number following the ISDN / telephone
 * numbering plan (see Section 9.1.2.5 of 3GPP TS 23.040). Its maximum size is
 * ADDR_SZ bytes.
 */
typedef struct msg_t {
	uint8_t len;			/* Length of the data payload */
	uint8_t *data;			/* Pointer to the data payload */
	uint8_t concat_ref_no;		/* Concatenated message reference number */
	uint8_t num_seg;		/* Number of segments making the message */
	uint8_t seq_no;			/* Seq No. of the current segment */
	char addr[ADDR_SZ + 1];		/* Null terminated address */
} msg_t;

/*
 * Encode a single segment of the message to an outgoing SMS PDU.
 *
 * Parameters:
 * 	msg_to_send - A struct that contains the segment of the SMS to be encoded
 * 	              along with necessary metadata.
 *
 * 	pdu         - A character buffer containing the encoded PDU. The maximum
 * 	              size of this buffer is MAX_PDU_SZ bytes.
 *
 * Returns:
 * 	0 if the encoding operation failed. On success, a positive number
 * 	representing the size of the encoded PDU as a hex string in bytes.
 */
int smscodec_encode(const msg_t *msg_to_send, char *pdu);

/*
 * Decode the incoming PDU string and populate the message structure accordingly.
 *
 * Parameters:
 * 	len      - Length of the PDU string in bytes
 * 	pdu      - PDU string
 * 	recv_msg - A struct that holds the received segment along with metadata
 * 	           needed to reconstruct the entire message.
 *
 * Returns:
 * 	0 if the decoding operation failed.
 * 	1 if the decoding operation succeeded.
 */
bool smscodec_decode(uint8_t len, const char *pdu, msg_t *recv_msg);

#endif
