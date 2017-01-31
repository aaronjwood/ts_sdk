/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"
#include "protocol.h"

proto_result proto_init()
{
#ifdef OTT_PROTOCOL
                return ott_protocol_init();
#elseif SMSNAS_PROTOCOL
                return PROTO_OK;
#endif
}

cost uint8_t *proto_get_rcvd_msg_ptr(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_rcv_buffer(msg);
#elseif SMSNAS_PROTOCOL
        return NULL;
#endif
}

proto_result proto_set_auth(const uint8_t *d_id, uint32_t d_id_sz,
                        const uint8_t *d_sec, uint32_t d_sec_sz)
{
#ifdef OTT_PROTOCOL
        return ott_set_auth(d_id, d_id_sz, d_sec, d_sec_sz);
#else
        return PROTO_OK;
#endif
}

uint32_t proto_get_default_polling()
{
#ifdef OTT_PROTOCOL
        return ott_get_default_polling();
#else
        return 0;
#endif
}

void proto_initiate_quit(bool send_nack)
{
#ifdef OTT_PROTOCOL
        ott_initiate_quit(send_nack);
#endif
}

uint32_t proto_get_sleep_interval(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_sleep_interval(msg);
#elseif SMSNAS_PROTOCOL
        return 0;
#endif
}

uint32_t proto_get_rcvd_data_len(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_rcvd_data_len(msg);
#elseif SMSNAS_PROTOCOL
        return 0;
#endif
}

proto_result proto_resend_init_config(proto_callback cb)
{
#ifdef OTT_PROTOCOL
        return ott_resend_init_config(cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

proto_result proto_send_msg_to_cloud(void *msg, uint32_t sz,
                                        proto_callback cb)
{
#ifdef OTT_PROTOCOL
        return ott_send_bytes_to_cloud(msg, sz, cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

void proto_send_ack()
{
#ifdef OTT_PROTOCOL
        ott_send_ack();
#elseif SMSNAS_PROTOCOL
        return;
#endif
}

void proto_send_nack()
{
#ifdef OTT_PROTOCOL
        ott_send_nack();
#elseif SMSNAS_PROTOCOL
        return;
#endif
}

proto_status proto_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
                                        proto_callback rcv_cb)
{

#ifdef OTT_PROTOCOL
        return ott_set_recv_buffer(rcv_buf, sz, rcv_cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

void proto_maintenance(bool poll_due)
{
#ifdef OTT_PROTOCOL
        ott_maintenance(poll_due);
#elseif SMSNAS_PROTOCOL
        return;
#endif
}
