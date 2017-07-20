/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef APP_INFO_H
#define APP_INFO_H

#define APP_NAME        "device_info"
#define SERIAL_NUM      "123"
#define PROF_NAME       "diagnostic_info"
#define PROF_ID         "456"

#if 0
typedef struct char_name_t
{
    const char *char_name;
    const char *para_type;
    const char *access_level;
    char current_value[10];
    char last_value[10];
    const char *value_unit;
    const char **provided_values;
    int num_of_provided_values;
    const char *additional_info;
} char_t;

#define SENSOR_PROFILE_ID_SIZE  (16)

typedef struct
{
    const char *profile;
    char id[SENSOR_PROFILE_ID_SIZE];
    char_t *char_name;   /*pointer to the characteristics array*/
    uint32_t chr_number;
} sensor_profile_data_t;

#endif

#endif
