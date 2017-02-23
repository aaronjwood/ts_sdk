/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __CC_BASIC_SERVICE_H
#define __CC_BASIC_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "cloud_comm.h"

/**
 * \file cc_control_service.h
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

extern const cc_service_descriptor cc_basic_service_descriptor;

#endif /* CC_BASIC_SERVICE_H */
