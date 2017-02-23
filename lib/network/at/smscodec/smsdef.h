/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef __SMSDEF_H
#define __SMSDEF_H

#define UDH_LEN				5	/* Length of total user data header */

/*
 * Sizes here are in bytes taken up in the final PDU hex string. This means '4A'
 * occupies two bytes.
 */

/* Outgoing PDU */
#define ENC_ADDR_SZ			19	/* 15 + 2 (len) + 2 (number type) */
#define OUT_PDU_OVR_HD			(10 + ENC_ADDR_SZ)
#define UD_SZ				(140*2)	/* Valid only for 8-bit encoding */

/* Incoming PDU */
#define IN_PDU_OVR_HD			(33*2)

#endif
