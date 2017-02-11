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

	DTMF_0 = 0x0A,			/* Binary DTMF digit codes */
	DTMF_1 = 0x01,
	DTMF_2 = 0x02,
	DTMF_3 = 0x03,
	DTMF_4 = 0x04,
	DTMF_5 = 0x05,
	DTMF_6 = 0x06,
	DTMF_7 = 0x07,
	DTMF_8 = 0x08,
	DTMF_9 = 0x09
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

	/* The header for the encoded output is its length and number type */
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
	if (dest == NULL || *dest == NULL)
		return false;

	int written = snprintf(*dest, HEXLEN + 1, "%02X", data);
	if (written <= 0)
		return false;
	*dest += written;
	return true;
}

/*
 * Convert a hex string of a given length to an unsigned 32-bit integer. The
 * result is stored in 'num'. It is assumed that num is an array of 8-bit
 * integers at least 'len / HEXLEN' bytes long.
 * Return 'true' on success and 'false' on failure.
 * On a successful call, (*src) is updated to point to the memory location right
 * after the last byte read.
 */
static bool hexnum(const char **src, uint8_t len, uint8_t *num)
{
	if (src == NULL || *src == NULL || num == NULL)
		return false;

	memset(num, 0, len / HEXLEN);
	for (uint8_t i = 0; i < len; i++) {
		uint8_t digit = (*src)[i];

		uint8_t idx = i >> 1;
		num[idx] *= 16;

		if (digit >= '0' && digit <= '9')
			num[idx] += (digit - '0');
		else if (digit >= 'A' && digit <= 'F')
			num[idx] += (10 + digit - 'A');
		else if (digit >= 'a' && digit <= 'f')
			num[idx] += (digit - 'a');
		else
			return false;
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

/*
 * Extract the first 'nbits' starting at bit 'start_bit' from the array 'src'.
 * The array is assumed to be long enough so that all bits can be extracted
 * successfully without crossing any memory boundaries.
 * 'nbits' cannot exceed 32. 'src' is assumed to store binary data in the
 * big-endian format. Local storage is assumed to be little-endian.
 * After a successful call 'start_bit' is incremented by 'nbits'.
 */
static uint32_t extract_bits(uint32_t *start_bit, uint8_t nbits, uint8_t *src)
{
	if (start_bit == NULL || nbits == 0 || src == NULL)
		return 0;

	if (nbits > 32)
		nbits = 32;

	uint8_t byte_offset = *start_bit >> 3;
	uint8_t bit_offset = *start_bit & 0x07;
	uint8_t n_bytes = nbits >> 3;
	uint8_t bits_remaining = nbits & 0x07;
	uint8_t whole_bytes = n_bytes + (bits_remaining > 0) + (bit_offset > 0);

	uint32_t bits = 0;
	memcpy(&bits, src + byte_offset, whole_bytes);
	bits = __builtin_bswap32(bits);
	bits <<= bit_offset;
	bits >>= (32 - nbits);

	*start_bit += nbits;
	return bits;
}

/*
 * Decode the address into a phone number. Data network addresses are not
 * supported. Digits are expected to be encoded using binary DTMF. Returns
 * 'true' on success, 'false' on failure.
 */
#define MAX_OA_PARAM_LEN		7
static bool decode_addr(const char **pdu, msg_t *recv_msg)
{
	/* Extract length of the field */
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	if (val > MAX_OA_PARAM_LEN)
		return false;

	/* Extract the binary data associated with the field */
	uint8_t bin[MAX_OA_PARAM_LEN];
	uint32_t bit_idx = 0;
	ON_FAIL(hexnum(pdu, val * HEXLEN, bin), false);

	/* Only binary DTMF encoded digits and no data network address */
	bool digit_mode = extract_bits(&bit_idx, 1, bin);
	bool number_mode = extract_bits(&bit_idx, 1, bin);
	if (digit_mode || number_mode)
		return false;

	/* Decode the phone number */
	val = extract_bits(&bit_idx, 8, bin);	/* Number of digits in address */
	for (uint8_t i = 0; i < val; i++) {
		uint8_t digit = extract_bits(&bit_idx, 4, bin);
		printf("%02X\n", digit);
	}

	return true;
}

static bool decode_bd(const char **pdu, msg_t *recv_msg)
{
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	*pdu += (val * HEXLEN);
	return true;
}

/*
static bool decode_bd_msg_ident(const char **pdu, msg_t *recv_msg)
{
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	*pdu += (val * HEXLEN);
	return true;
}

static bool decode_bd_user_data(const char **pdu, msg_t *recv_msg)
{
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	*pdu += (val * HEXLEN);
	return true;
}*/

bool smscodec_decode(uint8_t len, const char *pdu, msg_t *recv_msg)
{
	if (pdu == NULL || recv_msg == NULL || recv_msg->buf == NULL)
		return false;

	const char *rptr = pdu;

	/* The PDU always begins with a message type */
	uint8_t msg_type;
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
		uint8_t val[2] = {0};
		ON_FAIL(hexnum(&rptr, HEXLEN, val), false);
		switch (val[0]) {
		case TELESVC_IDENT:
			ON_FAIL(hexnum(&rptr, HEXLEN, val), false);
			if (val[0] != TELESVC_LEN)
				return false;
			ON_FAIL(hexnum(&rptr, val[0] * HEXLEN, val), false);
			if (val[0] != (TELESVC_WMT >> 8) ||
					val[1] != (TELESVC_WMT & 0xFF))
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
			ON_FAIL(hexnum(&rptr, HEXLEN, val), false);
			rptr += (val[0] * HEXLEN);
			break;
		}
	}
	return true;
}
