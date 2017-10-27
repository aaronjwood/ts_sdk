/* Copyright(C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "dev_profile_info.h"
#include "dbg.h"
#include "common_util.h"

/* created json null terminated string */
char *read_device()
{
        return create_unit_on_board_payload("AccelerationX", "AccelerationY",
                SERIAL_NUM, "accelerometer");
}

/* Just a stub */
void set_device_char(const cJSON *cname, const cJSON *value,
                        cmd_responce_data_t *cmd_resp_data)
{
}
