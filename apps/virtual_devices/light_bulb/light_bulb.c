/* Copyright(C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "light_bulb.h"

static char_t chrt_bulb[] = {
        {
                "unitState",
                "Boolean",
                "RW",
                "false",
                "NULL",
                "NULL"
        },
        {
                "dimmerValue",
                "NumberRange",
                "RW",
                "0",
                "Percent",
                "NULL"
        },
};

#define CHAR_NUM        (sizeof(chrt_bulb)/sizeof(char_t))

static bool check_charc(const cJSON *cname, const cJSON *value,
                        cmd_responce_data_t *cmd_resp_data)
{
        bool error = false;
        if (!cname) {
                snprintf(cmd_resp_data->status_message,
                        sizeof(cmd_resp_data->status_message), "%s",
                        MISS_CHAR);
                cmd_resp_data->err_code = MQTT_CMD_STATUS_BAD_REQUEST;
                return false;
        } else if (!value) {
                snprintf(cmd_resp_data->status_message,
                        sizeof(cmd_resp_data->status_message), "%s",
                        MISS_VALUE);
                cmd_resp_data->err_code = MQTT_CMD_STATUS_BAD_REQUEST;
                return false;
        } else if (cname && value) {
                if (!strncmp(cname->valuestring, "unitState",
                                strlen(cname->valuestring))) {
                        if (strncmp(value->valuestring, "true",
                                strlen(value->valuestring))) {
                                if (strncmp(value->valuestring, "false",
                                        strlen(value->valuestring))) {
                                        snprintf(cmd_resp_data->status_message,
                                        sizeof(cmd_resp_data->status_message),
                                        "%s",
                                        INV_VALUE);
                                        error = true;
                                }
                        }
                } else if (!strncmp(cname->valuestring, "dimmerValue",
                                strlen(cname->valuestring))) {
                        int vl = strtol(value->valuestring, (char **)NULL, 10);
                        if ((errno == ERANGE ) || vl < 0) {
                                errno = 0;
                                snprintf(cmd_resp_data->status_message,
                                        sizeof(cmd_resp_data->status_message),
                                        "%s", INV_VALUE);
                                error = true;
                        }
                } else {
                        error = true;
                        snprintf(cmd_resp_data->status_message,
                                sizeof(cmd_resp_data->status_message),
                                "%s", BAD_REQ);
                }
                if (error) {
                        cmd_resp_data->err_code = MQTT_CMD_STATUS_BAD_REQUEST;
                        return false;
                }
        }
        return true;
}

void set_device_char(const cJSON *cname, const cJSON *value,
                        cmd_responce_data_t *cmd_resp_data)
{
        if (!check_charc(cname, value, cmd_resp_data))
                return;
        for (int i = 0; i < CHAR_NUM; i++) {
                if (!strncmp(cname->valuestring, chrt_bulb[i].char_name,
                        strlen(cname->valuestring))) {

                        printf("Setting value for the bulb characteristic %s "
                                "from %s to %s\n", chrt_bulb[i].char_name,
                                chrt_bulb[i].cur_value, value->valuestring);
                        snprintf(chrt_bulb[i].cur_value,
                                sizeof(chrt_bulb[i].cur_value), "%s",
                                value->valuestring);
                        return;
                }
        }
        cmd_resp_data->err_code = MQTT_CMD_STATUS_BAD_REQUEST;
        snprintf(cmd_resp_data->status_message,
                sizeof(cmd_resp_data->status_message), "%s", INV_CHAR);
}

char *read_device()
{
        return NULL;
}
