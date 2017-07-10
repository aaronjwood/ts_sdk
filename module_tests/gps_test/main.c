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
	
		/* Read new NMEA received through GPS */
		if (gps_new_NMEA_received()) {

                	/* Parse the new NMEA received 
			 * and copy the parsed information */
			gps_parse_nmea(gps_last_NMEA(), &parsedNMEA);
                	
			dbg_printf("GPS Latitude: %f Longitude: %f\n",
                	parsedNMEA.latitudeDegrees,parsedNMEA.longitudeDegrees);
        	}
		dbg_printf("Powering down for 60 seconds\n\n");
		sys_sleep_ms(60000);

	}
	return 0;
}
