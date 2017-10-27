/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * Sensor module for the following sensors:
 * 1) HMC5883L (3-Axis Digital Compass)
 */

#include <stdbool.h>
#include "i2c_hal.h"

#include "sensor_interface.h"
#include "board_interface.h"
#include "hmc5883l_interpret.h"
#include "sys.h"
#include "dbg.h"
#include "cJSON.h"
#include <string.h>

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

#define I2C_TIMEOUT		2000	/* 2000ms timeout for I2C response */
periph_t  i2c_handle;
i2c_addr_t i2c_dest_addr;
#define READ_MATCH_TIMEOUT	1000   /* 1000ms timeout - read until matched*/

enum sensors {
	HMC5883L,
	NUM_SENSORS
};

hmc5883l_reading_t hmc_vals;

/* HMC5883L */
#define HMC5833L
#define HMC5883L_ADDR		0x1E
#define HMC_DATA_SZ		0x06
#define HMC_CTRL_REG_A		0x00
#define HMC_CTRL_REG_B		0x01
#define HMC_CTRL_REG_MODE	0x02

static uint8_t hmc_init[] = {
	0x70,			/* 5 Hz default, normal measurement */
	0xA0,			/* Gain=5, or any other desired gain */
	0x00,			/* Continuous-measurement mode */
	0x03			/* point to first data register 03 */
};

static bool init_sensors(void)
{
	i2c_dest_addr.slave = HMC5883L_ADDR;
	i2c_dest_addr.reg = HMC_CTRL_REG_A;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &hmc_init[0]));
	
	i2c_dest_addr.reg = HMC_CTRL_REG_B;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &hmc_init[1]));
	i2c_dest_addr.reg = HMC_CTRL_REG_MODE;
	EOE(i2c_write(i2c_handle, i2c_dest_addr, 1, &hmc_init[2]));

	return true;
}

bool si_init(void)
{
	uint32_t timeout_ms = 0;
#ifdef SIMULATE_SENSOR
	return true;
#endif
	/* Initialize the I2C bus on the processor */
	i2c_handle =  i2c_init(I2C_SCL, I2C_SDA, timeout_ms);

	if (i2c_handle != NO_PERIPH)
		return init_sensors();

	return false;
}

bool si_read_calib(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (!data || !data->bytes)
		return false;

#ifdef SIMULATE_SENSOR
	data->sz = 1;
	dbg_printf("\tCalibration table : %d bytes\n", data->sz);
	for (uint8_t i = 0; i < data->sz; i++)
		data->bytes[i] = i;
#endif
	return true;
}

uint8_t si_get_num_sensors(void)
{
	return NUM_SENSORS;
}

static bool read_hmc5883l(uint16_t max_sz, array_t *data)
{
	if (max_sz < HMC_DATA_SZ)
		return false;
	data->sz = HMC_DATA_SZ;
#ifdef SIMULATE_SENSOR
	for (uint8_t i = 0; i < data->sz; i++)
		data->bytes[i] = i;
	return true;
#endif
	i2c_dest_addr.slave = HMC5883L_ADDR;
	i2c_dest_addr.reg = hmc_init[3];
	EOE(i2c_read(i2c_handle, i2c_dest_addr, HMC_DATA_SZ , data->bytes));
	return true;
}

bool si_read_data(uint8_t idx, uint16_t max_sz, array_t *data)
{
	if (!data || !data->bytes)
		return false;
	if (idx > NUM_SENSORS - 1)
		return false;

	if (idx == HMC5883L)
	{
		EOE(read_hmc5883l(max_sz, data));
		ASSERT(hmc5883l_get_readings(data->sz, data->bytes, &hmc_vals));
	}

	return true;
}

bool si_sleep(void)
{
	/* None of the sensors have a sleep sequence */
	return true;
}

bool si_wakeup(void)
{
	/* None of the sensors have a wakeup sequence */
	return true;
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
	 * 			"characteristicsName": "magnetism",
	 * 			"currentValue": {
	 * 						"x":29.0126,
	 * 						"y":11.4885,
	 * 						"z":1001.4579
	 * 					}
	 * 		}
	 * 		]
	 * 	}
	 * }
	 */
	cJSON *magnet_char = cJSON_CreateObject();
	if (magnet_char == NULL)
		fatal_err("Failed to create the magnetism characteristic\n");

	cJSON_add_string(magnet_char, "characteristicsName", "magnetism");

	cJSON *val_magnet = cJSON_CreateObject();
	if (val_magnet == NULL)
		fatal_err("Failed to create magnetism JSON object\n");

	cJSON_add_number(val_magnet, "x", hmc_vals.magnetometer.x);
	cJSON_add_number(val_magnet, "y", hmc_vals.magnetometer.y);
	cJSON_add_number(val_magnet, "z", hmc_vals.magnetometer.z);

	cJSON_AddItemToObject(magnet_char, "currentValue", val_magnet);

	cJSON *char_arr = cJSON_CreateArray();
	if (char_arr == NULL)
		fatal_err("Failed to create characteristics array\n");

	cJSON_AddItemToArray(char_arr, magnet_char);

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
