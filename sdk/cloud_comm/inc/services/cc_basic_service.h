/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __CC_BASIC_SERVICE_H
#define __CC_BASIC_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "cloud_comm.h"

/**
 * \file cc_basic_service.h
 *
 * This module defines a service which provides the most basic data
 * communication to and from the cloud.  This is the core service used by
 * ThingSpace devices.
 *
 * With this service, the cloud can send variable length binary blobs
 * (aka "update" messages) to the device.  The device can send variable length
 * binary blobs (aka "status" messages) to the cloud.  The contents of the
 * blobs are defined entirely by the application using the service.
 *
 */

/* XXX Remove the second value once OTT/MQTT starts including BASIC header. */
#if defined(SMSNAS_PROTOCOL)
#define __CC_BASIC_SERVICE_OVERHEAD (1)
#else
#define __CC_BASIC_SERVICE_OVERHEAD (0)
#endif

/**
 * The maximum size of the binary blob that can be sent or received in a
 * Basic service message.
 */
#define CC_BASIC_MAX_RECV_DATA_SZ (CC_MAX_RECV_BUF_SZ - \
				   __CC_BASIC_SERVICE_OVERHEAD)
#define CC_BASIC_MAX_SEND_DATA_SZ (CC_MAX_SEND_BUF_SZ - \
				   __CC_BASIC_SERVICE_OVERHEAD)

/**
 * The size of CC_RECV_BUFFER() or CC_SEND_BUFFER() needed to receive or send
 * a zero-length Basic service message.
 */
#define CC_BASIC_MIN_RECV_BUF_SZ (__CC_BASIC_SERVICE_OVERHEAD)
#define CC_BASIC_MIN_SEND_BUF_SZ (__CC_BASIC_SERVICE_OVERHEAD)

extern const cc_service_descriptor cc_basic_service_descriptor;

#endif /* CC_BASIC_SERVICE_H */
