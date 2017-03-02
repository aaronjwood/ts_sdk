/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef __SMSDEF_H
#define __SMSDEF_H

#define UDHL_VAL			5	/* Length of total user data header */

/* Field lengths */
#define LEN_PID				1
#define LEN_DCS				1
#define LEN_FO				1
#define LEN_SCTS			7
#define LEN_UDL				1
#define LEN_MR				1

/*
 * Sizes here are in bytes taken up in the final PDU hex string. This means '4A'
 * occupies two bytes.
 */
#define ENC_ADDR_SZ			(12 * 2)
#define UD_SZ				(140 * 2)

/* Outgoing PDU overhead - Values are from SMS-SUBMIT layout in 3GPP TS 23.040 */
#define OUT_PDU_OVR_HD	((LEN_FO + LEN_MR + LEN_PID + LEN_DCS + LEN_UDL) * 2 + ENC_ADDR_SZ)

/* Incoming PDU overhead - Values are from SMS-DELIVER layout in 3GPP TS 23.040 */
#define IN_PDU_OVR_HD	((LEN_FO + LEN_PID + LEN_DCS + LEN_SCTS + LEN_UDL) * 2 + ENC_ADDR_SZ)

#endif
