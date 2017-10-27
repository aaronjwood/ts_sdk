/****************************************************************************
* Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
*
* File : bme280.h
*
* Date : 2016/07/04
*
* Revision : 2.0.5(Pressure and Temperature compensation code revision is 1.1
*               and Humidity compensation code revision is 1.0)
*
* Usage: Sensor Driver for BME280 sensor
*
****************************************************************************
*
* \section License
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
*   Redistributions in binary form must reproduce the above copyright
*   notice, this list of conditions and the following disclaimer in the
*   documentation and/or other materials provided with the distribution.
*
*   Neither the name of the copyright holder nor the names of the
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
* OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*
* The information provided is believed to be accurate and reliable.
* The copyright holder assumes no responsibility
* for the consequences of use
* of such information nor for any infringement of patents or
* other rights of third parties which may result from its use.
* No license is granted by implication or otherwise under any patent or
* patent rights of the copyright holder.
**************************************************************************/
/*! \file bme280.h
    \brief BME280 Sensor Driver Support Header File */
#ifndef __BME280_H__
#define __BME280_H__

#include <stdint.h>

/*unsigned integer types*/
typedef	uint8_t u8;/**< used for unsigned 8bit */
typedef	uint16_t u16;/**< used for unsigned 16bit */
typedef	uint32_t u32;/**< used for unsigned 32bit */
typedef	uint64_t u64;/**< used for unsigned 64bit */

/*signed integer types*/
typedef int8_t s8;/**< used for signed 8bit */
typedef	int16_t s16;/**< used for signed 16bit */
typedef	int32_t s32;/**< used for signed 32bit */
typedef	int64_t s64;/**< used for signed 64bit */

#define BME280_BUS_WRITE_FUNC(device_addr, register_addr,\
register_data, wr_len) bus_write(device_addr, register_addr,\
		register_data, wr_len)

#define BME280_BUS_READ_FUNC(device_addr, register_addr,\
		register_data, rd_len)bus_read(device_addr, register_addr,\
		register_data, rd_len)

/***************************************************************/
/**\name	GET AND SET BITSLICE FUNCTIONS       */
/***************************************************************/
#define BME280_DELAY_FUNC(delay_in_msec)\
		delay_func(delay_in_msec)

#define BME280_GET_BITSLICE(regvar, bitname)\
		((regvar & bitname##__MSK) >> bitname##__POS)

#define BME280_SET_BITSLICE(regvar, bitname, val)\
((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

/***************************************************************/
/**\name	COMMON USED CONSTANTS      */
/***************************************************************/
/* Constants */
#define BME280_NULL                          (0)
#define BME280_RETURN_FUNCTION_TYPE          s8
/* shift definitions*/
#define BME280_SHIFT_BIT_POSITION_BY_01_BIT			(1)
#define BME280_SHIFT_BIT_POSITION_BY_02_BITS			(2)
#define BME280_SHIFT_BIT_POSITION_BY_03_BITS			(3)
#define BME280_SHIFT_BIT_POSITION_BY_04_BITS			(4)
#define BME280_SHIFT_BIT_POSITION_BY_07_BITS			(7)
#define BME280_SHIFT_BIT_POSITION_BY_08_BITS			(8)
#define BME280_SHIFT_BIT_POSITION_BY_10_BITS			(10)
#define BME280_SHIFT_BIT_POSITION_BY_11_BITS			(11)
#define BME280_SHIFT_BIT_POSITION_BY_12_BITS			(12)
#define BME280_SHIFT_BIT_POSITION_BY_13_BITS			(13)
#define BME280_SHIFT_BIT_POSITION_BY_14_BITS			(14)
#define BME280_SHIFT_BIT_POSITION_BY_15_BITS			(15)
#define BME280_SHIFT_BIT_POSITION_BY_16_BITS			(16)
#define BME280_SHIFT_BIT_POSITION_BY_17_BITS			(17)
#define BME280_SHIFT_BIT_POSITION_BY_18_BITS			(18)
#define BME280_SHIFT_BIT_POSITION_BY_19_BITS			(19)
#define BME280_SHIFT_BIT_POSITION_BY_20_BITS			(20)
#define BME280_SHIFT_BIT_POSITION_BY_25_BITS			(25)
#define BME280_SHIFT_BIT_POSITION_BY_31_BITS			(31)
#define BME280_SHIFT_BIT_POSITION_BY_33_BITS			(33)
#define BME280_SHIFT_BIT_POSITION_BY_35_BITS			(35)
#define BME280_SHIFT_BIT_POSITION_BY_47_BITS			(47)

/* numeric definitions */
#define	BME280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH	    (26)
#define	BME280_HUMIDITY_CALIB_DATA_LENGTH		(7)
#define	BME280_GEN_READ_WRITE_DATA_LENGTH		(1)
#define	BME280_HUMIDITY_DATA_LENGTH			(2)
#define	BME280_TEMPERATURE_DATA_LENGTH			(3)
#define	BME280_PRESSURE_DATA_LENGTH			(3)
#define	BME280_ALL_DATA_FRAME_LENGTH			(8)
#define	BME280_INIT_VALUE				(0)
#define	BME280_CHIP_ID_READ_COUNT			(5)
#define	BME280_INVALID_DATA				(0)

/****************************************************/
/**\name	ERROR CODE DEFINITIONS  */
/***************************************************/
#define E_SUCCESS		((u8)0)
#define E_BME280_NULL_PTR       ((s8)-127)
#define E_BME280_OUT_OF_RANGE   ((s8)-2)
#define E_ERROR			((s8)-1)
#define BME280_CHIP_ID_READ_FAIL	((s8)-1)
#define BME280_CHIP_ID_READ_SUCCESS	((u8)0)

/****************************************************/
/**\name	CHIP ID DEFINITIONS  */
/***************************************************/
#define BME280_CHIP_ID			(0x60)
#define BME280_I2C_ADDRESS2		(0x77)
/****************************************************/
/**\name	POWER MODE DEFINITIONS  */
/***************************************************/
/* Sensor Specific constants */
#define BME280_SLEEP_MODE                    (0x00)
#define BME280_FORCED_MODE                   (0x01)
#define BME280_NORMAL_MODE                   (0x03)
#define BME280_SOFT_RESET_CODE               (0xB6)
/****************************************************/
/**\name	STANDBY DEFINITIONS  */
/***************************************************/
#define BME280_STANDBY_TIME_1_MS		(0x00)
#define BME280_STANDBY_TIME_63_MS		(0x01)
#define BME280_STANDBY_TIME_125_MS		(0x02)
#define BME280_STANDBY_TIME_250_MS		(0x03)
#define BME280_STANDBY_TIME_500_MS		(0x04)
#define BME280_STANDBY_TIME_1000_MS		(0x05)
#define BME280_STANDBY_TIME_10_MS		(0x06)
#define BME280_STANDBY_TIME_20_MS		(0x07)
/****************************************************/
/**\name	OVER SAMPLING DEFINITIONS  */
/***************************************************/
#define BME280_OVERSAMP_SKIPPED          (0x00)
#define BME280_OVERSAMP_1X               (0x01)
#define BME280_OVERSAMP_2X               (0x02)
#define BME280_OVERSAMP_4X               (0x03)
#define BME280_OVERSAMP_8X               (0x04)
#define BME280_OVERSAMP_16X              (0x05)
#define BME280_STANDARD_OVERSAMP_HUMIDITY	BME280_OVERSAMP_1X
/****************************************************/
/**\name	DEFINITIONS FOR ARRAY SIZE OF DATA   */
/***************************************************/
#define	BME280_DATA_FRAME_SIZE			(8)
/**< data frames includes temperature,
pressure and humidity*/
#define	BME280_CALIB_DATA_SIZE			(26)

#define	BME280_DATA_FRAME_PRESSURE_MSB_BYTE	(0)
#define	BME280_DATA_FRAME_PRESSURE_LSB_BYTE	(1)
#define	BME280_DATA_FRAME_PRESSURE_XLSB_BYTE	(2)
#define	BME280_DATA_FRAME_TEMPERATURE_MSB_BYTE	(3)
#define	BME280_DATA_FRAME_TEMPERATURE_LSB_BYTE	(4)
#define	BME280_DATA_FRAME_TEMPERATURE_XLSB_BYTE	(5)
#define	BME280_DATA_FRAME_HUMIDITY_MSB_BYTE	(6)
#define	BME280_DATA_FRAME_HUMIDITY_LSB_BYTE	(7)
/****************************************************/
/**\name	ARRAY PARAMETER FOR CALIBRATION     */
/***************************************************/
#define	BME280_TEMPERATURE_CALIB_DIG_T1_LSB		(0)
#define	BME280_TEMPERATURE_CALIB_DIG_T1_MSB		(1)
#define	BME280_TEMPERATURE_CALIB_DIG_T2_LSB		(2)
#define	BME280_TEMPERATURE_CALIB_DIG_T2_MSB		(3)
#define	BME280_TEMPERATURE_CALIB_DIG_T3_LSB		(4)
#define	BME280_TEMPERATURE_CALIB_DIG_T3_MSB		(5)
#define	BME280_PRESSURE_CALIB_DIG_P1_LSB       (6)
#define	BME280_PRESSURE_CALIB_DIG_P1_MSB       (7)
#define	BME280_PRESSURE_CALIB_DIG_P2_LSB       (8)
#define	BME280_PRESSURE_CALIB_DIG_P2_MSB       (9)
#define	BME280_PRESSURE_CALIB_DIG_P3_LSB       (10)
#define	BME280_PRESSURE_CALIB_DIG_P3_MSB       (11)
#define	BME280_PRESSURE_CALIB_DIG_P4_LSB       (12)
#define	BME280_PRESSURE_CALIB_DIG_P4_MSB       (13)
#define	BME280_PRESSURE_CALIB_DIG_P5_LSB       (14)
#define	BME280_PRESSURE_CALIB_DIG_P5_MSB       (15)
#define	BME280_PRESSURE_CALIB_DIG_P6_LSB       (16)
#define	BME280_PRESSURE_CALIB_DIG_P6_MSB       (17)
#define	BME280_PRESSURE_CALIB_DIG_P7_LSB       (18)
#define	BME280_PRESSURE_CALIB_DIG_P7_MSB       (19)
#define	BME280_PRESSURE_CALIB_DIG_P8_LSB       (20)
#define	BME280_PRESSURE_CALIB_DIG_P8_MSB       (21)
#define	BME280_PRESSURE_CALIB_DIG_P9_LSB       (22)
#define	BME280_PRESSURE_CALIB_DIG_P9_MSB       (23)
#define	BME280_HUMIDITY_CALIB_DIG_H1           (25)
#define	BME280_HUMIDITY_CALIB_DIG_H2_LSB		(0)
#define	BME280_HUMIDITY_CALIB_DIG_H2_MSB		(1)
#define	BME280_HUMIDITY_CALIB_DIG_H3			(2)
#define	BME280_HUMIDITY_CALIB_DIG_H4_MSB		(3)
#define	BME280_HUMIDITY_CALIB_DIG_H4_LSB		(4)
#define	BME280_HUMIDITY_CALIB_DIG_H5_MSB		(5)
#define	BME280_HUMIDITY_CALIB_DIG_H6			(6)
#define	BME280_MASK_DIG_H4		(0x0F)
/****************************************************/
/**\name	CALIBRATION REGISTER ADDRESS DEFINITIONS  */
/***************************************************/
/*calibration parameters */
#define BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG             (0x88)
#define BME280_TEMPERATURE_CALIB_DIG_T1_MSB_REG             (0x89)
#define BME280_TEMPERATURE_CALIB_DIG_T2_LSB_REG             (0x8A)
#define BME280_TEMPERATURE_CALIB_DIG_T2_MSB_REG             (0x8B)
#define BME280_TEMPERATURE_CALIB_DIG_T3_LSB_REG             (0x8C)
#define BME280_TEMPERATURE_CALIB_DIG_T3_MSB_REG             (0x8D)
#define BME280_PRESSURE_CALIB_DIG_P1_LSB_REG                (0x8E)
#define BME280_PRESSURE_CALIB_DIG_P1_MSB_REG                (0x8F)
#define BME280_PRESSURE_CALIB_DIG_P2_LSB_REG                (0x90)
#define BME280_PRESSURE_CALIB_DIG_P2_MSB_REG                (0x91)
#define BME280_PRESSURE_CALIB_DIG_P3_LSB_REG                (0x92)
#define BME280_PRESSURE_CALIB_DIG_P3_MSB_REG                (0x93)
#define BME280_PRESSURE_CALIB_DIG_P4_LSB_REG                (0x94)
#define BME280_PRESSURE_CALIB_DIG_P4_MSB_REG                (0x95)
#define BME280_PRESSURE_CALIB_DIG_P5_LSB_REG                (0x96)
#define BME280_PRESSURE_CALIB_DIG_P5_MSB_REG                (0x97)
#define BME280_PRESSURE_CALIB_DIG_P6_LSB_REG                (0x98)
#define BME280_PRESSURE_CALIB_DIG_P6_MSB_REG                (0x99)
#define BME280_PRESSURE_CALIB_DIG_P7_LSB_REG                (0x9A)
#define BME280_PRESSURE_CALIB_DIG_P7_MSB_REG                (0x9B)
#define BME280_PRESSURE_CALIB_DIG_P8_LSB_REG                (0x9C)
#define BME280_PRESSURE_CALIB_DIG_P8_MSB_REG                (0x9D)
#define BME280_PRESSURE_CALIB_DIG_P9_LSB_REG                (0x9E)
#define BME280_PRESSURE_CALIB_DIG_P9_MSB_REG                (0x9F)

#define BME280_HUMIDITY_CALIB_DIG_H1_REG                    (0xA1)

#define BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG                (0xE1)
#define BME280_HUMIDITY_CALIB_DIG_H2_MSB_REG                (0xE2)
#define BME280_HUMIDITY_CALIB_DIG_H3_REG                    (0xE3)
#define BME280_HUMIDITY_CALIB_DIG_H4_MSB_REG                (0xE4)
#define BME280_HUMIDITY_CALIB_DIG_H4_LSB_REG                (0xE5)
#define BME280_HUMIDITY_CALIB_DIG_H5_MSB_REG                (0xE6)
#define BME280_HUMIDITY_CALIB_DIG_H6_REG                    (0xE7)
/****************************************************/
/**\name	REGISTER ADDRESS DEFINITIONS  */
/***************************************************/
#define BME280_CHIP_ID_REG                   (0xD0)  /*Chip ID Register */
#define BME280_RST_REG                       (0xE0)  /*Softreset Register */
#define BME280_STAT_REG                      (0xF3)  /*Status Register */
#define BME280_CTRL_MEAS_REG                 (0xF4)  /*Ctrl Measure Register */
#define BME280_CTRL_HUMIDITY_REG             (0xF2)  /*Ctrl Humidity Register*/
#define BME280_CONFIG_REG                    (0xF5)  /*Configuration Register */
#define BME280_PRESSURE_MSB_REG              (0xF7)  /*Pressure MSB Register */
#define BME280_PRESSURE_LSB_REG              (0xF8)  /*Pressure LSB Register */
#define BME280_PRESSURE_XLSB_REG             (0xF9)  /*Pressure XLSB Register */
#define BME280_TEMPERATURE_MSB_REG           (0xFA)  /*Temperature MSB Reg */
#define BME280_TEMPERATURE_LSB_REG           (0xFB)  /*Temperature LSB Reg */
#define BME280_TEMPERATURE_XLSB_REG          (0xFC)  /*Temperature XLSB Reg */
#define BME280_HUMIDITY_MSB_REG              (0xFD)  /*Humidity MSB Reg */
#define BME280_HUMIDITY_LSB_REG              (0xFE)  /*Humidity LSB Reg */
/****************************************************/
/**\name        BIT MASK, LENGTH AND POSITION DEFINITIONS
FOR TEMPERATURE OVERSAMPLING  */
/***************************************************/
/* Control Measurement Register */
#define BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__POS             (5)
#define BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__MSK             (0xE0)
#define BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__LEN             (3)
#define BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__REG             \
(BME280_CTRL_MEAS_REG)
/****************************************************/
/**\name        BIT MASK, LENGTH AND POSITION DEFINITIONS
FOR PRESSURE OVERSAMPLING  */
/***************************************************/
#define BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__POS             (2)
#define BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__MSK             (0x1C)
#define BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__LEN             (3)
#define BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__REG             \
(BME280_CTRL_MEAS_REG)
/****************************************************/
/**\name        BIT MASK, LENGTH AND POSITION DEFINITIONS
FOR POWER MODE  */
/***************************************************/
#define BME280_CTRL_MEAS_REG_POWER_MODE__POS              (0)
#define BME280_CTRL_MEAS_REG_POWER_MODE__MSK              (0x03)
#define BME280_CTRL_MEAS_REG_POWER_MODE__LEN              (2)
#define BME280_CTRL_MEAS_REG_POWER_MODE__REG              \
(BME280_CTRL_MEAS_REG)
/****************************************************/
/**\name        BIT MASK, LENGTH AND POSITION DEFINITIONS
FOR HUMIDITY OVERSAMPLING  */
/***************************************************/
#define BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__POS             (0)
#define BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__MSK             (0x07)
#define BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__LEN             (3)
#define BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__REG            \
(BME280_CTRL_HUMIDITY_REG)
/****************************************************/
/**\name        BIT MASK, LENGTH AND POSITION DEFINITIONS
FOR STANDBY TIME  */
/***************************************************/
/* Configuration Register */
#define BME280_CONFIG_REG_TSB__POS                 (5)
#define BME280_CONFIG_REG_TSB__MSK                 (0xE0)
#define BME280_CONFIG_REG_TSB__LEN                 (3)
#define BME280_CONFIG_REG_TSB__REG                 (BME280_CONFIG_REG)
/****************************************************/
/**\name	BUS READ AND WRITE FUNCTION POINTERS */
/***************************************************/
#define BME280_WR_FUNC_PTR\
		s8 (*bus_write)(u8, u8,\
		u8 *, u8)

#define BME280_RD_FUNC_PTR\
		s8 (*bus_read)(u8, u8,\
		u8 *, u8)

#define BME280_MDELAY_DATA_TYPE u32

#define	BME280_3MS_DELAY	(3)
#define BME280_REGISTER_READ_DELAY (1)
/**************************************************************/
/**\name	STRUCTURE DEFINITIONS                         */
/**************************************************************/
/*!
 * @brief This structure holds all device specific calibration parameters
 */
struct bme280_calibration_param_t {
	u16 dig_T1;/**<calibration T1 data*/
	s16 dig_T2;/**<calibration T2 data*/
	s16 dig_T3;/**<calibration T3 data*/
	u16 dig_P1;/**<calibration P1 data*/
	s16 dig_P2;/**<calibration P2 data*/
	s16 dig_P3;/**<calibration P3 data*/
	s16 dig_P4;/**<calibration P4 data*/
	s16 dig_P5;/**<calibration P5 data*/
	s16 dig_P6;/**<calibration P6 data*/
	s16 dig_P7;/**<calibration P7 data*/
	s16 dig_P8;/**<calibration P8 data*/
	s16 dig_P9;/**<calibration P9 data*/

	u8  dig_H1;/**<calibration H1 data*/
	s16 dig_H2;/**<calibration H2 data*/
	u8  dig_H3;/**<calibration H3 data*/
	s16 dig_H4;/**<calibration H4 data*/
	s16 dig_H5;/**<calibration H5 data*/
	s8  dig_H6;/**<calibration H6 data*/

	s32 t_fine;/**<calibration T_FINE data*/
};
/*!
 * @brief This structure holds BME280 initialization parameters
 */
struct bme280_t {
	struct bme280_calibration_param_t cal_param;
	/**< calibration parameters*/

	u8 chip_id;/**< chip id of the sensor*/
	u8 dev_addr;/**< device address of the sensor*/

	u8 oversamp_temperature;/**< temperature over sampling*/
	u8 oversamp_pressure;/**< pressure over sampling*/
	u8 oversamp_humidity;/**< humidity over sampling*/
	u8 ctrl_hum_reg;/**< status of control humidity register*/
	u8 ctrl_meas_reg;/**< status of control measurement register*/
	u8 config_reg;/**< status of configuration register*/

	BME280_WR_FUNC_PTR;/**< bus write function pointer*/
	BME280_RD_FUNC_PTR;/**< bus read function pointer*/
	void (*delay_msec)(BME280_MDELAY_DATA_TYPE);/**< delay function pointer*/
};
/**************************************************************/
/**\name	FUNCTION DECLARATIONS                         */
/**************************************************************/
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION                       */
/**************************************************************/
/*!
 *	@brief This function is used for initialize
 *	the bus read and bus write functions
 *  and assign the chip id and I2C address of the BME280 sensor
 *	chip id is read in the register 0xD0 bit from 0 to 7
 *
 *	 @param bme280 structure pointer.
 *
 *	@note While changing the parameter of the bme280_t
 *	@note consider the following point:
 *	Changing the reference value of the parameter
 *	will changes the local copy or local reference
 *	make sure your changes will not
 *	affect the reference value of the parameter
 *	(Better case don't change the reference value of the parameter)
 *
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_init(struct bme280_t *bme280);
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION TRUE TEMPERATURE */
/**************************************************************/
/*!
 * @brief Reads actual temperature from uncompensated temperature
 * @note Returns the value in 0.01 degree Centigrade
 * Output value of "5123" equals 51.23 DegC.
 *
 *
 *
 *  @param  v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *  @return Returns the actual temperature
 *
*/
s32 bme280_compensate_temperature_int32(s32 v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION TRUE PRESSURE */
/**************************************************************/
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns the value in Pascal(Pa)
 * Output value of "96386" equals 96386 Pa =
 * 963.86 hPa = 963.86 millibar
 *
 *
 *
 *  @param v_uncomp_pressure_s32 : value of uncompensated pressure
 *
 *
 *
 *  @return Return the actual pressure output as u32
 *
*/
u32 bme280_compensate_pressure_int32(s32 v_uncomp_pressure_s32);
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION RELATIVE HUMIDITY */
/**************************************************************/
/*!
 * @brief Reads actual humidity from uncompensated humidity
 * @note Returns the value in %rH as unsigned 32bit integer
 * in Q22.10 format(22 integer 10 fractional bits).
 * @note An output value of 42313
 * represents 42313 / 1024 = 41.321 %rH
 *
 *
 *
 *  @param  v_uncomp_humidity_s32: value of uncompensated humidity
 *
 *  @return Return the actual relative humidity output as u32
 *
*/
u32 bme280_compensate_humidity_int32(s32 v_uncomp_humidity_s32);
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION UNCOMPENSATED PRESSURE,
 TEMPERATURE AND HUMIDITY */
/**************************************************************/
/*!
 * @brief This API used to read uncompensated
 * pressure,temperature and humidity
 *
 *
 *
 *
 *  @param  v_uncomp_pressure_s32: The value of uncompensated pressure.
 *  @param  v_uncomp_temperature_s32: The value of uncompensated temperature
 *  @param  v_uncomp_humidity_s32: The value of uncompensated humidity.
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_pressure_temperature_humidity(
s32 *v_uncomp_pressure_s32,
s32 *v_uncomp_temperature_s32, s32 *v_uncomp_humidity_s32);
/**************************************************************/
/**\name	FUNCTION FOR TRUE UNCOMPENSATED PRESSURE,
 TEMPERATURE AND HUMIDITY */
/**************************************************************/
/*!
 * @brief This API used to read true pressure, temperature and humidity
 *
 *
 *
 *
 *	@param  v_pressure_u32 : The value of compensated pressure.
 *	@param  v_temperature_s32 : The value of compensated temperature.
 *	@param  v_humidity_u32 : The value of compensated humidity.
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_pressure_temperature_humidity(
u32 *v_pressure_u32, s32 *v_temperature_s32, u32 *v_humidity_u32);
/**************************************************************/
/**\name	FUNCTION FOR CALIBRATION */
/**************************************************************/
/*!
 *	@brief This API is used to
 *	calibration parameters used for calculation in the registers
 *
 *  parameter | Register address |   bit
 *------------|------------------|----------------
 *	dig_T1    |  0x88 and 0x89   | from 0 : 7 to 8: 15
 *	dig_T2    |  0x8A and 0x8B   | from 0 : 7 to 8: 15
 *	dig_T3    |  0x8C and 0x8D   | from 0 : 7 to 8: 15
 *	dig_P1    |  0x8E and 0x8F   | from 0 : 7 to 8: 15
 *	dig_P2    |  0x90 and 0x91   | from 0 : 7 to 8: 15
 *	dig_P3    |  0x92 and 0x93   | from 0 : 7 to 8: 15
 *	dig_P4    |  0x94 and 0x95   | from 0 : 7 to 8: 15
 *	dig_P5    |  0x96 and 0x97   | from 0 : 7 to 8: 15
 *	dig_P6    |  0x98 and 0x99   | from 0 : 7 to 8: 15
 *	dig_P7    |  0x9A and 0x9B   | from 0 : 7 to 8: 15
 *	dig_P8    |  0x9C and 0x9D   | from 0 : 7 to 8: 15
 *	dig_P9    |  0x9E and 0x9F   | from 0 : 7 to 8: 15
 *	dig_H1    |         0xA1     | from 0 to 7
 *	dig_H2    |  0xE1 and 0xE2   | from 0 : 7 to 8: 15
 *	dig_H3    |         0xE3     | from 0 to 7
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_calib_param(void);
/**************************************************************/
/**\name        FUNCTION FOR TEMPERATURE OVER SAMPLING */
/**************************************************************/
/*!
 *      @brief This API is used to get
 *      the temperature oversampling setting in the register 0xF4
 *      bits from 5 to 7
 *
 *      value               |   Temperature oversampling
 * ---------------------|---------------------------------
 *      0x00                | Skipped
 *      0x01                | BME280_OVERSAMP_1X
 *      0x02                | BME280_OVERSAMP_2X
 *      0x03                | BME280_OVERSAMP_4X
 *      0x04                | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of temperature over sampling
 *
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_temperature(
u8 *v_value_u8);
/*!
 *      @brief This API is used to set
 *      the temperature oversampling setting in the register 0xF4
 *      bits from 5 to 7
 *
 *      value               |   Temperature oversampling
 * ---------------------|---------------------------------
 *      0x00                | Skipped
 *      0x01                | BME280_OVERSAMP_1X
 *      0x02                | BME280_OVERSAMP_2X
 *      0x03                | BME280_OVERSAMP_4X
 *      0x04                | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of temperature over sampling
 *
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_temperature(
u8 v_value_u8);
/**************************************************************/
/**\name        FUNCTION FOR PRESSURE OVER SAMPLING */
/**************************************************************/
/*!
 *      @brief This API is used to get
 *      the pressure oversampling setting in the register 0xF4
 *      bits from 2 to 4
 *
 *      value              | Pressure oversampling
 * --------------------|--------------------------
 *      0x00               | Skipped
 *      0x01               | BME280_OVERSAMP_1X
 *      0x02               | BME280_OVERSAMP_2X
 *      0x03               | BME280_OVERSAMP_4X
 *      0x04               | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07 | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of pressure oversampling
 *
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_pressure(
u8 *v_value_u8);
/*!
 *      @brief This API is used to set
 *      the pressure oversampling setting in the register 0xF4
 *      bits from 2 to 4
 *
 *      value              | Pressure oversampling
 * --------------------|--------------------------
 *      0x00               | Skipped
 *      0x01               | BME280_OVERSAMP_1X
 *      0x02               | BME280_OVERSAMP_2X
 *      0x03               | BME280_OVERSAMP_4X
 *      0x04               | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07 | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of pressure oversampling
 *
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_pressure(
u8 v_value_u8);

/**************************************************************/
/**\name        FUNCTION FOR HUMIDITY OVER SAMPLING */
/**************************************************************/
/*!
 *      @brief This API is used to get
 *      the humidity oversampling setting in the register 0xF2
 *      bits from 0 to 2
 *
 *      value               | Humidity oversampling
 * ---------------------|-------------------------
 *      0x00                | Skipped
 *      0x01                | BME280_OVERSAMP_1X
 *      0x02                | BME280_OVERSAMP_2X
 *      0x03                | BME280_OVERSAMP_4X
 *      0x04                | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of humidity over sampling
 *
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_humidity(u8 *v_value_u8);
/*!
 *      @brief This API is used to set
 *      the humidity oversampling setting in the register 0xF2
 *      bits from 0 to 2
 *
 *      value               | Humidity oversampling
 * ---------------------|-------------------------
 *      0x00                | Skipped
 *      0x01                | BME280_OVERSAMP_1X
 *      0x02                | BME280_OVERSAMP_2X
 *      0x03                | BME280_OVERSAMP_4X
 *      0x04                | BME280_OVERSAMP_8X
 *      0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of humidity over sampling
 *
 *
 *
 * @note The "BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 * register sets the humidity
 * data acquisition options of the device.
 * @note changes to this registers only become
 * effective after a write operation to
 * "BME280_CTRL_MEAS_REG" register.
 * @note In the code automated reading and writing of
 *      "BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 * @note register first set the
 * "BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 *  and then read and write
 *  the "BME280_CTRL_MEAS_REG" register in the function.
 *
 *
 *      @return results of bus communication function
 *      @retval 0 -> Success
 *      @retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_humidity(
u8 v_value_u8);
/**************************************************************/
/**\name	FUNCTION FOR POWER MODE*/
/**************************************************************/
/*!
 *	@brief This API used to get the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode
 *  value           |    mode
 * -----------------|------------------
 *	0x00            | BME280_SLEEP_MODE
 *	0x01 and 0x02   | BME280_FORCED_MODE
 *	0x03            | BME280_NORMAL_MODE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_power_mode(u8 *v_power_mode_u8);
/*!
 *	@brief This API used to set the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode
 *  value           |    mode
 * -----------------|------------------
 *	0x00            | BME280_SLEEP_MODE
 *	0x01 and 0x02   | BME280_FORCED_MODE
 *	0x03            | BME280_NORMAL_MODE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_power_mode(u8 v_power_mode_u8);
/**************************************************************/
/**\name	FUNCTION FOR SOFT RESET*/
/**************************************************************/
/*!
 * @brief Used to reset the sensor
 * The value 0xB6 is written to the 0xE0
 * register the device is reset using the
 * complete power-on-reset procedure.
 * @note Soft reset can be easily set using bme280_set_softreset().
 * @note Usage Hint : bme280_set_softreset()
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_soft_rst(void);
/**************************************************************/
/**\name	FUNCTION FOR STANDBY DURATION*/
/**************************************************************/
/*!
 *	@brief This API used to Read the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *
 *	@param v_standby_durn_u8 : The value of standby duration time value.
 *  value       | standby duration
 * -------------|-----------------------
 *    0x00      | BME280_STANDBY_TIME_1_MS
 *    0x01      | BME280_STANDBY_TIME_63_MS
 *    0x02      | BME280_STANDBY_TIME_125_MS
 *    0x03      | BME280_STANDBY_TIME_250_MS
 *    0x04      | BME280_STANDBY_TIME_500_MS
 *    0x05      | BME280_STANDBY_TIME_1000_MS
 *    0x06      | BME280_STANDBY_TIME_2000_MS
 *    0x07      | BME280_STANDBY_TIME_4000_MS
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_standby_durn(u8 *v_standby_durn_u8);
/*!
 *	@brief This API used to write the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *
 *	@param v_standby_durn_u8 : The value of standby duration time value.
 *  value       | standby duration
 * -------------|-----------------------
 *    0x00      | BME280_STANDBY_TIME_1_MS
 *    0x01      | BME280_STANDBY_TIME_63_MS
 *    0x02      | BME280_STANDBY_TIME_125_MS
 *    0x03      | BME280_STANDBY_TIME_250_MS
 *    0x04      | BME280_STANDBY_TIME_500_MS
 *    0x05      | BME280_STANDBY_TIME_1000_MS
 *    0x06      | BME280_STANDBY_TIME_2000_MS
 *    0x07      | BME280_STANDBY_TIME_4000_MS
 *
 *	@note Normal mode comprises an automated perpetual
 *	cycling between an (active)
 *	Measurement period and an (inactive) standby period.
 *	@note The standby time is determined by
 *	the contents of the register t_sb.
 *	Standby time can be set using BME280_STANDBY_TIME_125_MS.
 *
 *	@note Usage Hint : bme280_set_standby_durn(BME280_STANDBY_TIME_125_MS)
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_standby_durn(u8 v_standby_durn_u8);
/**************************************************************/
/**\name	FUNCTION FOR COMMON READ AND WRITE */
/**************************************************************/
/*!
 * @brief
 *	This API write the data to
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BME280_RETURN_FUNCTION_TYPE bme280_write_register(u8 v_addr_u8,
u8 *v_data_u8, u8 v_len_u8);
/*!
 * @brief
 *	This API reads the data from
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BME280_RETURN_FUNCTION_TYPE bme280_read_register(u8 v_addr_u8,
u8 *v_data_u8, u8 v_len_u8);
#endif
