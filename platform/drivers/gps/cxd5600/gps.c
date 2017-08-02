/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gps_hal.h"
#include "sys.h"
#include "dbg.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "gps_config.h"

#define GPS_RX_BUFFER_SIZE		255
#define GPS_SENTENCE_BUFFER_SIZE	128
#define GPS_TIMEOUT_MS			5000
#define GNSS_INT_POLL_STEP_MS		20
#define GNSS_INT_POLL_TIME_MS		(2000+GNSS_INT_POLL_STEP_MS)

static periph_t i2c_handle;
static uint8_t i2c_dest_addr;
static pin_name_t gps_dor_pin;
static pin_name_t gps_reset_pin;

static uint8_t gps_text[GPS_RX_BUFFER_SIZE];
static uint8_t sentenceBuffer[GPS_SENTENCE_BUFFER_SIZE];
volatile bool recvdflag;

static const uint8_t GNSS_CMD_GSP[] = "@GSP\r\n";
static const uint8_t GNSS_CMD_GSTP[] = "@GSTP\r\n";
static const uint8_t GNSS_CMD_GSP_RSP[] = "[GSP] Done";
static const uint8_t GNSS_CMD_GSTP_RSP[] = "[GSTP] Done";

static bool read_GPS_DOR_pin(uint16_t step_ms, uint16_t duration)
{
	uint16_t steps = 0;
	uint16_t max_steps = 0;

	if (step_ms && duration)
		max_steps = duration/step_ms;

	while (gpio_read(gps_dor_pin) != PIN_HIGH) {
		if (step_ms && steps > max_steps)
			break;
		sys_delay(step_ms);
		steps++;
	}
	if ((steps*step_ms) > duration)
		return false;
	else
		return true;
}

static int fastCeil(float num)
{
	int inum = (int)num;
	if (num == (float)inum)
		return inum;
	return inum + 1;
}

static int fastFloor(double x)
{
	int xi = (int)x;
	return x < xi ? xi - 1 : xi;
}

static int fastTrunc(double d)
{
	return (d > 0) ? fastFloor(d) : fastCeil(d) ;
}

static double floatmod(double a, double b)
{
	return (a - b * fastFloor(a / b));
}

static uint8_t parseHex(char c)
{
	if (c < '0')
		return 0;

	if (c <= '9')
		return c - '0';

	if (c < 'A')
		return 0;

	if (c <= 'F')
		return (c - 'A')+10;

	return 0;
}

static void parse_time(char *p, struct parsed_nmea_t *parsedNMEA)
{
	/* Parse Time */
	float timef = atof(p);
	uint32_t time = timef;
	parsedNMEA->hour = time / 10000;
	parsedNMEA->minute = (time % 10000) / 100;
	parsedNMEA->seconds = (time % 100);
	parsedNMEA->milliseconds = floatmod((double) timef, 1.0) * 1000;
	dbg_printf("hour  %d min %d sec %d\n",
	parsedNMEA->hour, parsedNMEA->minute, parsedNMEA->seconds);
}

static char *parse_latitude(char *p, struct parsed_nmea_t *parsedNMEA,
				 bool *retval)
{
	int32_t degree;
	long minutes;
	char degreebuff[10];

	if (',' != *p) {
		strncpy(degreebuff, p, 2);
		p += 2;
		degreebuff[2] = '\0';
		degree = atol(degreebuff) * 10000000;

		strncpy(degreebuff, p, 2); /* minutes */
		p += 3; /* skip decimal point */
		strncpy(degreebuff + 2, p, 4);
		degreebuff[6] = '\0';
		minutes = 50 * atol(degreebuff) / 3;

		parsedNMEA->latitude_fixed = degree + minutes;
		parsedNMEA->latitude = degree / 100000 + minutes * 0.000006F;
		parsedNMEA->latitude_degrees = (parsedNMEA->latitude
		-100 * fastTrunc(parsedNMEA->latitude/100))/60.0;
		parsedNMEA->latitude_degrees +=
		 fastTrunc(parsedNMEA->latitude/100);
	}

	p = strchr(p, ',')+1;
	if (',' != *p) {
		if (p[0] == 'S')
			parsedNMEA->latitude_degrees *= -1.0;
		if (p[0] == 'N')
			parsedNMEA->lat = 'N';
		else if (p[0] == 'S')
			parsedNMEA->lat = 'S';
		else if (p[0] == ',')
			parsedNMEA->lat = 0;
		else
			*retval = false;
	}
	return p;
}

static char *parse_longitude(char *p, struct parsed_nmea_t *parsedNMEA,
				 bool *retval)
{
	int32_t degree;
	long minutes;
	char degreebuff[10];

	if (',' != *p) {

		strncpy(degreebuff, p, 3);
		p += 3;
		degreebuff[3] = '\0';
		degree = atol(degreebuff) * 10000000;
		strncpy(degreebuff, p, 2); /* minutes */
		p += 3; /* skip decimal point */
		strncpy(degreebuff + 2, p, 4);
		degreebuff[6] = '\0';
		minutes = 50 * atol(degreebuff) / 3;

		parsedNMEA->longitude_fixed = degree + minutes;
		parsedNMEA->longitude = degree / 100000 + minutes * 0.000006F;
		parsedNMEA->longitude_degrees = (parsedNMEA->longitude
		-100 * fastTrunc(parsedNMEA->longitude/100))/60.0;
		parsedNMEA->longitude_degrees +=
		 fastTrunc(parsedNMEA->longitude/100);
	}

	p = strchr(p, ',')+1;

	if (',' != *p) {

		if (p[0] == 'W')
			parsedNMEA->longitude_degrees *= -1.0;
		if (p[0] == 'W')
			parsedNMEA->lon = 'W';
		else if (p[0] == 'E')
			parsedNMEA->lon = 'E';
		else if (p[0] == ',')
			parsedNMEA->lon = 0;
		else
			*retval = false;
	}
	return p;
}

static char *parse_GGA_param(char *p, struct parsed_nmea_t *parsedNMEA)
{
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->fix_quality = atoi(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->satellites = atoi(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->HDOP = atof(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->altitude = atof(p);

	p = strchr(p, ',')+1;
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->geoid_height = atof(p);

	return p;
}

static char *parse_RMC_param(char *p, struct parsed_nmea_t *parsedNMEA)
{
	/* Parse Speed */
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->speed = atof(p);

	/* Parse Angle */
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsedNMEA->angle = atof(p);

	p = strchr(p, ',')+1;
	if (',' != *p) {

		uint32_t fulldate = atof(p);
		parsedNMEA->day = fulldate / 10000;
		parsedNMEA->month = (fulldate % 10000) / 100;
		parsedNMEA->year = (fulldate % 100);

	}

	return p;
}

static bool validate_nmea(char *nmea)
{
	if (nmea[strlen(nmea)-4] == '*') {

		uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
		sum += parseHex(nmea[strlen(nmea)-2]);

		/* Validate the checksum */
		for (uint8_t i = 2; i < (strlen(nmea)-4); i++)
			sum ^= nmea[i];

		if (sum != 0) {
			dbg_printf("Bad NMEA Checksum");
			return false;
		}
	}
	return true;
}

/**
 * \brief Receive and read new GPS NMEA and set the recvd flag
 *
 * \retval true New NMEA was received successfully.
 * \retval false No new NMEA received.
 *
 */
static bool gps_new_NMEA_received()
{
	uint8_t *buffer = gps_text;
	if (!(i2c_master_read(i2c_handle, i2c_dest_addr,
		 GPS_RX_BUFFER_SIZE, buffer))) {
		dbg_printf("i2c_master_read failed\n");
		return false;
	}

	int bufferStartIndex = 0;
	int bufferEndIndex = 0;
	for (int i = 0; i < GPS_RX_BUFFER_SIZE; i++) {
		if (buffer[i] == '$')
			bufferStartIndex = i;
		else if (buffer[i] == '\n')
			bufferEndIndex = i;
	}

	int32_t sz = (&buffer[bufferEndIndex]
			 - &buffer[bufferStartIndex]) + 1;
	dbg_printf("sentenceSize calculate = %ld\r\n", sz);
	if (sz > 1) {
		memcpy(sentenceBuffer, &buffer[bufferStartIndex], sz);
		recvdflag = true;
	}
	return recvdflag;
}

/**
 * \brief Parse the received NMEA.
 * \detail Parse the received NMEA and store the parsed
 * values in the received structure format.
 *
 * \param[in] nmea Received NMEA which needs to be parsed.
 * \param[in] parsedNMEA  parsed values will be stored in this.
 *
 * \retval true NMEA was parsed successfully.
 * \retval false NMEA was not parsed successfully.
 */
static bool gps_parse_nmea(char *nmea, struct parsed_nmea_t *parsedNMEA)
{
	if (validate_nmea(nmea) == false)
		return false;

	/* Look for a few common sentences */
	if (strstr(nmea, "$GPGGA")) {

		/* Found GGA */
		char *bufptr = nmea;
		bool retval = true;

		/* Parse time */
		bufptr = strchr(bufptr, ',')+1;
		parse_time(bufptr, parsedNMEA);

		/* Parse Latitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_latitude(bufptr, parsedNMEA, &retval);
		if (retval == false)
			return false;

		/* Parse Longitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_longitude(bufptr, parsedNMEA, &retval);
		if (retval == false)
			return false;

		/* Parse other parameters */
		bufptr = parse_GGA_param(bufptr, parsedNMEA);
		return true;
	}
	if (strstr(nmea, "$GPRMC")) {

		/* Found RMC */
		char *bufptr = nmea;
		bool retval = true;

		/* Parse Time */
		bufptr = strchr(bufptr, ',')+1;
		parse_time(bufptr, parsedNMEA);

		bufptr = strchr(bufptr, ',')+1;
		if (bufptr[0] == 'A')
			parsedNMEA->fix = true;
		else if (bufptr[0] == 'V')
			parsedNMEA->fix = false;
		else
			return false;

		/* Parse Latitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_latitude(bufptr, parsedNMEA, &retval);
		if (retval == false)
			return false;

		/* Parse Longitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_longitude(bufptr, parsedNMEA, &retval);
		if (retval == false)
			return false;

		/* Parse GPRMC other parameters */
		bufptr = parse_RMC_param(bufptr, parsedNMEA);
		return true;
	}

	return false;
}

/**
 * \brief Return the last GPS NMEA received
 *
 * \retval last NMEA data pointer.
 *
 */
static char *gps_last_NMEA(void)
{
	recvdflag = false;
	return (char *)sentenceBuffer;
}

/**
 * \brief Sends the GPS command
 *
 * \retval true Command sent successfully.
 * \retval false Command not sent successfully.
 *
 */
static bool send_command(const uint8_t *cmd)
{
	uint8_t len = strlen((char *) cmd);
	dbg_printf("Sent command %s\n", cmd);

	/* Send command */
	if (!(i2c_master_write(i2c_handle, i2c_dest_addr, len, cmd))) {
		dbg_printf("i2c_master_write failed\n");
		return false;
	}
	return true;
}

/**
 * \brief Receive and verify GPS command Response
 *
 * \retval true Response is sucessful.
 * \retval false Response is not sucessful.
 *
 */
static bool verify_command_response(const uint8_t *response)
{
	if (read_GPS_DOR_pin(GNSS_INT_POLL_STEP_MS, GNSS_INT_POLL_TIME_MS)
		 == false) {
		dbg_printf("Response not received\n");
		return false;
	} else {
		/* Receive the response */
		memset(gps_text, 0, GPS_RX_BUFFER_SIZE);
		uint8_t *buffer = gps_text;
		if (!(i2c_master_read(i2c_handle, i2c_dest_addr,
			 GPS_RX_BUFFER_SIZE, buffer))) {
			dbg_printf("i2c_master_read failed\n");
			return false;
		}
		if (strstr((char *)buffer, (char *)response)) {
			dbg_printf("received %s\n", response);
			return true;
		} else {
			dbg_printf("Invalid response received\n");
			dbg_printf("%s\n", buffer);
			return false;
		}
	}
}

bool gps_module_init()
{
	gps_reset_pin = GPS_RESET_PIN;
	gpio_config_t reset_pin;
	reset_pin.dir = OUTPUT;
	reset_pin.pull_mode = PP_NO_PULL;
	reset_pin.speed = SPEED_LOW;
	gpio_init(gps_reset_pin, &reset_pin);
	gpio_write(gps_reset_pin, PIN_HIGH);

	sys_delay(2000);
	gps_dor_pin = GPS_DOR_PIN;
	gpio_config_t my_pin;
	my_pin.dir = INPUT;
	my_pin.pull_mode = PP_NO_PULL;
	my_pin.speed = SPEED_LOW;
	gpio_init(gps_dor_pin, &my_pin);

	i2c_handle =  i2c_init(GPS_I2C_SCL, GPS_I2C_SDA, GPS_TIMEOUT_MS);
	if (i2c_handle == NO_PERIPH)
		return false;

	dbg_printf("Reset CXD5600\n");
	i2c_dest_addr = GPS_SLAVE_ADDR;

	/* Send positioning command */
	if (send_command(GNSS_CMD_GSP) == false)
		return false;

	if (verify_command_response(GNSS_CMD_GSP_RSP) == false)
		return false;

	recvdflag   = false;
	return true;
}

bool gps_receive(struct parsed_nmea_t *parsedNEMA)
{
	if (i2c_handle == NO_PERIPH)
		return false;

	if (!parsedNEMA)
		return false;

	if (read_GPS_DOR_pin(GNSS_INT_POLL_STEP_MS, GNSS_INT_POLL_TIME_MS)
		 == true) {
		if (gps_new_NMEA_received() == true) {
			/* Parse the new NMEA received
			* and copy the parsed information */
			gps_parse_nmea(gps_last_NMEA(), parsedNEMA);
			return true;
		}
	}
	return false;
}

bool gps_sleep(void)
{
	if (i2c_handle == NO_PERIPH)
		return false;

	/* Stop Positioning */
	if (send_command(GNSS_CMD_GSTP) == false)
		return false;
	if (verify_command_response(GNSS_CMD_GSTP_RSP) == true)
		return true;
	else {
		/* Retry once again */
		if (verify_command_response(GNSS_CMD_GSTP_RSP) == true)
			return true;
		else
			return false;
	}
}

bool gps_wake(void)
{
	if (i2c_handle == NO_PERIPH)
		return false;

	/* Start positioning */
	if (send_command(GNSS_CMD_GSP) == false)
		return false;

	if (verify_command_response(GNSS_CMD_GSP_RSP) == false)
		return false;

	recvdflag   = false;
	return true;
}

