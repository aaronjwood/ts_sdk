/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/* Sensor module for the BMP180 temperature and pressure sensor */
#include <stdbool.h>
#include "i2c_hal.h"
#include "sensor_interface.h"
#include "board_interface.h"
#include "sys.h"
#include "dbg.h"
#include "cJSON.h"
#include <string.h>
#include "cloud_comm.h"

#ifdef MQTT_PROTOCOL
#define cJSON_add_number(cJSON_obj, name, num)	do { \
	cJSON *number = cJSON_CreateNumber(num); \
	if (number == NULL) \
		fatal_err("Failed to create JSON number (%d)\n", __LINE__); \
	cJSON_AddItemToObject(cJSON_obj, name, number); \
} while(0)

#define cJSON_add_string(cJSON_obj, name, str) do { \
	cJSON *string = cJSON_CreateString(str); \
	if (string == NULL) \
		fatal_err("Failed to create JSON string (%d)\n", __LINE__); \
	cJSON_AddItemToObject(cJSON_obj, name, string); \
} while(0)
#endif

/* Exit On Error (EOE) macro */
#define EOE(func) \
	do { \
		if (!(func)) \
			return false; \
	} while (0)

periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;
float temperature_val;
float pressure_val;

#define NUM_SENSORS	1
#define BMP180_ADDR	0x77	/* I2C device address for BMP180 */

/* Registers and data lengths internal to the sensor */
#define HEADER_SZ	0x01	/* Header size representing the length */
#define CALIB_ADDR	0xaa	/* Address of calib table in device */
#define CALIB_SZ	22	/* Calibration table is 22 bytes long */
#define MEASURE_CTL	0xf4	/* Measurement control register */
#define OUT_MSB		0xf6	/* Sensor output register */
#define TEMP_SZ		2	/* Size of temperature reading in bytes */
#define PRES_SZ		3	/* Size of pressure reading in bytes */
#define TEMP_CTL	0x2e	/* Temperature control register */
#define PRES_CTL	0x34	/* Pressure control register */

bool si_init(void)
{
	uint32_t timeout_ms = 0;
#ifdef SIMULATE_SENSOR
	return true;
#endif
	/* Initialize the I2C bus on the processor */
	i2c_handle =  i2c_init(I2C_SCL, I2C_SDA, timeout_ms);

	if (i2c_handle == NO_PERIPH)
		return false;

	return true;
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (!data || !data->bytes)
		return false;
	if (idx > NUM_SENSORS - 1)
		return false;
	if (max_sz < CALIB_SZ)
		return false;
	data->sz = CALIB_SZ;
#ifdef SIMULATE_SENSOR
	for (uint8_t i = 0; i < data->sz; i++)
		data->bytes[i] = i;
	return true;
#endif
	i2c_dest_addr.slave = BMP180_ADDR;
	i2c_dest_addr.reg = CALIB_ADDR;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz,  data->bytes));
	for (int i = 0; i < CALIB_SZ ; i++)
		dbg_printf("After calibration data[%d] = %x\n", i, \
							data->bytes[i]);
	return true;
}

bool si_sleep(void)
{
	/* BMP180 does not have a sleep sequence */
	return true;
}

bool si_wakeup(void)
{
	/* BMP180 does not have a wakeup sequence */
	return true;
}

bool si_read_data(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (idx > NUM_SENSORS - 1)
		return false;
	if (!data || !data->bytes)
		return false;
	/* Read temperature and pressure off the BMP180 sensor */
	uint8_t temp_cmd = TEMP_CTL;
	uint8_t pres_cmd = PRES_CTL;
	if (max_sz < (PRES_SZ + TEMP_SZ))
		return false;
	data->sz = PRES_SZ + TEMP_SZ;
#ifdef SIMULATE_SENSOR
	for (uint8_t i = 0; i < data->sz; i++)
		data->bytes[i] = i;
	temperature_val = *((data->bytes)+1) << 8 | *(data->bytes);
	pressure_val = *((data->bytes) + TEMP_SZ) << 16 | \
			*((data->bytes) + TEMP_SZ + 1) << 8 | \
			*((data->bytes) + TEMP_SZ + 2);
	return true;
#endif
	i2c_dest_addr.slave = BMP180_ADDR;
	i2c_dest_addr.reg = MEASURE_CTL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &temp_cmd));
	sys_delay(5);

	i2c_dest_addr.reg = OUT_MSB;
	data->sz = TEMP_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz, data->bytes));
	dbg_printf("Apps-bmp180: Temperature Value = %2x%2x\n",\
					*(data->bytes), *((data->bytes)+1));
	temperature_val = *((data->bytes)+1) << 8 | *(data->bytes);
	i2c_dest_addr.reg = MEASURE_CTL;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1 ,  &pres_cmd));
	sys_delay(5);

	data->sz = PRES_SZ;
	EOE(i2c_read(i2c_handle, i2c_dest_addr, data->sz,\
					 ((data->bytes)+TEMP_SZ)));
	dbg_printf("Apps-bmp180: Pressure Value = %2x%2x%2x\n",\
					*((data->bytes) + TEMP_SZ), \
					*((data->bytes) + TEMP_SZ + 1),	\
					*((data->bytes) + TEMP_SZ + 2));
	pressure_val = *((data->bytes) + TEMP_SZ) << 16 | \
			*((data->bytes) + TEMP_SZ + 1) << 8 | \
			*((data->bytes) + TEMP_SZ + 2);
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}

#ifdef MQTT_PROTOCOL
static cJSON *create_json_payload()
{
	/*
	 * payload :=
	 * {
	 * 	"sensor": {
	 * 		"characteristics": [
	 * 		{
	 * 			"characteristicsName": "temperature",
	 * 			"currentValue": 24.1234
	 * 		},
	 * 		{
	 * 			"characteristicsName": "pressure",
	 * 			"currentValue": 48.2125
	 * 		}
	 * 		]
	 * 	}
	 * }
	 */
	cJSON *temp_char = cJSON_CreateObject();
	if (temp_char == NULL)
		fatal_err("Failed to create the temperature characteristic\n");

	cJSON_add_string(temp_char, "characteristicsName", "temperature");
	cJSON_add_number(temp_char, "currentValue", temperature_val);

	cJSON *pressure_char = cJSON_CreateObject();
	if (pressure_char == NULL)
		fatal_err("Failed to create the pressure characteristic\n");

	cJSON_add_string(pressure_char, "characteristicsName", "pressure");
	cJSON_add_number(pressure_char, "currentValue", pressure_val);

	cJSON *char_arr = cJSON_CreateArray();
	if (char_arr == NULL)
		fatal_err("Failed to create characteristics array\n");

	cJSON_AddItemToArray(char_arr, temp_char);
	cJSON_AddItemToArray(char_arr, pressure_char);

	cJSON *sensor = cJSON_CreateObject();
	if (sensor == NULL)
		fatal_err("Failed to create the sensor JSON object\n");

	cJSON_AddItemToObject(sensor, "characteristics", char_arr);

	cJSON *payload = cJSON_CreateObject();
	if (payload == NULL)
		fatal_err("Failed to create payload JSON object\n");

	cJSON_AddItemToObject(payload, "sensor", sensor);

	return payload;
}

void send_json_payload(cc_buffer_desc *send_buffer, cc_data_sz *send_sz)
{
	cJSON *json_payload = create_json_payload();
	if (json_payload == NULL)
		fatal_err("Failed to create JSON payload\n");

	char *json_txt = cJSON_PrintUnformatted(json_payload);
	if (json_txt == NULL)
		fatal_err("Failed to retrieve JSON text\n");

	dbg_printf("\nOUT:%s\n", json_txt);

	*send_sz = strlen(json_txt);
	*send_sz = (*send_sz < CC_MAX_SEND_BUF_SZ) ? *send_sz : CC_MAX_SEND_BUF_SZ;
	char *buf = (char *)cc_get_send_buffer_ptr(send_buffer, CC_SERVICE_BASIC);
	memcpy(buf, json_txt, *send_sz);

	free(json_txt);
	cJSON_Delete(json_payload);
}
#endif
