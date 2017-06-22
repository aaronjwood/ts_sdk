/*
 * Copyright (C) 2017 Verizon. All rights reserved.
 * Configuration header for U-Blox TOBY-L201 for various protocols and SIM cards.
 */
#ifndef TS_SDK_MODEM_CONFIG_H
#define TS_SDK_MODEM_CONFIG_H

/*
 * Define the SIM type being used. Possible values are:
 * o M2M - Machine-to-Machine
 * o COM - Commercial
 * o DAK - Dakota (SMSNAS)
 */
#define M2M			0
#define COM			1
#define DAK			2

/*
 * The SIM_TYPE macro can be used to specialize the configuration for a given
 * scenario. Example: With protocol set to SMSNAS, UMNOCONF takes "0,0" when
 * using Dakota SIMs and takes "1,6" when using another type of SIM.
 */
#define SIM_TYPE		COM


/* SIM specific APN settings */
#if SIM_TYPE==M2M

#define MODEM_APN_CTX_ID	"1"
#define MODEM_APN_TYPE		"IP"
#define MODEM_APN_TYPE_ID	"0"
#define MODEM_APN_VALUE		"UWSEXT.GW15.VZWENTP"

#elif SIM_TYPE == COM

#define MODEM_APN_TYPE_ID	"2"

#endif	/* SIM_TYPE */


/* Protocol and SIM specific settings */
#if defined (OTT_PROTOCOL)

#define MODEM_UMNOCONF_VAL	"3,23"

#elif defined (SMSNAS_PROTOCOL)

#if SIM_TYPE == DAK
#define MODEM_UMNOCONF_VAL	"0,0"
#else
#define MODEM_UMNOCONF_VAL	"1,6"
#endif	/* SIM_TYPE */

#else

#error "Must specify a protocol"

#endif	/* PROTOCOL */

#endif
