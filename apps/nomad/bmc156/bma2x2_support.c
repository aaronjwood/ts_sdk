/**
  * Copyright (C) 2017 Sycamore Labs, LLC, All Rights Reserved
  *
  * bma2x2_support.c
*/

#include "bma2x2.h"
#include "dbg.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "sys.h"
#include "board_interface.h"

typedef struct {
	uint8_t sz;		/* Number of bytes contained in the data buffer */
	uint8_t *bytes;		/* Pointer to the data buffer */
} array_t;


/*----------------------------------------------------------------------------*
*	The following functions are used for reading and writing of
*	sensor data using I2C or SPI communication
*----------------------------------------------------------------------------*/

 /*	\Brief: The function is used as I2C bus read
 *	\Return : Status of the I2C read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register,
 *               will data is going to be read
 *	\param reg_data : This data read from the sensor,
 *               which is hold in an array
 *	\param cnt : The no of byte of data to be read
 */
s8 BMA2x2_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
 /*	\Brief: The function is used as I2C bus write
 *	\Return : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register,
 *              will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
s8 BMA2x2_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
/* \Brief: The function is used as SPI bus write
 * \Return : Status of the SPI write
 * \param dev_addr : The device address of the sensor
 * \param reg_addr : Address of the first register,
 *      will data is going to be written
 * \param reg_data : It is a value hold in the array,
 *	will be used for write the value into the register
 * \param cnt : The no of byte of data to be write
 */
/********************End of I2C/SPI function declarations*******************/
/*	Brief : The delay routine
 *	\param : delay in ms
 */
void BMA2x2_delay_msek(u32 msek);
/*!
 *	@brief This function is an example for delay
 *	@param : None
 *	@return : communication result
 */
s32 bma2x2_data_readout_template(void);
/*----------------------------------------------------------------------------*
*  struct bma2x2_t parameters can be accessed by using bma2x2
 *	bma2x2_t having the following parameters
 *	Bus write function pointer: BMA2x2_WR_FUNC_PTR
 *	Bus read function pointer: BMA2x2_RD_FUNC_PTR
 *	Burst read function pointer: BMA2x2_BRD_FUNC_PTR
 *	Delay function pointer: delay_msec
 *	I2C address: dev_addr
 *	Chip id of the sensor: chip_id
 *---------------------------------------------------------------------------*/
 #define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)

#define	I2C_BUFFER_LEN 28
#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */
periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;


struct bma2x2_t bma2x2;
/*----------------------------------------------------------------------------*
*  V_BMA2x2RESOLUTION_u8R used for selecting the accelerometer resolution
 *	12 bit
 *	14 bit
 *	10 bit
*----------------------------------------------------------------------------*/
extern u8 V_BMA2x2RESOLUTION_u8R;


void bmc156_nomad_init(periph_t i2c)
{
	s32 com_rslt = ERROR;
	u8 banwid = BMA2x2_INIT_VALUE;
    u8 bw_value_u8 = BMA2x2_INIT_VALUE;

    /* make sure the INT and RDY signals are setup */
    gpio_config_t accell_int_pin;
    pin_name_t accell_int_pin_name = ACCELL_INT;
    accell_int_pin.dir = INPUT;
    accell_int_pin.pull_mode = PP_NO_PULL;
    accell_int_pin.speed = SPEED_VERY_HIGH;
    gpio_init(accell_int_pin_name, &accell_int_pin);

    gpio_config_t mag_rdy_pin;
    pin_name_t mag_rdy_pin_name = MAG_READY;
    mag_rdy_pin.dir = INPUT;
    mag_rdy_pin.pull_mode = PP_NO_PULL;
    mag_rdy_pin.speed = SPEED_VERY_HIGH;
    gpio_init(mag_rdy_pin_name, &mag_rdy_pin);

/*	Note:
	* For the Suspend/Low power1 mode Idle time of
		at least 450us(micro seconds)
	* required for read/write operations*/
    bma2x2.bus_write = BMA2x2_I2C_bus_write;
    bma2x2.bus_read = BMA2x2_I2C_bus_read;
    bma2x2.dev_addr = BMA2x2_I2C_ADDR3; // BMC156 on nomad is strapped for 0x10
    bma2x2.delay_msec = BMA2x2_delay_msek;

    i2c_handle = i2c;
    bma2x2_init(&bma2x2);
    
    /*	For initialization it is required to set the mode of
 *	the sensor as "NORMAL"
 *	NORMAL mode is set from the register 0x11 and 0x12
 *	0x11 -> bit 5,6,7 -> set value as 0
 *	0x12 -> bit 5,6 -> set value as 0
 *	data acquisition/read/write is possible in this mode
 *	by using the below API able to set the power mode as NORMAL
 *	For the Normal/standby/Low power 2 mode Idle time
		of at least 2us(micro seconds)
 *	required for read/write operations*/
	/* Set the power mode as NORMAL*/
	com_rslt += bma2x2_set_power_mode(BMA2x2_MODE_NORMAL);
	
	/* This API used to Write the bandwidth of the sensor input
	value have to be given
	bandwidth is set from the register 0x10 bits from 1 to 4*/
	bw_value_u8 = 0x08;/* set bandwidth of 7.81Hz*/
	com_rslt += bma2x2_set_bw(bw_value_u8);

	/* This API used to read back the written value of bandwidth*/
	com_rslt += bma2x2_get_bw(&banwid);

}


static void int_to_buffer(char *buffer, int32_t n)
{
  buffer[0] = (n >> 24) & 0xFF;
  buffer[1] = (n >> 16) & 0xFF;
  buffer[2] = (n >> 8) & 0xFF;
  buffer[3] = n & 0xFF;
}

s32 bmc156_read_sensor(array_t *buffer_struct)

{
	/*Local variables for reading accel x, y and z data*/
	s16	accel_x_s16, accel_y_s16, accel_z_s16 = BMA2x2_INIT_VALUE;

	/* bma2x2acc_data structure used to read accel xyz data*/
	struct bma2x2_accel_data sample_xyz;
	/* bma2x2acc_data_temp structure used to read
		accel xyz and temperature data*/
	struct bma2x2_accel_data_temp sample_xyzt;
	/* status of communication*/
	s32 com_rslt = ERROR;
	/* Read the accel X data*/
	com_rslt += bma2x2_read_accel_x(&accel_x_s16);
	/* Read the accel Y data*/
	com_rslt += bma2x2_read_accel_y(&accel_y_s16);
	/* Read the accel Z data*/
	com_rslt += bma2x2_read_accel_z(&accel_z_s16);

	/* accessing the bma2x2acc_data parameter by using sample_xyz*/
	/* Read the accel XYZ data*/
	com_rslt += bma2x2_read_accel_xyz(&sample_xyz);
 char *buffer = (char *)buffer_struct->bytes; 
  
  int_to_buffer(&buffer[0],(int16_t)accel_x_s16);
  int_to_buffer(&buffer[2],(int16_t)accel_y_s16);
  int_to_buffer(&buffer[4],(int16_t)accel_z_s16);
  buffer_struct->sz = 6;
	/* accessing the bma2x2acc_data_temp parameter by using sample_xyzt*/
	/* Read the accel XYZT data*/
	com_rslt += bma2x2_read_accel_xyzt(&sample_xyzt);
    dbg_printf("Accelerometer: x=%d,y=%d z=%d\r\n",accel_x_s16,accel_y_s16,accel_z_s16);
	dbg_printf("Accelerometer w/temp: x=%d,y=%d z=%d t=%d\r\n",sample_xyzt.x,sample_xyzt.y,sample_xyzt.z, sample_xyzt.temp);
	return com_rslt;
}


 /*************************************************************************/
 /*	\Brief: The function is used as I2C bus write
 *	\Return : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register,
 *              will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
s8 BMA2x2_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
    int32_t iError = BMA2x2_INIT_VALUE;

    i2c_dest_addr.slave = dev_addr;
    i2c_dest_addr.reg = reg_addr;
    EOE(i2c_write(i2c_handle, i2c_dest_addr, cnt ,  (uint8_t *)&reg_data));
    return (int8_t)iError;
}

 /*   \Brief: The function is used as I2C bus read
 *    \Return : Status of the I2C read
 *    \param dev_addr : The device address of the sensor
 *    \param reg_addr : Address of the first register,
 *            will data is going to be read
 *    \param reg_data : This data read from the sensor,
 *            which is hold in an array
 *    \param cnt : The no of byte of data to be read
 */
s8 BMA2x2_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
    int32_t iError = BMA2x2_INIT_VALUE;
    uint8_t array[I2C_BUFFER_LEN] = {BMA2x2_INIT_VALUE};
    uint8_t stringpos = BMA2x2_INIT_VALUE;
    array[BMA2x2_INIT_VALUE] = reg_addr;

    i2c_dest_addr.slave = dev_addr;
    i2c_dest_addr.reg = reg_addr;
    EOE(i2c_read(i2c_handle, i2c_dest_addr, cnt, (uint8_t *)*&array));

    for (stringpos = BMA2x2_INIT_VALUE; stringpos < cnt; stringpos++) {
		*(reg_data + stringpos) = array[stringpos];
    }

    return (int8_t)iError;
}


/*	Brief : The delay routine
 *	\param : delay in ms
*/
void BMA2x2_delay_msek(u32 msek)
{
	/*Here you can write your own delay routine*/
}

