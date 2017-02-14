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

#define FO_TYPE_SMS_SUBMIT	0x01	/* SMS-SUBMIT message type */
#define FO_RD_ACCEPT		0x00	/* Accept messages with duplicate ref no */
#define FO_RD_REJECT		0x04	/* Reject messages with duplicate ref no */
#define FO_VPF_ABSENT		0x00	/* Validity period absent from message */
#define FO_VPF_RELATIVE		0x08	/* Validity period is in relative format */
#define FO_VPF_ENHANCED		0x10	/* Validity period uses enhanced format */
#define FO_VPF_ABSOLUTE		0x18	/* Validity period uses absolute format */
#define FO_SRR_REPORT_REQ	0x20	/* Status report requested */
#define FO_SRR_REPORT_NOT_REQ	0x00	/* No status report requested */
#define FO_UDHI_PRESENT		0x40	/* Payload has a header preceeding data */
#define FO_UDHI_ABSENT		0x00	/* Payload is entirely user data */
#define FO_RP_SET		0x80	/* A reply path was set */
#define FO_RP_NOT_SET		0x00	/* No reply path was set */

#define VAL_PID			0x00	/* SME-to-SME protocol */
#define VAL_DCS			0x04	/* No message class 8-bit encoding */

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
 * intl_num is expected to be an international number following the ISDN /
 * telephone numbering plan. Both buffers are NULL terminated.
 * Size of intl_num is expected to be at most ADDR_SZ + 1 bytes. Size of
 * (*enc_intl_num) is expected to be at most ENC_ADDR_SZ + 1 bytes.
 * Returns the 'true' on success and 'false' on failure.
 * On a successful call, (*enc_intl_num) is updated to point to the memory
 * location right after the last byte of the encoded output.
 */
#define STRIDE			2
#define ADDR_HDR_SZ		4	/* Size of the header for the encoded address */
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
 * Encode the UDL (and UDH if present) along with the user data. Returns 'true'
 * on success, 'false' on any error.
 */
#define UDH_LEN			0x05	/* Length of total user data header */
#define IEI_CONCAT		0x00	/* Concatenation IEI */
#define IEI_CONCAT_LEN		3	/* Lenght of the concatenation IEI field */
static bool encode_ud(const sms_t *msg_to_send, char **dest)
{
	if (msg_to_send->num_seg > 1) {
		ON_FAIL(hexstr(dest, 1 + UDH_LEN + msg_to_send->len), false);
		ON_FAIL(hexstr(dest, UDH_LEN), false);
		ON_FAIL(hexstr(dest, IEI_CONCAT), false);
		ON_FAIL(hexstr(dest, IEI_CONCAT_LEN), false);
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

uint16_t smscodec_encode(const sms_t *msg_to_send, char *pdu)
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
	val = FO_TYPE_SMS_SUBMIT | FO_RD_ACCEPT | FO_VPF_ABSENT |
		FO_SRR_REPORT_NOT_REQ | FO_RP_NOT_SET;
	if (msg_to_send->num_seg > 1)
		val |= FO_UDHI_PRESENT;
	ON_FAIL(hexstr(&wptr, val), 0);
	ON_FAIL(hexstr(&wptr, msg_ref_no), 0);
	msg_ref_no++;

	/* Write the destination address length and the encoded address */
	ON_FAIL(smscodec_encode_addr(msg_to_send->addr, &wptr), 0);

	/* Write the PID and DCS fields */
	ON_FAIL(hexstr(&wptr, VAL_PID), 0);
	ON_FAIL(hexstr(&wptr, VAL_DCS), 0);

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
static uint32_t extract_bits(size_t *start_bit, uint8_t nbits, const uint8_t *src)
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

/* Convert the binary DTMF value into a decimal character */
enum binary_dtmf {			/* Binary DTMF digit codes */
	DTMF_DIGIT_SIZE = 4,
	DTMF_0 = 0x0A,
	DTMF_1 = 0x01,
	DTMF_2,
	DTMF_3,
	DTMF_4,
	DTMF_5,
	DTMF_6,
	DTMF_7,
	DTMF_8,
	DTMF_9
};

static inline char bin_dtmf_to_char(uint8_t bin_dtmf)
{
	switch (bin_dtmf) {
	case DTMF_0:
		return '0';
	case DTMF_1:
	case DTMF_2:
	case DTMF_3:
	case DTMF_4:
	case DTMF_5:
	case DTMF_6:
	case DTMF_7:
	case DTMF_8:
	case DTMF_9:
		return (bin_dtmf + '0');
	default:
		return '0';
	}
}

/*
 * Decode the address into a phone number. Data network addresses are not
 * supported. Digits are expected to be encoded using binary DTMF. Returns
 * 'true' on success, 'false' on failure.
 */
#define MAX_OA_PARAM_LEN		7
static bool decode_addr(const char **pdu, sms_t *recv_msg)
{
	/* Extract length of the field */
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	if (val > MAX_OA_PARAM_LEN)
		return false;

	/* Extract the binary data associated with the field */
	uint8_t bin[MAX_OA_PARAM_LEN];
	size_t bit_idx = 0;
	ON_FAIL(hexnum(pdu, val * HEXLEN, bin), false);

	/* Only binary DTMF encoded digits and no data network address */
	bool digit_mode = extract_bits(&bit_idx, 1, bin);
	bool number_mode = extract_bits(&bit_idx, 1, bin);
	if (digit_mode || number_mode)
		return false;

	/* Decode the phone number */
	val = extract_bits(&bit_idx, 8, bin);	/* Number of digits in address */
	if (val > ADDR_SZ - 1)			/* Not including the '+' */
		return false;

	memset(recv_msg->addr, 0, sizeof(recv_msg->addr));
	for (uint8_t i = 0; i < val; i++) {
		uint8_t digit = extract_bits(&bit_idx, DTMF_DIGIT_SIZE, bin);
		recv_msg->addr[i] = bin_dtmf_to_char(digit);
	}
	return true;
}

/*
 * The message identifier sub-parameter indicates the presence / absence of the
 * User Data Header from the 3GPP standard in the message payload.
 */
#define OFFSET_MSG_ID_UDHI	20	/* Bit index of the UDHI bit */
#define MSG_ID_LEN		3	/* Length of the Message ID sub-param */
#define SZ_MSG_ID_FIELD		4	/* Size of the message ID field in bits */
#define MSG_TYPE_SMS_DEL	0x01	/* Value describing SMS-DELIVER type */
static bool decode_bd_msg_ident(const char **pdu)
{
	/* Extract the length field */
	uint8_t val = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
	if (val != MSG_ID_LEN)
		return false;

	/* Extract the binary data associated with the field */
	uint8_t bin[MSG_ID_LEN];
	ON_FAIL(hexnum(pdu, val * HEXLEN, bin), false);

	/* Assert the message type is SMS-DELIVER */
	size_t bit_idx = 0;
	val = extract_bits(&bit_idx, SZ_MSG_ID_FIELD, bin);
	if (val != MSG_TYPE_SMS_DEL)
		return false;

	/* The only important bit in this binary field is the UDH indicator bit */
	bit_idx = OFFSET_MSG_ID_UDHI;
	udh_present = extract_bits(&bit_idx, 1, bin);

	return true;
}

/* Decode the User Data Header and associated user data into the provided buffer */
#define SZ_UDHL		8	/* Size of the UDHL field in bits */
#define SZ_IEI		8	/* Size of the IEI field in bits */
#define SZ_IEIL		8	/* Size of the IEI Length field in bits */
#define SZ_IEID		8	/* Size of the IEI Data field in bits */
#define SZ_NUM_FIELDS	8	/* Size of "NUM_FIELDS" field in bits */
static bool decode_bd_udh(size_t *bit_idx, const uint8_t *bin, sms_t *recv_msg)
{
	uint8_t num_fields = extract_bits(bit_idx, SZ_NUM_FIELDS, bin);
	if (num_fields > MAX_BUF_SZ)
		return false;

	/* Parse UD header */
	uint8_t udhl = extract_bits(bit_idx, SZ_UDHL, bin);
	size_t end = *bit_idx + udhl * SZ_IEID;
	while (*bit_idx < end) {
		uint8_t iei = extract_bits(bit_idx, SZ_IEI, bin);
		uint8_t iei_len = extract_bits(bit_idx, SZ_IEIL, bin);
		switch (iei) {
		case IEI_CONCAT:
			if (iei_len != IEI_CONCAT_LEN)
				return false;
			recv_msg->ref_no = extract_bits(bit_idx, SZ_IEID, bin);
			recv_msg->num_seg = extract_bits(bit_idx, SZ_IEID, bin);
			recv_msg->seq_no = extract_bits(bit_idx, SZ_IEID, bin);
			break;
		default:
			return false;
		}
	}

	/* Only data remains */
	num_fields -= (udhl + 1);	/* Account for the UDHL field */
	recv_msg->len = num_fields;
	for (uint8_t i = 0; i < num_fields; i++)
		recv_msg->buf[i] = extract_bits(bit_idx, SZ_IEID, bin);
	return true;
}

/*
 * Decode the user data present in the SMS. This codec only handles 8-bit
 * encoded single or multi-part messages.
 */
#define MAX_UD_LEN	(MAX_BUF_SZ + 3) /* Max. user data parameter length */
#define SZ_ENC_FIELD	5                /* Size of the encoding field in bits */
#define SZ_DATA		8                /* Size of each data value in bits */
#define ENC_8BIT	0x00             /* Encoding Type : 8-Bit */
#define ENC_DCS		0x0A             /* Encoding Type : DCS */
#define ENC_DCS_VAL	0x04             /* Expected DCS encoding value */
static bool decode_bd_user_data(const char **pdu, sms_t *recv_msg)
{
	uint8_t len = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &len), false);
	if (len > MAX_UD_LEN)
		return false;

	uint8_t bin[len];
	ON_FAIL(hexnum(pdu, len * HEXLEN, bin), false);
	size_t bit_idx = 0;
	uint8_t enc = extract_bits(&bit_idx, SZ_ENC_FIELD, bin);
	recv_msg->num_seg = 1;
	if (!udh_present) {		/* Single part message */
		if (enc != ENC_8BIT)
			return false;
		/* Message Type field is omitted */
		uint8_t num_fields = extract_bits(&bit_idx, SZ_NUM_FIELDS, bin);
		if (num_fields > MAX_BUF_SZ)
			return false;

		recv_msg->len = num_fields;
		for (uint8_t i = 0; i < num_fields; i++)
			recv_msg->buf[i] = extract_bits(&bit_idx, SZ_DATA, bin);
	} else {			/* Multi-part message */
		if (enc != ENC_DCS)
			return false;
		/* Message Type field has the value describing the DCS encoding */
		uint8_t msg_type = extract_bits(&bit_idx, 8, bin);
		if (msg_type != ENC_DCS_VAL)
			return false;

		/* Decode the UDH header and user data into recv_msg */
		ON_FAIL(decode_bd_udh(&bit_idx, bin, recv_msg), false);
	}
	return true;
}

/*
 * The Bearer Data field comprises of sub-parameters. Only two sub-parameters
 * are parsed for incoming messages:
 * 1) The message ID
 * 2) User Data
 */
#define SUB_PARAM_MSG_ID	0x00	/* Bearer data sub-parameter: Message ID */
#define SUB_PARAM_UD		0x01	/* Bearer data sub-parameter: User Data */
static bool decode_bd(const char **pdu, sms_t *recv_msg)
{
	uint8_t len = 0;
	ON_FAIL(hexnum(pdu, HEXLEN, &len), false);
	const char *end = *pdu + len * HEXLEN;

	while (*pdu < end) {
		uint8_t val = 0;
		ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
		switch (val) {
		case SUB_PARAM_MSG_ID:
			ON_FAIL(decode_bd_msg_ident(pdu), false);
			break;
		case SUB_PARAM_UD:
			ON_FAIL(decode_bd_user_data(pdu, recv_msg), false);
			break;
		default:
			/* Ignore all other sub-parameters */
			ON_FAIL(hexnum(pdu, HEXLEN, &val), false);
			*pdu += (val * HEXLEN);
			break;
		}
	}

	return true;
}

/*
 * This codec accepts only two types of teleservices: WEMT (for 8-bit concat.
 * messages) and WMT (for 8-bit single part SMS).
 */
#define TELESVC_WMT	4098	/* Wireless messaging teleservice type */
#define TELESVC_WEMT	4101	/* Wireless enhanced messaging teleservice type */
#define TELESVC_LEN	2	/* Length of teleservice parameter in bytes */
static bool decode_telesvc(const char **pdu)
{
	uint8_t val[2] = {0};
	/* Parse Length */
	ON_FAIL(hexnum(pdu, HEXLEN, val), false);
	if (val[0] != TELESVC_LEN)
		return false;

	/* Parse teleservice type */
	ON_FAIL(hexnum(pdu, val[0] * HEXLEN, val), false);
	if ((val[0] != (TELESVC_WEMT >> 8) || val[1] != (TELESVC_WEMT & 0xFF)) &&
		(val[0] != (TELESVC_WMT >> 8) || val[1] != (TELESVC_WMT & 0xFF)))
		return false;
	return true;
}

#define SMS_P2P_TYPE	0x00	/* Message type is SMS point-to-point */
#define TELESVC_ID	0x00	/* Teleservice parameter identifier */
#define ORIG_ADDR_ID	0x02	/* Originating address identifier */
#define BEARER_DATA_ID	0x08	/* Bearer data identifier */
bool smscodec_decode(uint8_t len, const char *pdu, sms_t *recv_msg)
{
	if (pdu == NULL || recv_msg == NULL || recv_msg->buf == NULL)
		return false;

	const char *rptr = pdu;

	/* The PDU always begins with a message type */
	uint8_t msg_type;
	ON_FAIL(hexnum(&rptr, HEXLEN, &msg_type), false);
	if (msg_type != SMS_P2P_TYPE)
		return false;

	/* Assume a single part 8-bit message */
	udh_present = false;

	/*
	 * The rest of the PDU consists of multiple structures of the form:
	 * {PARAM_IDENTIFIER, PARAM_DATA_LEN, PARAM_DATA}
	 * These structures can appear in any order in the PDU.
	 */
	while (rptr < pdu + len) {
		uint8_t val = 0;
		ON_FAIL(hexnum(&rptr, HEXLEN, &val), false);
		switch (val) {
		case TELESVC_ID:
			ON_FAIL(decode_telesvc(&rptr), false);
			break;
		case ORIG_ADDR_ID:
			ON_FAIL(decode_addr(&rptr, recv_msg), false);
			break;
		case BEARER_DATA_ID:
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
