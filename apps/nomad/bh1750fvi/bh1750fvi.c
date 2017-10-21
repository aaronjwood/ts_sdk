/**++
 * Copyright (C) 2017 SycamoreLabs, LLC, All Rights Reserved.
 *
 * bh1750fvi.c
 * ROHM Semi light sensor
 *
 **/
#include <stdio.h>
#include "bh1750fvi.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "sys.h"
#include "dbg.h"
#include "board_interface.h"

#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)

#define	I2C_BUFFER_LEN 28
#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */
periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;


void bh1750fvi_init(periph_t i2c)
{
	dbg_printf("Initing bh1750fvi\r\n");
	i2c_handle = i2c;
	gpio_config_t ls_reset_pin;
    pin_name_t ls_reset_pin_name = PB15;
    ls_reset_pin.dir = INPUT;
    ls_reset_pin.pull_mode = PP_NO_PULL;
    ls_reset_pin.speed = SPEED_VERY_HIGH;
    gpio_init(ls_reset_pin_name, &ls_reset_pin);
	gpio_write(ls_reset_pin_name, PIN_LOW);
	sys_delay(2);
	gpio_write(ls_reset_pin_name, PIN_HIGH);
	

  uint8_t command = OP_POWER_DOWN;
  int8_t result = i2c_bus_write(BH1750FVI_DEFAULT_ADDR, 0x00, &command, 1);
  if (result < 0 ) {
    dbg_printf("Error initing light sensor. error = %d", result);
  } else {
    dbg_printf("Light sensor inited!\r\n");
  }
}

uint16_t bh1750fvi_read_sensor(void)
{
  uint32_t tmp;
  uint8_t raw[2];
  
  uint8_t command = OP_POWER_ON;

  int8_t result = i2c_bus_write(BH1750FVI_DEFAULT_ADDR,0x00, &command, 1);
  command = OP_SINGLE_HRES1;
  result = i2c_bus_write(BH1750FVI_DEFAULT_ADDR,0x00, &command, 1);
  
  dbg_printf("wrote to light sensor");
  i2c_delay_msek(DELAY_HMODE);
  
  result = i2c_bus_read(BH1750FVI_DEFAULT_ADDR, 0x00, raw, 2);
  tmp = (raw[0] << 24) | (raw[1] << 16);
  tmp /= RES_DIV;
  dbg_printf("light sensor result = %d\r\n", result);
  return (uint16_t)(tmp);
}

int8_t i2c_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
  
  int32_t iError = BH1750FVI_INIT_VALUE;
    i2c_dest_addr.slave = dev_addr;
    i2c_dest_addr.reg = reg_addr;
    EOE(i2c_write(i2c_handle, i2c_dest_addr, cnt ,  (uint8_t *)&reg_data));
    return (int8_t)iError;
 
  return (int8_t)iError;
}

/*\Brief: The function is used as I2C bus read
 *\Return : Status of the I2C read
 *\param dev_addr : The device address of the sensor
 *\param reg_addr : Address of the first register, will data is going to be read
 *\param reg_data : This data read from the sensor, which is hold in an array
 *\param cnt : The no of data byte of to be read
 */
int8_t i2c_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
  
  int32_t iError = BH1750FVI_INIT_VALUE;
  uint8_t array[I2C_BUFFER_LEN] = {BH1750FVI_INIT_VALUE};
  uint8_t stringpos = BH1750FVI_INIT_VALUE;
  array[BH1750FVI_INIT_VALUE] = reg_addr;

	i2c_dest_addr.slave = dev_addr;
	i2c_dest_addr.reg = reg_addr;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, cnt, (uint8_t *)*&array));

	for (stringpos = BH1750FVI_INIT_VALUE; stringpos < cnt; stringpos++) {
		*(reg_data + stringpos) = array[stringpos];
	}


  return (int8_t)iError;
}

/*Brief : The delay routine
 *\param : delay in ms
 */
void i2c_delay_msek(uint32_t msek)
{
  /*Here you can write your own delay routine*/
sys_delay(10*msek);
}
