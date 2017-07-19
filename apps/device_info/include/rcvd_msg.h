/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef PROC_MSG_H
#define PROC_MSG_H

#include <stdint.h>

#define MQTT_CMD_STATUS_PROCESSING             (102)
#define MQTT_CMD_STATUS_OK                     (200)
#define MQTT_CMD_STATUS_ACCEPTED               (202)
#define MQTT_CMD_STATUS_NO_CONTENT             (204)
#define MQTT_CMD_STATUS_BAD_REQUEST            (400)
#define MQTT_CMD_STATUS_UNAUTHORIZED           (401)
#define MQTT_CMD_STATUS_FORBIDDEN              (403)
#define MQTT_CMD_STATUS_NOT_FOUND              (404)
#define MQTT_CMD_STATUS_NOT_ACCEPTABLE         (406)
#define MQTT_CMD_STATUS_CONFLICT               (409)
#define MQTT_CMD_STATUS_RANGE_NOT_SATISFIABLE  (416)
#define MQTT_CMD_STATUS_RANGE_LOCKED           (423)
#define MQTT_CMD_STATUS_INTERNAL_ERROR         (500)
#define MQTT_CMD_STATUS_NOT_IMPLEMENTED        (501)

#define NO_UUID         "NO_UUID_DATA"
#define OK              "OK"
#define BAS_REQ         "BAD_REQUEST"
#define NO_CMD          "NO_CMD"
#define MISS_CHAR       "Missing Characteristic"
#define INV_CHAR        "Invalid Characteristic"
#define WRNG_CMD        "Unknown command"
#define MISS_UUID       "Missing UUID"

#define INVALID                 -1
#define VALID                   0
#define MAX_CMD_SIZE            40
#define MAX_STATUS_MSG_SIZE     120

typedef struct {
        char command[MAX_CMD_SIZE];
        char uuid[MAX_CMD_SIZE];
        uint32_t err_code;
        char status_message[MAX_STATUS_MSG_SIZE];
} cmd_responce_data_t;

void process_rvcd_msg(const char *recvd, uint32_t sz);

#endif
