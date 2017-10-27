/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#ifdef SIMULATE_SENSOR
/* Dont care */
#define PB8	0
#define PB9	0
#endif

#define I2C_SCL		PB8   /* I2C Serial clock pin */
#define I2C_SDA		PB9   /* I2C Serial data pin */

#endif
