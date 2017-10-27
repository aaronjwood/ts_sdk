/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef LB_INFO_H
#define LB_INFO_H

#include <stdbool.h>
#include "rcvd_msg.h"
#include "cJSON.h"

#define APP_NAME        "virtual_light_bulb"
#define SERIAL_NUM      "123"
#define PROF_NAME       "light_bulb"
#define PROF_ID         "456"

#define VALUE_SZ        10
#define DEV_PROF_ID_SZ  16

/* This is structure is used for all the characteristics names
 * which are provided under device model
 */
typedef struct characteristics_t {
        const char *char_name;
        const char *par_type;
        const char *access_level;
        char cur_value[VALUE_SZ];
        const char *measurement_unit;
        const char *additional_info;
} char_t;

typedef struct device_profile_data_t {
        const char *name;
        char id[DEV_PROF_ID_SZ];
        char_t *dev_charstics;
} dev_prof_t;

void set_device_char(const cJSON *cname, const cJSON *value,
                        cmd_responce_data_t *cmd_resp_data);

char *read_device();

#endif
