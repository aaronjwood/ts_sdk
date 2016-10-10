/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "cloud_comm.h"
#include "ott_protocol.h"

/* XXX: Replace with our server:port */
#define SERVER_NAME	"www.tapr.org"
#define SERVER_PORT	"443"

static uint8_t dev_ID[UUID_SZ];
static const uint8_t *dev_sec;
static const uint16_t dev_sec_sz;
static bool send_in_progress;

bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec)
{
	if (d_ID == NULL || d_sec == NULL || d_sec_sz == 0 ||
			(dev_sec_sz + UUID_SZ > MAX_DATA_SZ))
		return false;

	memcpy(dev_ID, d_ID, UUID_SZ);
	dev_sec = malloc(dev_sec_sz);
	memcpy(dev_sec, d_sec, dev_sec_sz);

	if (ott_protocol_init() != OTT_OK)
		return false;

	send_in_progress = false;
	return true;
}

cc_status cc_send_bytes_to_cloud(uint16_t sz, uint8_t *data)
{
	if (send_in_progress)
		return CC_SEND_BUSY;

	/* Initiate a connection to the cloud server */
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK)
		return CC_SEND_FAILED;

	/* Authenticate the device with the cloud */
	if (ott_send_auth_to_cloud(CF_PENDING, dev_ID, dev_sec_sz, dev_sec)
			!= OTT_OK)
		return CC_SEND_FAILED;

	return CC_SEND_SUCCESS;
}
