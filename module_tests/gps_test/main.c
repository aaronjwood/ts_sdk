/* Copyright (c) 2017 Verizon. All rights reserved */

/*
 * Minimal test application program. It initializes the GPS module, receive the
 * location information from and prints it on the console.
 * To keep the test program simple, error handling is kept to a minimum.
 * Wherever possible, the program halts with an ASSERT in case the API fails.
 */

#include <string.h>
#include "sys.h"
#include "dbg.h"
#include "gps_hal.h"

struct parsed_nmea_t parsedNMEA;

int main()
{
	sys_init();
	dbg_module_init();
	dbg_printf("Begin :\n");

	dbg_printf("Initializing GPS module\n");
	ASSERT(gps_module_init() == true);
	while (1) {
		/* Receive the GPS data */
		if (gps_receive(&parsedNMEA)) {
			dbg_printf("GPS Latitude: %f Longitude: %f\n",
				parsedNMEA.latitude_degrees,
				parsedNMEA.longitude_degrees);
		}
	}
	return 0;
}
