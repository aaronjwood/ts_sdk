/* Copyright (C) 2017 Verizon. All rights reserved. */
#include "smscodec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HEXLEN			2	/* 2 hexadecimal characters */
#define ADDR_HDR_SZ		4	/* Size of the header for the encoded address */

/* On failure, return _val */
#define ON_FAIL(_x, _val) \
	do { \
		if (!(_x)) \
			return (_val); \
	} while(0)

enum {
	MTI_SMS_SUBMIT = 0x01,

	RD_ACCEPT = 0x00,
	RD_REJECT = 0x04,

	VPF_ABSENT = 0x00,
	VPF_RELATIVE = 0x08,
	VPF_ENHANCED = 0x10,
	VPF_ABSOLUTE = 0x18,

	SRR_REPORT_REQ = 0x20,
	SRR_REPORT_NOT_REQ = 0x00,

	UDHI_PRESENT = 0x40,
	UDHI_ABSENT = 0x00,

	RP_SET = 0x80,
	RP_NOT_SET = 0x00,

	PID_VAL = 0x00,			/* SME-to-SME protocol */
	DCS_VAL = 0x04,			/* No message class, 8-bit encoding */

	UDH_LEN = 0x05,			/* Length of total user data header */
	IEI_CONCAT = 0x00,		/* Concatenation IEI */
	IEI_CONCAT_LEN = 0x03		/* Length of concatenation IEI field */
};

/*
 * 'intl_num' is expected to be an international number following the ISDN /
 * telephone numbering plan. Both buffers are NULL terminated. Size of intl_num
 * must be at most ADDR_SZ + 1 bytes and that of enc_intl_num must be at most
 * ENC_ADDR_SZ + 1 bytes. Returns the length of the encoded output.
 */
#define STRIDE			2
static bool smscodec_encode_addr(const char *intl_num, char **enc_intl_num)
{
	uint8_t n_idx = 0;
	uint8_t e_idx = ADDR_HDR_SZ;
	uint8_t num_len = strlen(intl_num);
	uint8_t enc_len = num_len;

	/* Ignore the leading '+' if any */
	if (intl_num[0] == '+') {
		n_idx++;
		enc_len--;
	}

	/* The header for the encoded output is its length */
	if (snprintf(*enc_intl_num, ADDR_HDR_SZ + 1, "%02X91", enc_len) < 0)
		return false;

	/*
	 * Take digits two at a time, swap them and then copy the result into the
	 * destination buffer. For an odd number of digits, assume an 'F' after
	 * the last digit.
	 */
	for (; n_idx < num_len; n_idx += STRIDE, e_idx += STRIDE) {
		if (intl_num[n_idx + 1] == '\0')
			(*enc_intl_num)[e_idx] = 'F';
		else
			(*enc_intl_num)[e_idx] = intl_num[n_idx + 1];
		(*enc_intl_num)[e_idx + 1] = intl_num[n_idx];
	}

	*enc_intl_num += e_idx;
	return true;
}

/*
 * Convert the byte into a string representing its hex form and write it to the
 * destination buffer.
 * Return 'true' on success and 'false' on failure.
 */
static bool hexstr(uint8_t data, char **dest)
{
	int written = snprintf(*dest, HEXLEN + 1, "%02X", data);
	if (written <= 0)
		return false;
	*dest += written;
	return true;
}

/*
 * Encode the UDL (and UDH if present) along with the user data. Returns 'true'
 * on success, 'false' on any error.
 */
static bool encode_ud(const msg_t *msg_to_send, char **dest)
{
	if (msg_to_send->num_seg > 1) {
		ON_FAIL(hexstr(1 + UDH_LEN + msg_to_send->len, dest), false);
		ON_FAIL(hexstr(UDH_LEN, dest), false);
		ON_FAIL(hexstr(IEI_CONCAT, dest), false);
		ON_FAIL(hexstr(IEI_CONCAT_LEN, dest), false);
		ON_FAIL(hexstr(msg_to_send->concat_ref_no, dest), false);
		ON_FAIL(hexstr(msg_to_send->num_seg, dest), false);
		ON_FAIL(hexstr(msg_to_send->seq_no, dest), false);
	} else if (msg_to_send->num_seg == 1) {
		ON_FAIL(hexstr(msg_to_send->len, dest), false);
	}

	for (uint8_t i = 0; i < msg_to_send->len; i++)
		ON_FAIL(hexstr(*(msg_to_send->data + i), dest), false);
	return true;
}

int smscodec_encode(const msg_t *msg_to_send, char *pdu)
{
	static uint8_t msg_ref_no = 0;

	if (msg_to_send == NULL || msg_to_send->data == NULL ||
			msg_to_send->addr[0] == '\0' || pdu == NULL)
		return -1;

	if (msg_to_send->num_seg == 0)
		return -1;

	if (msg_to_send->len == 0)
		return -1;

	if (msg_to_send->seq_no > msg_to_send->num_seg)
		return -1;

	if (msg_to_send->num_seg > 1 && msg_to_send->seq_no == 0)
		return -1;

	char *wptr = pdu;
	uint8_t val = 0;

	/* Write the first octet and message reference number into the PDU */
	val = MTI_SMS_SUBMIT | RD_ACCEPT | VPF_ABSENT | SRR_REPORT_NOT_REQ |
		RP_NOT_SET;
	if (msg_to_send->num_seg > 0)
		val |= UDHI_PRESENT;
	ON_FAIL(hexstr(val, &wptr), -1);
	ON_FAIL(hexstr(msg_ref_no, &wptr), -1);
	msg_ref_no++;

	/* Write the destination address length and the encoded address */
	ON_FAIL(smscodec_encode_addr(msg_to_send->addr, &wptr), -1);

	/* Write the PID and DCS fields */
	ON_FAIL(hexstr(PID_VAL, &wptr), -1);
	ON_FAIL(hexstr(DCS_VAL, &wptr), -1);

	/* Write the User Data field */
	ON_FAIL(encode_ud(msg_to_send, &wptr), -1);

	return (wptr - pdu);
}
