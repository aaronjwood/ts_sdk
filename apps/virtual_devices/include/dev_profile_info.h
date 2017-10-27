/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef DEV_INFO_H
#define DEV_INFO_H

#if defined(LIGHT_BULB)
#include "light_bulb.h"
#elif defined(ACCELEROMETER)
#include "accelerometer.h"
#elif defined(GPS)
#include "gps.h"
#else
#error "Define valid virtual device"
#endif

#endif
