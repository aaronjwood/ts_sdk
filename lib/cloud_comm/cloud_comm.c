/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "cloud_comm.h"
#include "ott_protocol.h"

bool cc_init(void)
{
	if (ott_protocol_init() != OTT_OK)
		return false;
	return true;
}

cc_status cc_send_bytes_to_cloud(cc_data_sz sz, uint8_t *data)
{
	/* Initiate a connection to the cloud server */
	if (ott_initiate_connection() != OTT_OK)
		return CC_ERROR;
	return CC_OK;
}
