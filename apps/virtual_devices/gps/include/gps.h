/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef LB_INFO_H
#define LB_INFO_H

#include <stdbool.h>
#include "rcvd_msg.h"
#include "cJSON.h"

#define APP_NAME        "virtual_gps"
#define SERIAL_NUM      "123"
#define PROF_NAME       "gps"
#define PROF_ID         "456"

void set_device_char(const cJSON *cname, const cJSON *value,
                        cmd_responce_data_t *cmd_resp_data);

char *read_device();

#endif
