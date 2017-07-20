/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef PROC_MSG_H
#define PROC_MSG_H

#include <stdint.h>

#define MQTT_CMD_STATUS_OK                     (200)
#define MQTT_CMD_STATUS_BAD_REQUEST            (400)

#define NO_UUID         "NO_UUID_DATA"
#define OK              "OK"
#define BAD_REQ         "BAD_REQUEST"
#define NO_CMD          "NO_CMD"
#define MISS_CHAR       "Missing Characteristic"
#define INV_CHAR        "Invalid Characteristic"
#define WRNG_CMD        "Unknown command"
#define MISS_UUID       "Missing UUID"

#define INVALID                 -1
#define VALID                   0
#define MAX_CMD_SIZE            40

typedef struct rsp_t {
        char *rsp_msg;
        uint32_t rsp_len;
        bool on_board;
        char uuid[MAX_CMD_SIZE];
} rsp;

void process_rvcd_msg(const char *recvd, uint32_t sz, rsp *rsp_to_remote);

#endif
