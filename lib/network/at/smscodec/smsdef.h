/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef __SMSDEF_H
#define __SMSDEF_H

/*
 * Sizes here are in bytes taken up in the final PDU hex string. This means '4A'
 * occupies two bytes.
 */
#define ADDR_SZ				16 /* 15 digit number and '+' */
#define ENC_ADDR_SZ			19 /* 15 + 2 (len) + 2 (number type) */
#define PDU_OVR_HD			(10 + ENC_ADDR_SZ)
#define UD_SZ				(140*2)	/* Valid only for 8-bit encoding */
#define MAX_PDU_SZ			(PDU_OVR_HD + UD_SZ)

/* Maximum size of the buffer in bytes */
#define MAX_BUF_SZ			140

/*
 * This structure encodes a single segment of the SMS.
 * In case of a single SMS, num_seg is 1 and ref_no and seq_no are "don't care"
 * values.
 * Segments belonging to a concatenated SMS must have the same ref_no. num_seg
 * must take a value of 2 or greater and seq_no must not be greater than num_seg
 * and must be greater than 0.
 * The address is an international number following the ISDN / telephone
 * numbering plan (see Section 9.1.2.5 of 3GPP TS 23.040). Its maximum size is
 * ADDR_SZ bytes.
 * Maximum size of 'buf' is MAX_BUF_SZ.
 */
typedef struct sms_t {
	uint8_t len;			/* Length of the buffer content */
	uint8_t *buf;			/* Pointer to the buffer containing the payload */
	uint8_t ref_no;			/* Concatenated message reference number */
	uint8_t num_seg;		/* Number of segments making the message */
	uint8_t seq_no;			/* Seq No. of the current segment */
	char addr[ADDR_SZ + 1];		/* Null terminated address */
} sms_t;

#endif
