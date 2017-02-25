/* Copyright (C) 2017 Verizon. All rights reserved. */

#include "smscodec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* On failure, return _val */
#define ON_FAIL(_x, _val) \
	do { \
		if (!(_x)) \
			return (_val); \
	} while(0)

#define HEXLEN			2	/* 2 hexadecimal characters = 1 Byte */

/* First octet bit definitions for SMS-SUBMIT */
#define FO_TYPE_SMS_SUBMIT	0x01	/* SMS-SUBMIT message type */
#define FO_RD_ACCEPT		0x00	/* Accept messages with duplicate ref no */
#define FO_RD_REJECT		0x04	/* Reject messages with duplicate ref no */
#define FO_VPF_ABSENT		0x00	/* Validity period absent from message */
#define FO_VPF_RELATIVE		0x08	/* Validity period is in relative format */
#define FO_VPF_ENHANCED		0x10	/* Validity period uses enhanced format */
#define FO_VPF_ABSOLUTE		0x18	/* Validity period uses absolute format */
#define FO_SRR_REPORT_REQ	0x20	/* Status report requested */
#define FO_SRR_REPORT_NOT_REQ	0x00	/* No status report requested */

/* First octet bit definitions for SMS-DELIVER (bits 3 & 4 are "Don't Care") */
#define FO_TYPE_SMS_DELIVER	0x00	/* SMS-DELIVER message type */
#define FO_MMS_TRUE		0x00	/* SC has more messages to send to MS */
#define FO_MMS_FALSE		0x04	/* SC has no more messages to send to MS */
#define FO_LP_INACTIVE		0x00	/* Loop prevention inactive / not supported */
#define FO_LP_ACTIVE		0x08	/* Message is forwarded / spawned */
#define FO_SRI_REPORT		0x20	/* Status report shall be returned */
#define FO_SRI_NO_REPORT	0x00	/* Status report shall not be returned */

/* First octet bit definitions common to both SMS-SUBMIT and SMS-DELIVER */
#define FO_UDHI_PRESENT		0x40	/* Payload has a header preceeding data */
#define FO_UDHI_ABSENT		0x00	/* Payload is entirely user data */
#define FO_RP_SET		0x80	/* A reply path was set */
#define FO_RP_NOT_SET		0x00	/* No reply path was set */

/* PID and DCS values to be used for outgoing SMSes */
#define PID_VAL			0x00	/* SME-to-SME protocol */
#define DCS_VAL			0x04	/* No message class 8-bit encoding */

/* IEI Values */
#define UDH_IEI_CONCAT		0x00	/* IEI for concatenated messages */
#define UDH_IEI_CONCAT_LEN	0x03	/* Number of concat. IEI parameters */

#define ISDN_NUM_TYPE		0x91	/* Number type */

#define SCTS_LEN		0x07	/* Length of Service Center Timestamp */

static bool udh_present;		/* Set if UDH is present in received SMS */

/*
 * Convert the byte into a string representing its hex form and write it to the
 * destination buffer.
 * Return 'true' on success and 'false' on failure.
 * On a successful call, (*dest) is updated to point to the memory location right
 * after the last byte written.
 */
static bool hexstr(char **dest, uint8_t data)
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
 * Convert a hex string of a given length to an integer. The result is stored in
 * the big endian format in the array 'num'. It is assumed the array is at least
 * 'len / HEXLEN' bytes long. The value of 'len' is the number of complete bytes
 * represented by the hexadecimal digits in the input array.
 * Return 'true' on success and 'false' on failure.
 * On a successful call, (*src) is updated to point to the memory location right
 * after the last byte read.
 */
static bool hexnum(const char **src, uint8_t len, uint8_t *num)
{
	if (src == NULL || *src == NULL || num == NULL)
		return false;

	memset(num, 0, len);
	len *= HEXLEN;
	for (uint8_t i = 0; i < len; i++) {
		uint8_t digit = (*src)[i];

		uint8_t idx = i >> 1;
		num[idx] *= 16;

		if (digit >= '0' && digit <= '9')
			num[idx] += (digit - '0');
		else if (digit >= 'A' && digit <= 'F')
			num[idx] += (10 + digit - 'A');
		else if (digit >= 'a' && digit <= 'f')
			num[idx] += (10 + digit - 'a');
		else
			return false;
	}

	*src += len;
	return true;
}

/*
 * 'intl_num' is expected to be an international number following the ISDN /
 * telephone numbering plan. Both buffers are NULL terminated.
 * Size of 'intl_num' is expected to be at most ADDR_SZ + 1 bytes. Size of
 * (*enc_intl_num) is expected to be at most ENC_ADDR_SZ + 1 bytes.
 * Returns the 'true' on success and 'false' on failure.
 * On a successful call, (*enc_intl_num) is updated to point to the memory
 * location right after the last byte of the encoded output.
 */
#define STRIDE			2
#define ADDR_HDR_SZ		4	/* Size of the header for the encoded address */
static bool encode_addr(const char *intl_num, char **enc_intl_num)
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
 * Encode the UDL (and UDH if present) along with the user data. Write the
 * encoded output into the memory pointed by (*dest).
 * Returns 'true' on success, 'false' on any error.
 * On a successful call, (*dest) is updated to point to the memory location
 * after the last byte written.
 */
static bool encode_ud(const sms_t *msg_to_send, char **dest)
{
	if (msg_to_send->num_seg > 1) {
		ON_FAIL(hexstr(dest, 1 + UDHL_VAL + msg_to_send->len), false);
		ON_FAIL(hexstr(dest, UDHL_VAL), false);
		ON_FAIL(hexstr(dest, UDH_IEI_CONCAT), false);
		ON_FAIL(hexstr(dest, UDH_IEI_CONCAT_LEN), false);
		ON_FAIL(hexstr(dest, msg_to_send->ref_no), false);
		ON_FAIL(hexstr(dest, msg_to_send->num_seg), false);
		ON_FAIL(hexstr(dest, msg_to_send->seq_no), false);
	} else if (msg_to_send->num_seg == 1) {
		ON_FAIL(hexstr(dest, msg_to_send->len), false);
	}

	for (uint8_t i = 0; i < msg_to_send->len; i++)
		ON_FAIL(hexstr(dest, msg_to_send->buf[i]), false);
	return true;
}

static bool decode_addr(const char **pdu, sms_t *recv_msg)
{
	/* Get the length of the encoded address */
	uint8_t len = 0;
	ON_FAIL(hexnum(pdu, 1, &len), false);

	/* Get the type of the address */
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, 1, &val), false);
	if (val != ISDN_NUM_TYPE)
		return false;

	/* Decode the address - take two digits at a time and swap them */
	for (uint8_t i = 0; i < len; i += HEXLEN) {
		recv_msg->addr[i] = (*pdu)[i + 1];
		if ((*pdu)[i] != 'F' && (*pdu)[i] != 'f')
			recv_msg->addr[i + 1] = (*pdu)[i];
	}
	recv_msg->addr[len] = '\0';
	len = (len & 1) ? len + 1 : len;	/* Round to next even number */
	*pdu += len;

	return true;
}

static bool decode_ud(const char **pdu, sms_t *recv_msg)
{
	/* Get the user data length */
	uint8_t len = 0, val = 0;
	ON_FAIL(hexnum(pdu, 1, &len), false);
	if (udh_present) {
		/* Parse User Data Header */
		recv_msg->len = len - UDHL_VAL - 1;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		if (val != UDHL_VAL)
			return false;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		if (val != UDH_IEI_CONCAT)
			return false;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		if (val != UDH_IEI_CONCAT_LEN)
			return false;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		recv_msg->ref_no = val;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		recv_msg->num_seg = val;
		ON_FAIL(hexnum(pdu, 1, &val), false);
		recv_msg->seq_no = val;

		for (uint8_t i = 0; i < recv_msg->len; i++) {
			ON_FAIL(hexnum(pdu, 1, &val), false);
			recv_msg->buf[i] = val;
		}
	} else {
		/* Format: Length field followed by raw 8-bit data */
		recv_msg->len = len;
		for (uint8_t i = 0; i < len; i++) {
			ON_FAIL(hexnum(pdu, 1, &val), false);
			recv_msg->buf[i] = val;
		}
		recv_msg->ref_no = 0;
		recv_msg->num_seg = 1;
		recv_msg->seq_no = 0;
	}
	return true;
}

bool smscodec_decode(uint16_t len, const char *pdu, sms_t *recv_msg)
{
	if (pdu == NULL || recv_msg == NULL || recv_msg->buf == NULL ||
			recv_msg->addr == NULL)
		return false;

	/* Assume a single part 8-bit message */
	udh_present = false;

	/*
	 * The first few bytes of the SMS-DELIVER PDU as reported by the modem
	 * is the SMSC address. This shouldn't be relevant for the SMSNAS
	 * protocol and is therefore skipped here.
	 */
	const char *rptr = pdu;
	uint8_t val = 0;
	ON_FAIL(hexnum(&rptr, 1, &val), false);
	rptr += val * HEXLEN;

	/* Retrieve the first octet */
	ON_FAIL(hexnum(&rptr, 1, &val), false);
	uint8_t fo_expected = FO_TYPE_SMS_DELIVER | FO_MMS_FALSE | FO_LP_INACTIVE |
		FO_SRI_NO_REPORT | FO_RP_NOT_SET;
	if ((val & fo_expected) != fo_expected)
		return false;
	udh_present = ((val & FO_UDHI_PRESENT) == FO_UDHI_PRESENT);

	/* Decode the originating address */
	ON_FAIL(decode_addr(&rptr, recv_msg), false);

	/* Verify PID and DCS values */
	uint8_t pid = 0, dcs = 0;
	ON_FAIL(hexnum(&rptr, 1, &pid), false);
	ON_FAIL(hexnum(&rptr, 1, &dcs), false);
	if (pid != PID_VAL && dcs != DCS_VAL)
		return false;

	/* Service center timestamp. This can be skipped since we do not use it */
	rptr += SCTS_LEN * HEXLEN;

	/* Decode the User Data */
	ON_FAIL(decode_ud(&rptr, recv_msg), false);
	return true;
}

uint16_t smscodec_encode(const sms_t *msg_to_send, char *pdu)
{
	if (msg_to_send == NULL || msg_to_send->buf == NULL ||
			msg_to_send->addr == NULL || pdu == NULL)
		return 0;

	if (msg_to_send->num_seg == 0 || msg_to_send->len == 0)
		return 0;

	if ((msg_to_send->seq_no > msg_to_send->num_seg) ||
			(msg_to_send->num_seg > 1 && msg_to_send->seq_no == 0))
		return 0;

	static uint8_t msg_ref_no = 0;
	char *wptr = pdu;
	uint8_t val = 0;

	/* Write the first octet and message reference number into the PDU */
	val = FO_TYPE_SMS_SUBMIT | FO_RD_ACCEPT | FO_VPF_ABSENT |
		FO_SRR_REPORT_NOT_REQ | FO_RP_NOT_SET;
	if (msg_to_send->num_seg > 1)
		val |= FO_UDHI_PRESENT;
	ON_FAIL(hexstr(&wptr, val), 0);
	ON_FAIL(hexstr(&wptr, msg_ref_no), 0);
	msg_ref_no++;

	/* Write the destination address length and the encoded address */
	ON_FAIL(encode_addr(msg_to_send->addr, &wptr), 0);

	/* Write the PID and DCS fields */
	ON_FAIL(hexstr(&wptr, PID_VAL), 0);
	ON_FAIL(hexstr(&wptr, DCS_VAL), 0);

	/* Write the User Data field */
	ON_FAIL(encode_ud(msg_to_send, &wptr), 0);

	return (wptr - pdu);
}
