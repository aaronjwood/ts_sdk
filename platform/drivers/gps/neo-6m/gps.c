/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gps_hal.h"
#include "sys.h"
#include "dbg.h"
#include "uart_hal.h"
#include "ts_sdk_board_config.h"

#define GPS_RX_BUFFER_SIZE      2048
#define GPS_SENTENCE_BUFFER_SIZE 128
#define GPS_SEND_TIMEOUT_MS     2000
#define GPS_RCV_TIMEOUT_MS      5000

typedef uint16_t buf_sz;
static periph_t uart;
static volatile bool received_gps_text;
static uint8_t gps_text[GPS_RX_BUFFER_SIZE];
static uint8_t sentenceBuffer[GPS_SENTENCE_BUFFER_SIZE];

volatile bool recvdflag;
static const uint8_t RCV_RESET[] = { 0xB5, 0x62, 0x06, 0x04, 0x04,
					 0x00, 0x00, 0x01, 0x01,
					 0x00, 0x10, 0x6b };

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
	uart_rx(uart, buffer, GPS_RX_BUFFER_SIZE, GPS_RCV_TIMEOUT_MS);
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
	memcpy(sentenceBuffer, &buffer[bufferStartIndex], sz);

	recvdflag = true;
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

bool gps_module_init()
{
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
		.hw_flow_ctrl = false,
		.irq = false
	};

	uart = uart_init(&pins, &config);
	if (uart == NO_PERIPH)
		return false;

	dbg_printf("GNSS: reset Neo-6M, allow only $GPGGA\n");
	uint8_t buffer[20] = {0};

	if (!gps_tx(RCV_RESET, sizeof(RCV_RESET), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	sys_delay(2000);

	if (!gps_tx(RMC_Off, sizeof(RMC_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	if (!gps_tx(VTG_Off, sizeof(VTG_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	if (!gps_tx(GSA_Off, sizeof(GSA_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	if (!gps_tx(GSV_Off, sizeof(GSV_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	if (!gps_tx(GLL_Off, sizeof(GLL_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	if (!gps_tx(ZDA_Off, sizeof(ZDA_Off), GPS_SEND_TIMEOUT_MS))
		return false;
	if (!gps_rx(buffer, sizeof(buffer), GPS_RCV_TIMEOUT_MS))
		return false;

	recvdflag   = false;
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

