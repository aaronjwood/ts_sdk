/*
 * Copyright (C) 2017 Verizon. All rights reserved.
 * Configuration header for Sequans Monarch CAT-M modem for various protocols and SIM cards.
 */
#ifndef TS_SDK_MODEM_CONFIG_H
#define TS_SDK_MODEM_CONFIG_H

/*
 * Only the CAT-M SIM card is allowed on this modem
 */
#define CAT_M			0

/*
 * The SIM_TYPE macro can be used to specialize the configuration for a given
 * scenario.
 */
#define SIM_TYPE		CAT_M

#endif
