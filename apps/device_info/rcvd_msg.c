/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * Process received message
 */

#include <string.h>
#include "dbg.h"
#include "cJSON.h"
#include "oem_hal.h"
#include "app_info.h"
#include "rcvd_msg.h"
#include "utils.h"

#define DEV_ID          16
#define NET_INTFC       "eth0"

#define DEBUG_PROC_MSG
#ifdef DEBUG_PROC_MSG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define DEBUG_PROC_ERROR
#ifdef DEBUG_PROC_ERROR
#define PRINTF_ERR(...)     printf(__VA_ARGS__)
#else
#define PRINTF_ERR(...)
#endif

#define IN_FUNCTION_AT() \
	do { \
		PRINTF("%s:%d:\n", __func__, __LINE__);\
	} while (0)

#define RETURN_ERROR_VAL(string, val) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (val); \
	} while (0)

#define RETURN_ERROR(string) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		goto done; \
	} while (0)

#define MAX_STATUS_MSG_SIZE     120

typedef struct {
        char command[MAX_CMD_SIZE];
        char uuid[MAX_CMD_SIZE];
        uint32_t err_code;
        char status_message[MAX_STATUS_MSG_SIZE];
} cmd_responce_data_t;

static void prepare_resp(cmd_responce_data_t *cmd_resp_data, const char *cmd,
                        const char *uuid, uint32_t status_code,
                        const char *status_message)
{
        if (cmd_resp_data != NULL) {
                if (cmd != NULL)
			snprintf(cmd_resp_data->command,
				sizeof(cmd_resp_data->command), "%s", cmd);
                if (uuid != NULL)
			snprintf(cmd_resp_data->uuid,
				sizeof(cmd_resp_data->uuid), "%s", uuid);
                cmd_resp_data->err_code = status_code;
                if (status_message != NULL)
			snprintf(cmd_resp_data->status_message,
				sizeof(cmd_resp_data->status_message), "%s",
				status_message);
        }
}

static const char *create_getopt_payload(void)
{
        return oem_get_all_profile_info_in_json();
}

static char *fill_otpcmd_resp_msg(const cJSON *cname, char *prof,
                        char *char_name, cmd_responce_data_t *cmd_resp_data)
{

        cJSON *object = NULL;
        const char *payload_obj = NULL;
        char *msg = NULL;
        bool otp_payload = false;

        if (cmd_resp_data == NULL)
                RETURN_ERROR_VAL("response pointer is null", NULL);

        object = cJSON_CreateObject();
        if (object == NULL)
                RETURN_ERROR_VAL("creating json object failed", NULL);

        if (cmd_resp_data->err_code == MQTT_CMD_STATUS_OK) {
                oem_update_profiles_info(NULL);

                if (!char_name && !prof)
                        payload_obj = create_getopt_payload();
                else if (!char_name)
                        payload_obj = prof;
                else if (char_name && (!prof))
                        payload_obj = char_name;
                otp_payload = true;
        } else
                otp_payload = false;

        cJSON *temp = cJSON_CreateString(cmd_resp_data->command);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "UCD",temp);
        if (cname) {
                temp = cJSON_CreateString(cname->valuestring);
                if (!temp) {
                        RETURN_ERROR("failed");
                }
                cJSON_AddItemToObject(object, "CNAME", temp);
        }

        temp = cJSON_CreateString(cmd_resp_data->uuid);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "CUUID", temp);

        temp = cJSON_CreateString(cmd_resp_data->status_message);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "SMSG", temp);

        temp = cJSON_CreateNumber((double)cmd_resp_data->err_code);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "SCD", temp);

        if (otp_payload) {
                temp = cJSON_CreateRaw(payload_obj);
                if (!temp) {
                        RETURN_ERROR("failed");
                }
                cJSON_AddItemToObject(object, "PLD", temp);
        }

        msg = cJSON_PrintUnformatted(object);
done:
        cJSON_Delete(object);
        return msg;
}

static char *prepare_device_info(const cJSON *cname, const char *cmditem,
                        const cJSON *uuid, cmd_responce_data_t *cmd_resp_data,
			uint32_t *rsp_len)
{
        char *prof = NULL;
        char *char_name = NULL;
        char *msg = NULL;
        *rsp_len = 0;

        if (uuid == NULL) {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, MISS_UUID);
                msg = fill_otpcmd_resp_msg(cname, NULL, NULL, cmd_resp_data);
                return msg;
        }

        if (cname) {
                prof = oem_get_profile_info_in_json(cname->valuestring);
                if (!prof)
                        char_name = oem_get_characteristic_info_in_json(
                                                cname->valuestring);
        }
        if (cname && !prof && !char_name) {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, INV_CHAR);
                msg = fill_otpcmd_resp_msg(cname, prof, char_name, cmd_resp_data);
        } else if (cname && (prof || char_name))
                msg = fill_otpcmd_resp_msg(cname, prof, char_name, cmd_resp_data);
        else
                msg = fill_otpcmd_resp_msg(NULL, NULL, NULL, cmd_resp_data);
        *rsp_len = strlen(msg) + 1;
        return msg;
}

static char *create_onboard_msg(uint32_t *msg_len)
{
        cJSON *object = NULL;
        cJSON *sensor_object = NULL;
        cJSON *item = NULL;
        char *msg = NULL;
        *msg_len  = 0;
        char device_id[DEV_ID];

        if (!utils_get_device_id(device_id, DEV_ID, NET_INTFC))
                RETURN_ERROR_VAL("device id retreival failed", NULL);

        object = cJSON_CreateObject();
        if (!object)
                RETURN_ERROR_VAL("creating json object failed", NULL);

        cJSON *temp = cJSON_CreateString(APP_NAME);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "unitName", temp);

        temp = cJSON_CreateString(device_id);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "unitMacId", temp);

        temp = cJSON_CreateString(SERIAL_NUM);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "unitSerialNo", temp);

        sensor_object = cJSON_CreateObject();
        if (!sensor_object) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "sensor", sensor_object);

        temp = cJSON_CreateString(PROF_NAME);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(sensor_object, "name", temp);


        temp = cJSON_CreateString(PROF_ID);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(sensor_object, "id", temp);

        item = cJSON_CreateArray();
        if (!item) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(sensor_object, "characteristics", item);
        msg = cJSON_PrintUnformatted(object);
        *msg_len = strlen(msg) + 1;
done:
        cJSON_Delete(object);
        return msg;
}

static char *create_cmd_resp_msg(cmd_responce_data_t *cmd_resp_data, cJSON *pl,
                                uint32_t *rsp_len)
{
        cJSON *object = cJSON_CreateObject();
        char *msg = NULL;
        *rsp_len = 0;
        if (!object)
                RETURN_ERROR_VAL("creating json object failed", NULL);

        cJSON *temp = cJSON_CreateString(cmd_resp_data->command);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "unitCommand", temp);

        temp = cJSON_CreateString(cmd_resp_data->uuid);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "commandUUID", temp);

        temp = cJSON_CreateString(cmd_resp_data->status_message);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "statusMsg", temp);

        temp = cJSON_CreateNumber((double)cmd_resp_data->err_code);
        if (!temp) {
                RETURN_ERROR("failed");
        }
        cJSON_AddItemToObject(object, "statusCode", temp);

        if (pl)
                cJSON_AddItemToObject(object, "payload", pl);

        msg = cJSON_Print(object);
        *rsp_len = strlen(msg) + 1;
done:
        cJSON_Delete(object);
        return msg;
}

static char *process_server_cmd_msg(const char *msg,
                                cmd_responce_data_t *cmd_resp_data,
                                rsp *rsp_to_remote)
{

        cJSON *object = NULL;
        cJSON *cname = NULL;
        cJSON *cmditem = NULL;
        cJSON *uuid = NULL;
        uint32_t *rsp_len = &(rsp_to_remote->rsp_len);
        *rsp_len = 0;
        object = cJSON_Parse(msg);

        if (object != NULL) {
                cmditem = cJSON_GetObjectItem(object, "unitCommand");
                if (!cmditem) {
                        cmditem = cJSON_GetObjectItem(object, "UCD");
                        if (cmditem != NULL)
                                PRINTF("%s:%d: short JSON rcvd\n",
                                        __func__, __LINE__);
                        else {
                                PRINTF("%s:%d: rcvd empty json\n",
                                        __func__, __LINE__);
                                cJSON_Delete(object);
                                rsp_to_remote->on_board = true;
                                return create_onboard_msg(rsp_len);
                        }
                }
        }

        if (object == NULL) {
                /* This is bad request or malformed msg so return status
                 * accordingly
                 */
                prepare_resp(cmd_resp_data, NO_CMD, NO_UUID,
                        MQTT_CMD_STATUS_BAD_REQUEST, BAD_REQ);
                rsp_to_remote->on_board = false;
                snprintf(rsp_to_remote->uuid, MAX_CMD_SIZE, "%s", NO_UUID);
                return create_cmd_resp_msg(cmd_resp_data, NULL, rsp_len);
        }

        uuid = cJSON_GetObjectItem(object, "CUUID");
        if (!uuid)
                uuid = cJSON_GetObjectItem(object, "commandUUID");

        prepare_resp(cmd_resp_data, cmditem->valuestring,
                (uuid != NULL) ? uuid->valuestring : NO_UUID,
                MQTT_CMD_STATUS_OK, OK);

        if (!strcmp(cmditem->valuestring, "GetOtp")) {

                cname = cJSON_GetObjectItem(object, "CNAME");
                if (!cname)
                        cname = cJSON_GetObjectItem(object,
                                                "characteristicsName");
                rsp_to_remote->on_board = false;
                if (uuid) {
                        snprintf(rsp_to_remote->uuid, MAX_CMD_SIZE, "%s",
                                uuid->valuestring);
                } else
                        snprintf(rsp_to_remote->uuid, MAX_CMD_SIZE, "%s",
                                NO_UUID);
                char *rsp_dev = prepare_device_info(cname, cmditem->valuestring,
                        uuid, cmd_resp_data, rsp_len);
                cJSON_Delete(object);
                return rsp_dev;
        } else {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, WRNG_CMD);
                rsp_to_remote->on_board = false;
                if (uuid)
                        snprintf(rsp_to_remote->uuid, MAX_CMD_SIZE, "%s",
                                uuid->valuestring);
                else
                        snprintf(rsp_to_remote->uuid, MAX_CMD_SIZE, "%s",
                                NO_UUID);
                char *rsp_dev = create_cmd_resp_msg(cmd_resp_data, NULL,
                                        rsp_len);
                cJSON_Delete(object);
                return rsp_dev;
        }
        *rsp_len = 0;
        return NULL;
}

void process_rvcd_msg(const char *server_msg, uint32_t sz, rsp *rsp_to_remote)
{
        cmd_responce_data_t cmd_resp_data;
        printf("Message received.......\n");
        printf("%s\n", server_msg);
        rsp_to_remote->rsp_msg = process_server_cmd_msg(server_msg,
                                                &cmd_resp_data, rsp_to_remote);
}
