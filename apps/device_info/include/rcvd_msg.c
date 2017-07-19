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

        if(cmd_resp_data == NULL)
                return NULL;
        object = cJSON_CreateObject();
        if (object == NULL)
                return NULL;

        if (cmd_resp_data->err_code == MQTT_CMD_STATUS_OK) {
                oem_update_profiles_info(NULL);
                if (!char_name && !prof)
                        payload_obj = create_getopt_payload();
                else if (!char_name) {
                        payload_obj = prof;
                } else if (char_name && (!prof_name)) {
                        payload_obj = char_name;
                }
                otp_payload = true;
        } else
                otp_payload = false;

        cJSON_AddItemToObject(object, (acronym == true) ? "UCD" : "unitCommand",
                        cJSON_CreateString(cmd_resp_data->command));
        if (cname)
                cJSON_AddItemToObject(object,
                        (acronym == true) ? "CNAME" : "characteristicsName",
                        cJSON_CreateString(cname->valuestring));

        cJSON_AddItemToObject(object,
                (acronym == true) ? "CUUID" : "commandUUID",
                cJSON_CreateString(cmd_resp_data->uuid));

        cJSON_AddItemToObject(object, (acronym == true) ? "SMSG" : "statusMsg",
                cJSON_CreateString(cmd_resp_data->status_message));

        cJSON_AddItemToObject(object, (acronym == true) ? "SCD" : "statusCode",
                        cJSON_CreateNumber((double)cmd_resp_data->err_code));
        if (otp_payload)
                cJSON_AddItemToObject(object,
                        (acronym == true) ? "PLD" : "payload",
                        cJSON_CreateString(payload_obj));

        msg = cJSON_PrintUnformatted(object);
        cJSON_Delete(object);
        return msg;
}

static char *prepare_device_info(const cJSON *cname, const char *cmditem,
                        const cJSON *uuid, cmd_responce_data_t *cmd_resp_data,
                        bool acronym)
{

        bool oem_prof_flag = false;
        bool oem_char_flag = false;
        char *prof = NULL;
        char *char_name = NULL;
        char *msg = NULL;
        int i = 0;

        if (uuid == NULL) {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, MISS_UUID);
                msg = fill_otpcmd_resp_msg(cname, NULL, NULL,
                                        cmd_resp_data, acronym);
                cJSON_Delete(object);
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
                msg = fill_otpcmd_resp_msg(cname, NULL, NULL,
                        cmd_resp_data, acronym);
        return msg;
}

static void process_server_cmd_msg(const char *msg,
                                cmd_responce_data_t *cmd_resp_data)
{

        int i = 0;
        uint8_t dev_onboard_seq = 0;
        bool characteristic_found = false;
        bool acronym = false;
        bool unitonboard_pub_req = false;

        cJSON *object = NULL;
        cJSON *cname = NULL;
        cJSON *cmditem = NULL;
        cJSON *uuid = NULL;

        object = cJSON_Parse(msg);

        if (object != NULL) {
                cmditem = cJSON_GetObjectItem(object, "unitCommand");
                if (cmditem == NULL) {
                        cmditem = cJSON_GetObjectItem(object, "UCD");
                        if (cmditem != NULL) {
                                acronym = true;
                                PRINTF("%s:%d: short JSON rcvd\n",
                                        __func__, __LINE__);
                        } else {
                                dev_onboard_seq++;
                                PRINTF("%s:%d: rcvd empty json\n",
                                        __func__, __LINE__);
                                unitonboard_pub_req = true;
                        }
                }
        }

        if (object == NULL) {
                prepare_resp(cmd_resp_data, NO_CMD, NO_UUID,
                        MQTT_CMD_STATUS_BAD_REQUEST, BAD_REQ);
                return; //dipen, figure this out what to return
        }

        if (cmditem == NULL) {
                prepare_resp(cmd_resp_data, NO_CMD, NO_UUID, MQTT_CMD_STATUS_OK,
                                OK);
                return; //dipen, figure this out what to return
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
                prepare_device_info(cname, cmditem->valuestring, uuid,
                        cmd_resp_data, acronym);
        } else {
                prepare_resp(cmd_resp_data, NULL, NULL,
                        MQTT_CMD_STATUS_BAD_REQUEST, WRNG_CMD);
                cJSON_Delete(object);
                return;
        }

        cJSON_Delete(object);
        return;
}

static char *create_cmd_resp_msg(cmd_responce_data_t *cmd_resp_data)
{
        cJSON *object = cJSON_CreateObject();
        char *msg = NULL;

        if (!object)
                return NULL;

        cJSON_AddItemToObject(object, "unitCommand", cJSON_CreateString(cmd_resp_data->command));
        cJSON_AddItemToObject(object, "commandUUID", cJSON_CreateString(cmd_resp_data->uuid));
        cJSON_AddItemToObject(object, "statusMsg",   cJSON_CreateString(cmd_resp_data->status_message));
        cJSON_AddItemToObject(object, "statusCode",  cJSON_CreateNumber((double)cmd_resp_data->err_code));
        if(get_payload_obj)
        cJSON_AddItemToObject( object, "payload", get_payload_obj);


        msg = cJSON_Print( object );
        cJSON_Delete( object );

        if(get_payload_obj)
        get_payload_obj = NULL;

        return msg;
}

void process_rvcd_msg(const char *server_msg,
                        cmd_responce_data_t *cmd_resp_data)
{

        char *json_msg = NULL;

        printf("Message received: %s\n", server_msg);
        process_server_cmd_msg(server_msg, cmd_resp_data);

        if (unitonboard_pub_req) {
                json_msg = vzw_json_set_onboard_msg();
                if (json_msg != NULL) {
                IOT_DEBUG("ThingSpaceApp : Sending UNITOnBoard message\n");
                thingspace_publish_event(lh_pub_topic_onboard, json_msg, PUB_MSG_EXPIRY_TIME);
                tx_byte_release(json_msg);
                } else {
                IOT_DEBUG("ThingSpaceApp : Onboard msg creation failed\n");
                }
        }

        json_msg = vzw_json_create_cmd_resp_msg(&cmd_resp_data);

        if(json_msg != NULL) {
        IOT_DEBUG("ThingSpaceApp : Sending ServerCmdResponse message\n");
        thingspace_publish_cmd_response(cmd_resp_data.uuid, json_msg, PUB_MSG_EXPIRY_TIME);
        tx_byte_release(json_msg);
        } else {
        IOT_DEBUG("ThingSpaceApp : Resp msg creation failed\n");
        }
}
