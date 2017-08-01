/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * Raspberry pi 3 does not support peripherals for now
 *
 * Target board : Raspberry Pi 3
 * Target SoC   : BCM2837
 */

#include "i2c_hal.h"

periph_t i2c_init(pin_name_t scl, pin_name_t sda, uint32_t timeout_ms)
{
	return NO_PERIPH;
}

bool i2c_write(periph_t hdl, i2c_addr_t addr, uint8_t len, const uint8_t *buf)
{
        return false;
}

bool i2c_read(periph_t hdl, i2c_addr_t addr, uint8_t len, uint8_t *buf)
{
	return false;
}

void i2c_pwr(periph_t hdl, bool state)
{
	/* XXX: Stub for now */
}
