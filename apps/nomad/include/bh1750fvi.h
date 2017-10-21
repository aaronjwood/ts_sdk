/**++
 * Copyright (C) 2017 SycamoreLabs, LLC, All Rights Reserved.
 *
 * bh1750fvi.h
 * ROHM Semi light sensor
 *
 **/

#ifndef __BH1750FVI_H__
#define __BH1750FVI_H__

#include "pin_std_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BH1750FVI_INIT_VALUE 0 

/*
 * The results (16 bit) need to be divided by 1.2. Use fixed math.
 * multiply divisor and raw by 65535 
 */ 
#define RES_DIV                 (78642)
#define OP_POWER_DOWN           (0x00)
#define OP_POWER_ON             (0x01)
#define OP_RESET                (0x03)
#define OP_CONT_HRES1           (0x10)
#define OP_CONT_HRES2           (0x11)
#define OP_CONT_LRES            (0x13)
#define OP_SINGLE_HRES1         (0x20)
#define OP_SINGLE_HRES2         (0x21)
#define OP_SINGLE_LRES          (0x23)
#define OP_CHANGE_TIME_H_MASK   (0x40)
#define OP_CHANGE_TIME_L_MASK   (0x60)

#define DELAY_HMODE             (120000)    /**< typ. 120ms in H-mode */
#define DELAY_LMODE             (16000)     /**< typ. 16ms in L-mode */

#define BH1750FVI_ADDR_PIN_HIGH          (0x5c)      
#define BH1750FVI_ADDR_PIN_LOW         (0x23)      
#define BH1750FVI_DEFAULT_ADDR          BH1750FVI_ADDR_PIN_LOW

#define BH1750FVI_I2C_MAX_CLK           I2C_SPEED_FAST

#define	BH1750FVI_3MS_DELAY	(3)
#define BH1750FVI_REGISTER_READ_DELAY (1)

int8_t i2c_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
int8_t i2c_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
void i2c_delay_msek(uint32_t msek);

void bh1750fvi_init(periph_t i2c);

uint16_t bh1750fvi_read_sensor(void);

#ifdef __cplusplus
}
#endif 

#endif /* __BH1750FVI_H__ */
 
