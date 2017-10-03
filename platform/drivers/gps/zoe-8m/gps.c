/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gps_hal.h"
#include "sys.h"
#include "dbg.h"
#include "uart_hal.h"
#include "gps_config.h"
#include "gpio_hal.h"

#define GPS_RX_BUFFER_SIZE      2048
#define GPS_SENTENCE_BUFFER_SIZE 128
#define GPS_SEND_TIMEOUT_MS     2000
#define GPS_RCV_TIMEOUT_MS      5000

typedef uint16_t buf_sz;
static periph_t uart;
static volatile bool received_gps_text;
static uint8_t gps_text[GPS_RX_BUFFER_SIZE];
static uint8_t sentence_buffer[GPS_SENTENCE_BUFFER_SIZE];

static bool recvd_flag;
static const uint8_t RCV_RESET[] = { 0xB5, 0x62, 0x06, 0x04, 0x04,
					 0x00, 0x00, 0x01, 0x01,
					 0x00, 0x10, 0x6b };

static const uint8_t REVERT_NMEA[] = { 0xB5, 0x62, 0x06, 0x17, 0x14, 
				       0x00, 0x10, 0x23, 0x0C, 0x00, 
				       0x60, 0x00, 0x00, 0x00, 0x00, 
				       0x01, 0x01, 0x01, 0x00, 0x00, 
				       0x00, 0x00, 0x00, 0x00, 0x00, 
				       0x00, 0xD3, 0x28 };

static const uint8_t RMC_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X04, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X03, 0X3F };

static const uint8_t VTG_Off[]	= { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X05, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X04, 0X46 };

static const uint8_t GSA_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X02, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X01, 0X31 };

static const uint8_t GSV_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X03, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X02, 0X38 };

static const uint8_t GLL_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X01, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X00, 0X2A};

static const uint8_t ZDA_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X08, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0X07, 0X5B };

static const uint8_t GGA_Off[]   = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X00, 0X00,
					 0X00, 0X00, 0X00, 0X00,
					 0X00, 0XFF, 0X23};

static const uint8_t GGA_On[]    = { 0XB5, 0X62, 0X06, 0X01, 0X08,
					 0X00, 0XF0, 0X00, 0X00,
					 0X01, 0X01, 0X00, 0X00,
					 0X00, 0X01, 0X2C };

static const uint8_t SAVE_Config[] = { 0xB5, 0X62, 0X06, 0X09, 0X0D, 0X00, 0X00, 0X00, 0X00, 0X00, 0XFF, 0XFF, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X1B, 0XA9 };

/**
 * \brief Send data over GPS.
 * \details This is a blocking send. In case the modem blocks
 * the flow via hardware flow control, the call will block for
 * at most 'timeout_ms' milliseconds.
 *
 * \param[in] data - Pointer to the data to be sent.
 * \param[in] size - Number of bytes to send
 * \param[in] timeout_ms - Total number of milliseconds to wait before
 * giving up in case of a busy channel.
 *
 * \retval true Data was sent out successfully.
 * \retval false Send aborted due to timeout or null pointer
 * provided for data.
 */
static bool gps_tx(const uint8_t *data, buf_sz size, uint16_t timeout_ms)
{
	if (!data)
		return false;

	if (size == 0)
		return false;

	if (uart_tx(uart, (uint8_t *)data, size, timeout_ms) != true)
		return false;

	return true;
}

/**
 * \brief Receive data from GPS.
 * \details This is a blocking receive. In case the receiver blocks the flow via
 * hardware flow control, the call will block for at most 'timeout_ms'
 * milliseconds.
 *
 * \param[in] data Pointer of the buffer to receive the data.
 * \param[in] size Number of bytes in buffer to receive the data.
 * \param[in] timeout_ms Total number of milliseconds to wait before giving up
 * in case of a busy channel.
 *
 * \retval true Data was received successfully.
 * \retval false Receive aborted due to timeout or null pointer was provided for
 * the data.
 *
 */
static bool gps_rx(uint8_t *data, buf_sz size, uint16_t timeout_ms)
{
	if (!data)
		return false;

	if (size == 0)
		return false;

	if (uart_rx(uart, data, size, timeout_ms) != true)
		return false;

	return true;
}

static int fast_ceil(float num)
{
	int inum = (int)num;
	if (num == (float)inum)
		return inum;
	return inum + 1;
}

static int fast_floor(double x)
{
	int xi = (int)x;
	return x < xi ? xi - 1 : xi;
}

static int fast_trunc(double d)
{
	return (d > 0) ? fast_floor(d) : fast_ceil(d) ;
}

static double float_mod(double a, double b)
{
	return (a - b * fast_floor(a / b));
}

static uint8_t parse_hex(char c)
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

static void parse_time(char *p, struct parsed_nmea_t *parsed_nmea)
{
	/* Parse Time */
	float timef = atof(p);
	uint32_t time = timef;
	parsed_nmea->hour = time / 10000;
	parsed_nmea->minute = (time % 10000) / 100;
	parsed_nmea->seconds = (time % 100);
	parsed_nmea->milliseconds = float_mod((double) timef, 1.0) * 1000;
	dbg_printf("GPS time: hour:%d min:%d sec:%d\n\r",
		parsed_nmea->hour, parsed_nmea->minute, parsed_nmea->seconds);
}

static char *parse_latitude(char *p, struct parsed_nmea_t *parsed_nmea,
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

		parsed_nmea->latitude_fixed = degree + minutes;
		parsed_nmea->latitude = degree / 100000 + minutes * 0.000006F;
		parsed_nmea->latitude_degrees = (parsed_nmea->latitude
		-100 * fast_trunc(parsed_nmea->latitude/100))/60.0;
		parsed_nmea->latitude_degrees +=
		 fast_trunc(parsed_nmea->latitude/100);
	}

	p = strchr(p, ',')+1;
	if (',' != *p) {
		if (p[0] == 'S')
			parsed_nmea->latitude_degrees *= -1.0;
		if (p[0] == 'N')
			parsed_nmea->lat = 'N';
		else if (p[0] == 'S')
			parsed_nmea->lat = 'S';
		else if (p[0] == ',')
			parsed_nmea->lat = 0;
		else
			*retval = false;
	}
	dbg_printf("GPS Latitude: %f\n\r",
		parsed_nmea->latitude_degrees);
	return p;
}

static char *parse_longitude(char *p, struct parsed_nmea_t *parsed_nmea,
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

		parsed_nmea->longitude_fixed = degree + minutes;
		parsed_nmea->longitude = degree / 100000 + minutes * 0.000006F;
		parsed_nmea->longitude_degrees = (parsed_nmea->longitude
		-100 * fast_trunc(parsed_nmea->longitude/100))/60.0;
		parsed_nmea->longitude_degrees +=
		 fast_trunc(parsed_nmea->longitude/100);
	}

	p = strchr(p, ',')+1;

	if (',' != *p) {

		if (p[0] == 'W') {
			parsed_nmea->longitude_degrees *= -1.0;
			parsed_nmea->lon = 'W';
		} else if (p[0] == 'E')
			parsed_nmea->lon = 'E';
		else if (p[0] == ',')
			parsed_nmea->lon = 0;
		else
			*retval = false;
	}
	dbg_printf("GPS Longitude: %f\n\r",
		parsed_nmea->longitude_degrees);
	return p;
}

static char *parse_GGA_param(char *p, struct parsed_nmea_t *parsed_nmea)
{
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->fix_quality = atoi(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->satellites = atoi(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->HDOP = atof(p);

	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->altitude = atof(p);

	p = strchr(p, ',')+1;
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->geoid_height = atof(p);

	dbg_printf("GPS fix quality indicator: %d\n\r",
		parsed_nmea->fix_quality);
	dbg_printf("GPS number of satellites in use: %d\n\r",
		parsed_nmea->satellites);
	dbg_printf("GPS HDOP: %f\n\r",
		parsed_nmea->HDOP);
	dbg_printf("GPS altitude: %f\n\r",
		parsed_nmea->altitude);
	dbg_printf("GPS geoidal height: %f\n\r",
		parsed_nmea->geoid_height);
	return p;
}

static char *parse_RMC_param(char *p, struct parsed_nmea_t *parsed_nmea)
{
	/* Parse Speed */
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->speed = atof(p);

	/* Parse Angle */
	p = strchr(p, ',')+1;
	if (',' != *p)
		parsed_nmea->angle = atof(p);

	p = strchr(p, ',')+1;
	if (',' != *p) {

		uint32_t fulldate = atof(p);
		parsed_nmea->day = fulldate / 10000;
		parsed_nmea->month = (fulldate % 10000) / 100;
		parsed_nmea->year = (fulldate % 100);

	}
	dbg_printf("GPS speed: %f\n\r",
		parsed_nmea->speed);
	dbg_printf("GPS angle: %f\n\r",
		parsed_nmea->angle);
	dbg_printf("GPS date: day:%d month:%d year:%d\n\r",
		parsed_nmea->day, parsed_nmea->month,
		parsed_nmea->year);
	return p;
}

static bool validate_nmea(char *nmea)
{
	uint8_t len = strlen(nmea);
	if (nmea[len-4] == '*') {
		uint16_t sum = parse_hex(nmea[len-3]) * 16;
		sum += parse_hex(nmea[len-2]);

		/* Validate the checksum */
		for (uint8_t i = 2; i < (len-4); i++)
			sum ^= nmea[i];

		if (sum != 0) {
			dbg_printf("Bad NMEA Checksum\n\r");
			return false;
		}
	}
	return true;
}

/**
 * \brief Receive and read new GPS NMEA and set the recvd_flag
 *
 * \retval true New NMEA was received successfully.
 * \retval false No new NMEA received.
 *
 */
static bool gps_new_NMEA_received()
{
	uint8_t *buffer = gps_text;
	uart_rx(uart, buffer, GPS_RX_BUFFER_SIZE, GPS_RCV_TIMEOUT_MS);
	int buffer_start_index = 0;
	int buffer_end_index = 0;
	for (int i = 0; i < GPS_RX_BUFFER_SIZE; i++) {
	  if (buffer[i] == '$')
		buffer_start_index = i;
	  else if (buffer[i] == '\n')
		buffer_end_index = i;
	}

	int32_t sz = buffer_end_index - buffer_start_index + 1;
	dbg_printf("sentenceSize calculate = %ld\r\n", sz);

	if (sz > 1)
		memcpy(sentence_buffer, &buffer[buffer_start_index], sz);
        dbg_printf("Sentence buffer=%s",sentence_buffer);
	recvd_flag = true;
	return recvd_flag;
}

/**
 * \brief Parse the received NMEA.
 * \detail Parse the received NMEA and store the parsed
 * values in the received structure format.
 *
 * \param[in] nmea Received NMEA which needs to be parsed.
 * \param[in] parsed_nmea  parsed values will be stored in this.
 *
 * \retval true NMEA was parsed successfully.
 * \retval false NMEA was not parsed successfully.
 */
static bool gps_parse_nmea(char *nmea, struct parsed_nmea_t *parsed_nmea)
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
		parse_time(bufptr, parsed_nmea);

		/* Parse Latitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_latitude(bufptr, parsed_nmea, &retval);
		if (retval == false)
			return false;

		/* Parse Longitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_longitude(bufptr, parsed_nmea, &retval);
		if (retval == false)
			return false;

		/* Parse other parameters */
		bufptr = parse_GGA_param(bufptr, parsed_nmea);
		return true;
	}
	if (strstr(nmea, "$GPRMC")) {

		/* Found RMC */
		char *bufptr = nmea;
		bool retval = true;

		/* Parse Time */
		bufptr = strchr(bufptr, ',')+1;
		parse_time(bufptr, parsed_nmea);

		bufptr = strchr(bufptr, ',')+1;
		if (bufptr[0] == 'A')
			parsed_nmea->fix = true;
		else if (bufptr[0] == 'V')
			parsed_nmea->fix = false;
		else
			return false;

		/* Parse Latitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_latitude(bufptr, parsed_nmea, &retval);
		if (retval == false)
			return false;

		/* Parse Longitude */
		bufptr = strchr(bufptr, ',')+1;
		bufptr = parse_longitude(bufptr, parsed_nmea, &retval);
		if (retval == false)
			return false;

		/* Parse GPRMC other parameters */
		bufptr = parse_RMC_param(bufptr, parsed_nmea);
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
	recvd_flag = false;
	return (char *)sentence_buffer;
}

bool gps_module_set_reporting(void)
{
	uint8_t buffer[20] = {0};

	if (!gps_tx(REVERT_NMEA, sizeof(REVERT_NMEA), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit REVERT_NMEA");
	  return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on REVERT_NMEA");
		return false;
	}

	if (!gps_tx(RMC_Off, sizeof(RMC_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit RMC_Off");
	  return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on RMC_Off");
		return false;
	}
	if (!gps_tx(VTG_Off, sizeof(VTG_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit VTG_Off");
		return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on VTG_Off");
		return false;
	}

	if (!gps_tx(GSA_Off, sizeof(GSA_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit GSA_Off");
		return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on GSA_Off");
		return false;
	}

	if (!gps_tx(GSV_Off, sizeof(GSV_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit GSV_Off");
		return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on GSV_Off");
		return false;
	}
	if (!gps_tx(GLL_Off, sizeof(GLL_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit GLL_Off");
		return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on GLL_Off");
		return false;
	}
	if (!gps_tx(ZDA_Off, sizeof(ZDA_Off), GPS_SEND_TIMEOUT_MS)) {
          dbg_printf("failed to xmit ZDA_Off");
		return false;
	}
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS)) {
	  dbg_printf("failed rcv on ZDA_Off");
		return false;
	}
        dbg_printf("ZOE set for GGA only\r\n");
	return true;
}

bool gps_module_init()
{

       /* on nomad, there is a reset line for the ZOE. It should
        never be used commonly, but reserved for extreme case
        as it will basically do a hard reset and things will have
        to be set up again
       */
        gpio_config_t zoe_reset_pin;
        pin_name_t zoe_reset_pin_name = GPS_RESET_PIN;
        zoe_reset_pin.dir = OUTPUT;
        zoe_reset_pin.pull_mode = PP_PULL_UP;
        zoe_reset_pin.speed = SPEED_VERY_HIGH;
        gpio_init(zoe_reset_pin_name, &zoe_reset_pin);
        gpio_write(zoe_reset_pin_name, PIN_HIGH);

	/* Configure UART for GPS */
	const struct uart_pins pins = {
		.tx = GPS_TX_PIN,
		.rx = GPS_RX_PIN,
		.rts = NC,
		.cts = NC
	};
	const uart_config config = {
		.baud = GPS_BAUD_RATE,
		.data_width = GPS_UART_DATA_WIDTH,
		.parity = GPS_UART_PARITY,
		.stop_bits = GPS_UART_STOP_BITS_1,
		.priority = GPS_UART_IRQ_PRIORITY,
		.irq = false
	};

	uart = uart_init(&pins, &config);
	if (uart == NO_PERIPH)
		return false;

	dbg_printf("GNSS: reset Zoe-8M, allow only $GPGGA\n\r");
	uint8_t buffer[20] = {0};

	if (!gps_tx(RCV_RESET, sizeof(RCV_RESET), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	sys_delay(2000); 

	recvd_flag   = false;

        if (!gps_module_set_reporting())
	  return false;
	if (!gps_tx(SAVE_Config, sizeof(SAVE_Config), GPS_SEND_TIMEOUT_MS))
	  return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
	  return false;
	return true;
}

bool gps_receive(struct parsed_nmea_t *parsedNEMA)
{
	if (!parsedNEMA)
		return false;

	if (gps_new_NMEA_received() == true) {

		/* Parse the new NMEA received
		 * and copy the parsed information */
		gps_parse_nmea(gps_last_NMEA(), parsedNEMA);
		return true;
	}
	return false;
}

bool gps_sleep(void)
{
	uint8_t buffer[20] = {0};

	if (uart_tx(uart, (uint8_t *)GGA_Off,
		 sizeof(GGA_Off), GPS_SEND_TIMEOUT_MS) != true)
		return false;

	if (uart_rx(uart, buffer, sizeof(buffer),
		 GPS_RCV_TIMEOUT_MS) != true)
		return false;

	return true;
}

bool gps_wake(void)
{

	uint8_t buffer[20] = {0};

	if (uart_tx(uart, (uint8_t *)GGA_On,
		 sizeof(GGA_On), GPS_SEND_TIMEOUT_MS) != true)
		return false;

	if (uart_rx(uart, buffer, sizeof(buffer),
		 GPS_RCV_TIMEOUT_MS) != true)
		return false;

	return true;
}

