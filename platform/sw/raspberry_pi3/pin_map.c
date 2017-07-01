/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include "pin_map.h"

/*
 * The following tables are just a stub, raspberry pi 3 does not support
 * peripherals
 *
 * Target board : Raspberry Pi 3
 * Target SoC   : BCM2837
 */

#define END_OF_MAP	{NC, {0}, 0, 0}


const pin_map_t uart_tx_map[] = {
	END_OF_MAP
};

const pin_map_t uart_rx_map[] = {
	END_OF_MAP
};

const pin_map_t uart_rts_map[] = {
	END_OF_MAP
};

const pin_map_t uart_cts_map[] = {
	END_OF_MAP
};


const pin_map_t i2c_sda_map[] = {
	END_OF_MAP
};

const pin_map_t i2c_scl_map[] = {
	END_OF_MAP
};
