/*
 * Copyright (C) 2017 Sycamore Labs, LLC, All Rights Reserved
 *
 * bme280-beduin.c
 *
 * interface to bme280 on beduin-v2
 *
 */

#include "bme280-beduin.h"
#include <stdint.h>
#include "bme280.h"
#include "dbg.h"
#include "i2c_hal.h"
#include "sys.h"
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

struct bme280_t bme280;

void int_to_buffer(char *buffer, int32_t n);
int32_t bme280_beduin_set_operational_mode(void);
float bme280_beduin_read_pressure(void);

void bme280_beduin_init(void)
{
     uint32_t timeout_ms = 0;
     i2c_handle =  i2c_init(I2C_SCL, I2C_SDA, timeout_ms);
     bme280_beduin_set_operational_mode();
}

int32_t bme280_beduin_set_operational_mode(void) 
{
   /* result of communication results*/
    int32_t com_rslt = E_ERROR;

    // routines to talk to the I2C
    bme280.bus_write = BME280_I2C_bus_write;
    bme280.bus_read = BME280_I2C_bus_read;
    bme280.dev_addr = BME280_I2C_ADDRESS2; // BME280 on beduin is strapped for 0x77
    bme280.delay_msec = BME280_delay_msek;

     // init the bme280 struct
    com_rslt = bme280_init(&bme280);

    /*	For initialization it is required to set the mode of
     *	the sensor as "NORMAL"
     *	data acquisition/read/write is possible in this mode
     *	by using the below API able to set the power mode as NORMAL*/
     /* Set the power mode as NORMAL*/
     com_rslt += bme280_set_power_mode(BME280_NORMAL_MODE);
    
    /*	For reading the pressure, humidity and temperature data it is required to
     *	set the OSS setting of humidity, pressure and temperature
     * The "BME280_CTRLHUM_REG_OSRSH" register sets the humidity
     * data acquisition options of the device.
     * changes to this registers only become effective after a write operation to
     * "BME280_CTRLMEAS_REG" register.
     * In the code automated reading and writing of "BME280_CTRLHUM_REG_OSRSH"
     * register first set the "BME280_CTRLHUM_REG_OSRSH" and then read and write
     * the "BME280_CTRLMEAS_REG" register in the function
     */
    com_rslt += bme280_set_oversamp_humidity(BME280_OVERSAMP_16X);

    /* set the pressure oversampling*/
    com_rslt += bme280_set_oversamp_pressure(BME280_OVERSAMP_16X);

    /* set the temperature oversampling*/
    com_rslt += bme280_set_oversamp_temperature(BME280_OVERSAMP_1X);


    /* This API used to Write the standby time of the sensor input
     *	value have to be given
     *	Normal mode comprises an automated perpetual cycling between an (active)
     *	Measurement period and an (inactive) standby period.
     *	The standby time is determined by the contents of the register t_sb.
     *	Standby time can be set using BME280_STANDBYTIME_125_MS.
     *	Usage Hint : bme280_set_standbydur(BME280_STANDBYTIME_125_MS)*/
     com_rslt += bme280_set_standby_durn(BME280_STANDBY_TIME_125_MS);

     return com_rslt;
}

void int_to_buffer(char *buffer, int32_t n)
{
  buffer[0] = (n >> 24) & 0xFF;
  buffer[1] = (n >> 16) & 0xFF;
  buffer[2] = (n >> 8) & 0xFF;
  buffer[3] = n & 0xFF;
}

int32_t bme280_beduin_read_sensor(array_t *buffer_struct)
{

  int32_t v_data_uncomp_hum_s32 = BME280_INIT_VALUE;
  int32_t v_data_uncomp_pres_s32 = BME280_INIT_VALUE;
  int32_t v_data_uncomp_temp_s32 = BME280_INIT_VALUE;
  int32_t v_data_temp[2] = {BME280_INIT_VALUE,BME280_INIT_VALUE};
  uint32_t v_data_pres[2] = {BME280_INIT_VALUE,BME280_INIT_VALUE};
  uint32_t v_data_hum[2] = {BME280_INIT_VALUE,BME280_INIT_VALUE};
  int32_t com_rslt = E_ERROR;
  com_rslt = bme280_beduin_wake();
  sys_delay(1000);  
  // let's make sure t_fine is set
  com_rslt += bme280_read_uncomp_temperature(&v_data_uncomp_temp_s32);
  com_rslt += bme280_read_uncomp_pressure(&v_data_uncomp_pres_s32);
  com_rslt += bme280_read_uncomp_humidity(&v_data_uncomp_hum_s32);
  v_data_temp[0] = bme280_compensate_temperature_int32(v_data_uncomp_temp_s32);
  v_data_pres[0] = bme280_compensate_pressure_int32(v_data_uncomp_pres_s32);
  v_data_hum[0] = bme280_compensate_humidity_int32(v_data_uncomp_hum_s32);

 char *buffer = (char *)buffer_struct->bytes; 
  
  int_to_buffer(&buffer[0],(int32_t)v_data_pres[0]);
  int_to_buffer(&buffer[4],v_data_temp[0]);
  int_to_buffer(&buffer[8],(int32_t)v_data_hum[0]);
    
  buffer_struct->sz = 4 * 3;
  float imp_temp = ((float)(v_data_temp[0])/100)*1.8+32;// convert to fahrenheit
  float imp_press = ((float)(v_data_pres[0])/100)*.0295300; // convert to inches of mercury
  float imp_humi = ((float)(v_data_hum[0])/1024);// relative humidity
  float dewpt = ((float)v_data_temp[0]/100) - ((100 - imp_humi) / 5. );
  dewpt = dewpt * 1.8 + 32;
  dbg_printf("Temp: %.f DegF,  Press: %.2f inHg,  Humi: %.f%% rH,  DewPt: %.f DegF\r\n",
	 imp_temp,
	 imp_press,
	 imp_humi,
	 dewpt); 
  bme280_beduin_sleep();
  return com_rslt;

}

int32_t bme280_beduin_wake(void)
{
  int32_t com_rslt = E_ERROR;
  com_rslt +=  bme280_set_power_mode(BME280_NORMAL_MODE);
  return com_rslt;
}
int32_t bme280_beduin_sleep(void)
{
  int32_t com_rslt = E_ERROR;
  com_rslt += bme280_set_power_mode(BME280_SLEEP_MODE);
  return com_rslt;
}
/*-------------------------------------------------------------------*
*	This is a sample code for read and write the data by using I2C/SPI
*	Use either I2C or SPI based on your need
*	The device address defined in the bme280.h file
*-----------------------------------------------------------------------*/
 /*	\Brief: The function is used as I2C bus write
 *	\Return : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
s8 BME280_I2C_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
    int32_t iError = BME280_INIT_VALUE;

    i2c_dest_addr.slave = dev_addr;
    i2c_dest_addr.reg = reg_addr;
    EOE(i2c_write(i2c_handle, i2c_dest_addr, cnt ,  (uint8_t *)&reg_data));
    return (int8_t)iError;
}

 /*	\Brief: The function is used as I2C bus read
 *	\Return : Status of the I2C read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be read
 *	\param reg_data : This data read from the sensor, which is hold in an array
 *	\param cnt : The no of data byte of to be read
 */
s8 BME280_I2C_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
    int32_t iError = BME280_INIT_VALUE;
    uint8_t array[I2C_BUFFER_LEN] = {BME280_INIT_VALUE};
    uint8_t stringpos = BME280_INIT_VALUE;
    array[BME280_INIT_VALUE] = reg_addr;

    i2c_dest_addr.slave = dev_addr;
    i2c_dest_addr.reg = reg_addr;
    EOE(i2c_read(i2c_handle, i2c_dest_addr, cnt, (uint8_t *)*&array));

    for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++) {
		*(reg_data + stringpos) = array[stringpos];
    }

    return (int8_t)iError;
}

/*	Brief : The delay routine
 *	\param : delay in ms
*/
void BME280_delay_msek(uint32_t msek)
{
	/*Here you can write your own delay routine*/
	sys_delay(10*msek);
}

/*
float bme280_beduin_read_pressure(void) {
  int64_t var1, var2, p;
  struct bme280_t *p_bme280 = &bme280;
  struct bme280_calibration_param_t cal_param = p_bme280->cal_param;;


   u8 a_data_u8[BME280_PRESSURE_DATA_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE};

   int32_t com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_PRESSURE_MSB_REG,
			a_data_u8, BME280_PRESSURE_DATA_LENGTH);
   dbg_printf("I2C read return = %ld", com_rslt);
  s32 data_from_wire = a_data_u8[0];
  data_from_wire <<= 8;
  data_from_wire |= a_data_u8[1];
  data_from_wire <<= 8;
  data_from_wire |= a_data_u8[2];
  data_from_wire >>= 4;

  var1 = ((int64_t)cal_param.t_fine) - 128000;
  var2 = var1 * var1 * (s64)cal_param.dig_P6;;
  var2 = var2 + ((var1*(s64)cal_param.dig_P5)<<17);
  var2 = var2 + (((s64)cal_param.dig_P4)<<35);
  var1 = ((var1 * var1 * (int64_t)cal_param.dig_P3)>>8) +
    ((var1 * (s64)cal_param.dig_P2)<<12);
  var1 = (((((int64_t)1)<<47)+var1))*((s64)cal_param.dig_P1)>>33;

  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - data_from_wire;;
  p = (((p<<31) - var2)*3125) / var1;
  var1 = (((s64)cal_param.dig_P9) * (p>>13) * (p>>13)) >> 25;
  var2 = (((s64)cal_param.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((s64)cal_param.dig_P7)<<4);
  return (float)p/256;
}
*/
