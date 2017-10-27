/* Copyright(C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"
#include "common_util.h"
#include "cJSON.h"

#define DEV_ID_LEN     50
#define D_VALUE_SZ      7

#define cJSON_add_number(cJSON_obj, name, num)	do { \
	cJSON *number = cJSON_CreateNumber(num); \
	if (number == NULL)  { \
		printf("%s:%d: Failed to create JSON number\n", __func__, __LINE__); \
                while (1); \
        } \
	cJSON_AddItemToObject(cJSON_obj, name, number); \
} while(0)

#define cJSON_add_string(cJSON_obj, name, str) do { \
	cJSON *string = cJSON_CreateString(str); \
        if (string == NULL)  { \
		printf("%s:%d: Failed to create string\n", __func__, __LINE__); \
                while (1); \
        } \
	cJSON_AddItemToObject(cJSON_obj, name, string); \
} while(0)

#define cJSON_create_object(obj) do { \
        obj = cJSON_CreateObject(); \
        if (obj == NULL)  { \
		printf("%s:%d: Failed to create object\n", __func__, __LINE__); \
                while (1); \
        } \
} while(0)

#define cJSON_create_array(char_arry) do { \
        char_arry = cJSON_CreateArray(); \
        if (char_arry == NULL)  { \
		printf("%s:%d: Failed to create characteristics array\n", __func__, __LINE__); \
                while (1); \
        } \
} while(0)

int random_generator(void)
{
	int random;
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1)
		exit(-1);
	if (read(fd, &random, 4) != 4)
		random = 0;
	close(fd);
	return random;
}

static cJSON *create_json_payload(const char *char1, const char *char2,
        const char *s_num, const char *app_name)
{
	char value[D_VALUE_SZ];
	char dev_id[DEV_ID_LEN];

	if (!utils_get_device_id(dev_id, DEV_ID_LEN, "eth0")) {
		printf("%s:%d: Failed to retrieve device id\n",
			__func__, __LINE__);
		return NULL;
	}

	cJSON *payload = cJSON_CreateObject();
	if (payload == NULL) {
		printf("%s:%d: Failed to create payload object\n",
			__func__, __LINE__);
		return NULL;
	}

	cJSON_add_string(payload, "unitName", "VZW_LH_UNIT_01");
	cJSON_add_string(payload, "unitMacId", dev_id);
	cJSON_add_string(payload, "unitSerialNo", s_num);

	cJSON *sensor;
	cJSON_create_object(sensor);

	cJSON_AddItemToObject(payload, "sensor", sensor);

	cJSON *char_arr;
	cJSON_create_array(char_arr);

	cJSON_AddItemToObject(sensor, "characteristics", char_arr);

	cJSON *x_char, *y_char;
	cJSON_create_object(x_char);
	cJSON_add_string(x_char, "characteristicsName", char1);
	float accl = (float)(random_generator() / 100.00);
	snprintf(value, D_VALUE_SZ, "%g", accl);
	cJSON_add_string(x_char, "currentValue", value);
	cJSON_AddItemToArray(char_arr, x_char);

	cJSON_create_object(y_char);
	cJSON_add_string(y_char, "characteristicsName", char2);
	accl = (float)(random_generator() / 100.00);
	snprintf(value, D_VALUE_SZ, "%g", accl);
	cJSON_add_string(y_char, "currentValue", value);

	cJSON_AddItemToArray(char_arr, y_char);

	cJSON *avail_arr;
	cJSON_create_array(avail_arr);
	cJSON *aval_unit;
	cJSON_create_object(aval_unit);
	cJSON_add_string(aval_unit, "name", app_name);
	cJSON_add_string(aval_unit, "id", "");
	cJSON_AddItemToArray(avail_arr, aval_unit);
	cJSON_AddItemToObject(payload, "availableUnits", avail_arr);

	return payload;
}

char *create_unit_on_board_payload(const char *char1, const char *char2,
        const char *s_num, const char *app_name)
{
	if (!char1 || !char2 || !s_num || !app_name)
		return NULL;

	cJSON *json_payload = create_json_payload(char1, char2, s_num, app_name);
	if (json_payload == NULL)
		return NULL;

	char *json_txt = cJSON_PrintUnformatted(json_payload);
	if (json_txt == NULL) {
		printf("%s:%d: Failed to creaste string from json payload\n",
			__func__, __LINE__);
		return NULL;
	}

	cJSON_Delete(json_payload);
	return json_txt;
}
