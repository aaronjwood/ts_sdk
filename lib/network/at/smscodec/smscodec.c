/* Copyright (C) 2017 Verizon. All rights reserved. */

#include "smscodec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HEXLEN			2	/* 2 hexadecimal characters = 1 Byte */
#define ADDR_HDR_SZ		4	/* Size of the header for the encoded address */

/* On failure, return _val */
#define ON_FAIL(_x, _val) \
	do { \
		if (!(_x)) \
			return (_val); \
	} while(0)

enum encoding {
	MTI_SMS_SUBMIT_TYPE = 0x01,	/* SMS-SUBMIT message type */

	RD_ACCEPT = 0x00,		/* Accept messages with duplicate ref no */
	RD_REJECT = 0x04,		/* Reject messages with duplicate ref no */

	VPF_ABSENT = 0x00,		/* Validity period absent from message */
	VPF_RELATIVE = 0x08,		/* Validity period is in relative format */
	VPF_ENHANCED = 0x10,		/* Validity period uses enhanced format */
	VPF_ABSOLUTE = 0x18,		/* Validity period uses absolute format */

	SRR_REPORT_REQ = 0x20,		/* Status report requested */
	SRR_REPORT_NOT_REQ = 0x00,	/* No status report requested */

	UDHI_PRESENT = 0x40,		/* Payload has a header preceeding data */
	UDHI_ABSENT = 0x00,		/* Payload is entirely user data */

	RP_SET = 0x80,			/* A reply path was set */
	RP_NOT_SET = 0x00,		/* No reply path was set */

	PID_VAL = 0x00,			/* SME-to-SME protocol */
	DCS_VAL = 0x04,			/* No message class, 8-bit encoding */

	UDH_LEN = 0x05,			/* Length of total user data header */
	IEI_CONCAT = 0x00,		/* Concatenation IEI */
	IEI_CONCAT_LEN = 0x03		/* Length of concatenation IEI field */
};

enum decoding {
	SMS_P2P_TYPE = 0x00,		/* Message type is SMS point to point */

	TELESVC_IDENT = 0x00,		/* Teleservice parameter identifier */
	TELESVC_LEN = 0x02,		/* Teleservice parameter length */
	TELESVC_WMT = 4098,		/* Wireless messaging teleservice */

	ORIG_ADDR = 0x02,		/* Originating address identifier */

	BEARER_DATA = 0x08,		/* Bearer data identifier */
};

static bool udh_present;		/* Set if UDH is present in received SMS */

/*
 * intl_num is expected to be an international number following the ISDN /
 * telephone numbering plan. Both buffers are NULL terminated.
 * Size of intl_num is expected to be at most ADDR_SZ + 1 bytes. Size of
 * (*enc_intl_num) is expected to be at most ENC_ADDR_SZ + 1 bytes.
 * Returns the 'true' on success and 'false' on failure.
 * On a successful call, (*enc_intl_num) is updated to point to the memory
 * location right after the last byte of the encoded output.
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
 * On a successful call, (*dest) is updated to point to the memory location right
 * after the last byte written.
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
 * Convert a hex string of a given length to an unsigned 32-bit integer. Length
 * cannot exceed 8 characters. The result is stored in num.
 * Return 'true' on success and 'false' on failure.
 * On a successful call, (*src) is updated to point to the memory location right
 * after the last byte read.
 */
static bool hexnum(const char **src, uint8_t len, uint32_t *num)
{
	if (len > 8)
		return false;
	*num = 0;
	for (uint8_t i = 0; i < len; i++) {
		uint8_t digit = (*src)[i];
		*num *= 16;
		if (digit >= '0' && digit <= '9')
			*num += (digit - '0');
		else if (digit >= 'A' && digit <= 'F')
			*num += (digit - 'A');
		else if (digit >= 'a' && digit <= 'f')
			*num += (digit - 'a');
		else
			return 0;
	}
	*src += len;
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
		ON_FAIL(hexstr(msg_to_send->buf[i], dest), false);
	return true;
}

uint16_t smscodec_encode(const msg_t *msg_to_send, char *pdu)
{
	static uint8_t msg_ref_no = 0;

	if (msg_to_send == NULL || msg_to_send->buf == NULL ||
			msg_to_send->addr[0] == '\0' || pdu == NULL)
		return 0;

	if (msg_to_send->num_seg == 0 || msg_to_send->len == 0)
		return 0;

	if ((msg_to_send->seq_no > msg_to_send->num_seg) ||
			(msg_to_send->num_seg > 1 && msg_to_send->seq_no == 0))
		return 0;

	char *wptr = pdu;
	uint8_t val = 0;

	/* Write the first octet and message reference number into the PDU */
	val = MTI_SMS_SUBMIT_TYPE | RD_ACCEPT | VPF_ABSENT | SRR_REPORT_NOT_REQ
		| RP_NOT_SET;
	if (msg_to_send->num_seg > 0)
		val |= UDHI_PRESENT;
	ON_FAIL(hexstr(val, &wptr), 0);
	ON_FAIL(hexstr(msg_ref_no, &wptr), 0);
	msg_ref_no++;

	/* Write the destination address length and the encoded address */
	ON_FAIL(smscodec_encode_addr(msg_to_send->addr, &wptr), 0);

	/* Write the PID and DCS fields */
	ON_FAIL(hexstr(PID_VAL, &wptr), 0);
	ON_FAIL(hexstr(DCS_VAL, &wptr), 0);

	/* Write the User Data field */
	ON_FAIL(encode_ud(msg_to_send, &wptr), 0);

	return (wptr - pdu);
}

static bool decode_addr(const char **pdu, msg_t *recv_msg)
{
	uint32_t val = 0;
	/* Read the length */
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	return true;
}

static bool decode_bd(const char **pdu, msg_t *recv_msg)
{
	uint32_t val = 0;
	/* Read the length */
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	return true;
}

static bool decode_bd_msg_ident(const char **pdu, msg_t *recv_msg)
{
	uint32_t val = 0;
	/* Read the length */
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	return true;
}

static bool decode_bd_user_data(const char **pdu, msg_t *recv_msg)
{
	uint32_t val = 0;
	/* Read the length */
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	return true;
}

bool smscodec_decode(uint8_t len, const char *pdu, msg_t *recv_msg)
{
	if (pdu == NULL || recv_msg == NULL || recv_msg->buf == NULL)
		return false;

	const char *rptr = pdu;

	/* The PDU always begins with a message type */
	uint32_t msg_type;
	ON_FAIL(hexnum(&rptr, HEXLEN, &msg_type), false);
	if (msg_type != SMS_P2P_TYPE)
		return false;

	udh_present = false;

	/*
	 * The rest of the PDU consists of multiple structures of the form:
	 * {PARAM_IDENTIFIER, PARAM_DATA_LEN, PARAM_DATA}
	 * These structures can appear in any order in the PDU.
	 */
	while (rptr != pdu + len) {
		uint32_t val = 0;
		ON_FAIL(hexnum(&rptr, HEXLEN, &val), false);
		switch (val) {
		case TELESVC_IDENT:
			ON_FAIL(hexnum(&rptr, HEXLEN, &val), false);
			if (val != TELESVC_LEN)
				return false;
			ON_FAIL(hexnum(&rptr, val * HEXLEN, &val), false);
			if (val != TELESVC_WMT)
				return false;
			break;
		case ORIG_ADDR:
			ON_FAIL(decode_addr(&rptr, recv_msg), false);
			break;
		case BEARER_DATA:
			ON_FAIL(decode_bd(&rptr, recv_msg), false);
			break;
		default:
			/* Ignore all other parameters */
			ON_FAIL(hexnum(&rptr, HEXLEN, &val), false);
			rptr += (val * HEXLEN);
			break;
		}
	}
	return true;
}
