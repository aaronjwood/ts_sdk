/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * Process received message
 */

#include <string.h>
#include "dbg.h"
#include "cJSON.h"
#include "oem.h"

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

#define RETURN_ERROR(string, ret) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (ret); \
	} while (0)

#define MAX_STATUS_MSG_SIZE     120

typedef struct {
        char command[MAX_CMD_SIZE];
        char uuid[MAX_CMD_SIZE];
        uint32_t err_code;
        char status_message[MAX_STATUS_MSG_SIZE];
} cmd_responce_data_t;

static rsp rsp_to_remote;

static void prepare_resp(cmd_responce_data_t *cmd_resp_data, const char *cmd,
                        const char *uuid, uint32_t status_code,
                        const char *status_message)
{
        if (cmd_resp_data != NULL) {
                if (cmd != NULL)
                        strncpy(cmd_resp_data->command, cmd,
                                sizeof(cmd_resp_data->command));
                if (uuid != NULL)
                        strncpy(cmd_resp_data->uuid, uuid,
                                sizeof(cmd_resp_data->uuid));
                cmd_resp_data->err_code = status_code;
                if (status_message != NULL)
                        strncpy(cmd_resp_data->status_message, status_message,
                                sizeof(cmd_resp_data->status_message));
        }
}

static const char *create_getopt_payload(bool acronym)
{
        return oem_get_all_profile_info_in_json(acronym);
}

static char *fill_otpcmd_resp_msg(cJSON *cname, char *prof,
                        char *chr_name, cmd_responce_data_t *cmd_resp_data,
                        bool acronym)
{
        cJSON *object = NULL;
        char *payload_obj = NULL;
        char *msg = NULL;
        bool otp_payload = false;

        if (cmd_resp_data == NULL)
                return NULL;
        object = cJSON_CreateObject();
        if (object == NULL)
                return NULL;

        if (cmd_resp_data->err_code == MQTT_CMD_STATUS_OK) {
                oem_update_profiles_info(NULL);
                if (!char_name && !prof)
                        payload_obj = create_getopt_payload();
                else if (!char_name)
                        payload_obj = prof;
                else if (char_name && (!prof_name))
                        payload_obj = char_name;
                otp_payload = true;
        } else
                otp_payload = false;

        cJSON *temp = cJSON_CreateString(cmd_resp_data->command);
        if (!temp) {
                PRINTF("%s:%d: failed\n", __func__, __LINE__);
                goto done;
        }
        cJSON_AddItemToObject(object, (acronym == true) ? "UCD" : "unitCommand",
                        temp);
        if (cname) {
                temp = cJSON_CreateString(cname->valuestring);
                if (!temp) {
                        PRINTF("%s:%d: failed\n", __func__, __LINE__);
                        goto done;
                }
                cJSON_AddItemToObject(object,
                        (acronym == true) ? "CNAME" : "characteristicsName",
                        temp);
        }

        temp = cJSON_CreateString(cmd_resp_data->uuid);
        if (!temp) {
                PRINTF("%s:%d: failed\n", __func__, __LINE__);
                goto done;
        }
        cJSON_AddItemToObject(object,
                (acronym == true) ? "CUUID" : "commandUUID",
                temp);

        temp = cJSON_CreateString(cmd_resp_data->status_message);
        if (!temp) {
                PRINTF("%s:%d: failed\n", __func__, __LINE__);
                goto done;
        }
        cJSON_AddItemToObject(object, (acronym == true) ? "SMSG" : "statusMsg",
                temp);

        temp = cJSON_CreateNumber((double)cmd_resp_data->err_code);
        if (!temp) {
                PRINTF("%s:%d: failed\n", __func__, __LINE__);
                goto done;
        }
        cJSON_AddItemToObject(object, (acronym == true) ? "SCD" : "statusCode",
                        temp);
        if (otp_payload) {
                temp = cJSON_CreateString(payload_obj);
                if (!temp) {
                        PRINTF("%s:%d: failed\n", __func__, __LINE__);
                        goto done;
                }
                cJSON_AddItemToObject(object,
                        (acronym == true) ? "PLD" : "payload", temp);
        }

        msg = cJSON_PrintUnformatted(object);
done:
        cJSON_Delete(object);
        return msg;
}

static char *prepare_device_info(const cJSON *cname, const char *cmditem,
                        const cJSON *uuid, cmd_responce_data_t *cmd_resp_data,
                        bool acronym, uint32_t *rsp_len)
{

        bool oem_prof_flag = false;
        bool oem_char_flag = false;
        char *prof = NULL;
        char *char_name = NULL;
        char *msg = NULL;
        *rsp_len = 0;
        int i = 0;

        if (uuid == NULL) {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, MISS_UUID);
                msg = fill_otpcmd_resp_msg(cname, NULL, NULL,
                                        cmd_resp_data, acronym);
                return msg;
        }

        if (cname) {
                prof = oem_get_profile_info_in_json(cname->valuestring,
                                acronym);
                if (!prof)
                        char_name = oem_get_characteristic_info_in_json(
                                                cname->valuestring, acronym);
        }
        if (cname && !prof && !char_name) {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, INV_CHAR);
                msg = fill_otpcmd_resp_msg(cname, prof, char_name,
                        cmd_resp_data);
        } else if (cname && (prof || char_name)) {
                msg = fill_otpcmd_resp_msg(cname, prof, char_name,
                                cmd_resp_data, acronym);
        } else
                msg = fill_otpcmd_resp_msg(NULL, NULL, NULL,
                        cmd_resp_data, acronym);
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
                RETURN_ERROR("device id retreival failed", NULL);

        object = cJSON_CreateObject();
        if (!object)
                RETURN_ERROR("creating json object failed", NULL);

        cJSON *temp = cJSON_CreateString(APP_NAME);
        if (!temp) {
                PRINTF("creating unitname json failed\n");
                goto error;
        }
        cJSON_AddItemToObject(object, "unitName", temp);

        temp = cJSON_CreateString(device_id);
        if (!temp) {
                PRINTF("creating unitMacId json failed\n");
                goto error;
        }
        cJSON_AddItemToObject(object, "unitMacId", temp);

        temp = cJSON_CreateString(SERIAL_NUM);
        if (!temp) {
                PRINTF("creating serial nunmber failed\n");
                goto error;
        }
        cJSON_AddItemToObject(object, "unitSerialNo", temp);

        sensor_object = cJSON_CreateObject();
        if (!sensor_object) {
                PRINTF("creating sensor object failed\n");
                goto error;
        }
        cJSON_AddItemToObject(object, "sensor", sensor_object);

        temp = cJSON_CreateString(PROF_NAME);
        if (!temp) {
                PRINTF("creating profile name failed\n");
                goto error;
        }
        cJSON_AddItemToObject(sensor_object, "name", temp);


        temp = cJSON_CreateString(PROF_ID);
        if (!temp) {
                PRINTF("creating profile id failed\n");
                goto error;
        }
        cJSON_AddItemToObject(sensor_object, "id", temp));

        item = cJSON_CreateArray();
        if (!item) {
                PRINTF("creating array failed\n");
                goto error;
        }
        cJSON_AddItemToObject(sensor_object, "characteristics", item);
        msg = cJSON_PrintUnformatted(object);
        *msg_len = strlen(msg) + 1;
error:
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
                return NULL;

        cJSON *temp = cJSON_CreateString(cmd_resp_data->command);
        if (!temp) {
                PRINTF("creating command json failed\n");
                goto done;
        }
        cJSON_AddItemToObject(object, "unitCommand", temp);

        temp = cJSON_CreateString(cmd_resp_data->uuid);
        if (!temp) {
                PRINTF("creating uuid json failed\n");
                goto done;
        }
        cJSON_AddItemToObject(object, "commandUUID", temp);

        temp = cJSON_CreateString(cmd_resp_data->status_message);
        if (!temp) {
                PRINTF("creating stat msg json failed\n");
                goto done;
        }
        cJSON_AddItemToObject(object, "statusMsg", temp);

        temp = cJSON_CreateNumber((double)cmd_resp_data->err_code);
        if (!temp) {
                PRINTF("creating erro code json failed\n");
                goto done;
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
                                cmd_responce_data_t *cmd_resp_data, char *uuid,
                                bool *on_board, uint32_t *rsp_len)
{

        bool acronym = false;
        cJSON *object = NULL;
        cJSON *cname = NULL;
        cJSON *cmditem = NULL;
        cJSON *uuid = NULL;
        *rsp_len = 0;
        object = cJSON_Parse(msg);

        if (object != NULL) {
                cmditem = cJSON_GetObjectItem(object, "unitCommand");
                if (!cmditem) {
                        cmditem = cJSON_GetObjectItem(object, "UCD");
                        if (cmditem != NULL) {
                                acronym = true;
                                PRINTF("%s:%d: short JSON rcvd\n",
                                        __func__, __LINE__);
                        } else {
                                PRINTF("%s:%d: rcvd empty json\n",
                                        __func__, __LINE__);
                                cJSON_Delete(object);
                                *on_board = true;
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
                *on_board = false;
                snprintf(uuid, MAX_CMD_SIZE, "%s", NO_UUID);
                return create_cmd_resp_msg(cmd_resp_data, NULL, rsp_len);
        }

        if (acronym == true)
                uuid = cJSON_GetObjectItem(object, "CUUID");
        else
                uuid = cJSON_GetObjectItem(object, "commandUUID");

        prepare_resp(cmd_resp_data, cmditem->valuestring,
                (uuid != NULL) ? uuid->valuestring : NO_UUID,
                MQTT_CMD_STATUS_OK, OK);

        if (!strcmp(cmditem->valuestring, "GetOtp")) {
                if (acronym)
                        cname = cJSON_GetObjectItem(object, "CNAME");
                else
                        cname = cJSON_GetObjectItem(object,
                                                "characteristicsName");
                cJSON_Delete(object);
                *on_board = false;
                if (uuid)
                        snprintf(uuid, MAX_CMD_SIZE, "%s", uuid->valuestring);
                else
                        snprintf(uuid, MAX_CMD_SIZE, "%s", NO_UUID);

                return prepare_device_info(cname, cmditem->valuestring, uuid,
                                cmd_resp_data, acronym, rsp_len);
        } else {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, WRNG_CMD);
                cJSON_Delete(object);
                *on_board = false;
                if (uuid)
                        snprintf(uuid, MAX_CMD_SIZE, "%s", uuid->valuestring);
                else
                        snprintf(uuid, MAX_CMD_SIZE, "%s", NO_UUID);
                return create_cmd_resp_msg(cmd_resp_data, NULL, rsp_len);
        }
        *rsp_len = 0;
        return NULL;
}

rsp *process_rvcd_msg(const char *server_msg, uint32_t sz)
{
        char *json_msg = NULL;
        cmd_responce_data_t cmd_resp_data;
        printf("Message received: %s\n", server_msg);
        bool onboard = false;
        rsp_to_remote.rsp_msg = process_server_cmd_msg(server_msg,
                                                &cmd_resp_data,
                                                rsp_to_remote.uuid,
                                                &rsp_to_remote.on_board,
                                                &rsp_to_remote.rsp_len);
        return &rsp_to_remote;
}
