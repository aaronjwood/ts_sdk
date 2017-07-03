/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include <stm32f4xx_hal.h>
#include <math.h>
#include "gps_hal.h"
#include "dbg.h"
#include "uart_hal.h"
#include "gpio_hal.h"
#include "ts_sdk_board_config.h"

#define CALLBACK_TRIGGER_MARK	((buf_sz)(GPS_RX_BUFFER_SIZE * ALMOST_FULL_FRAC))
#define ALMOST_FULL_FRAC	0.6	/* Call the receive callback once this
					 * fraction of the buffer is full.
					 */
#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define MICRO_SEC_MUL		1000000
#define TIMEOUT_BYTE_US		CEIL((12 * MICRO_SEC_MUL), BAUD_RATE)

//static UART_HandleTypeDef comm_uart;
static periph_t uart;
static volatile bool received_gps_text;
static uint8_t gps_text[GPS_RX_BUFFER_SIZE];
static uint8_t sentenceBuffer[GPS_SENTENCE_BUFFER_SIZE];

volatile bool recvdflag;
bool paused;

char RCV_RESET[] = {0xB5, 0X62, 0x06, 0x01, 0x04,0x00, 0x01, 0x01,0x00,0x16,0x91};
char RMC_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X04, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X03, 0X3F};
char VTG_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X05, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X04, 0X46};
char GSA_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X02, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X01, 0X31};
char GSV_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X02, 0X38};
char GLL_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X2A};
char ZDA_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X07, 0X5B};
char GGA_Off[] = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0XFF, 0X23};
char GGA_On[]  = { 0XB5, 0X62, 0X06, 0X01, 0X08, 0X00, 0XF0, 0X00, 0X00, 0X01, 0X01, 0X00, 0X00, 0X00, 0X01, 0X2C};
//char GPS_On[] = {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00,0x09, 0x00, 0x17, 0x76};
//char GPS_Off[] = {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00,0x08, 0x00, 0x16, 0x74};
//char setPSM[] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };


/*
 * Initialize the UART hardware on the beduin board with the following settings:
 * Baud rate       : 9600 bps
 * Data width      : 8 bits
 * Parity bits     : None
 * Stop bits       : 1
 * HW flow control : None (beduin only has rx/tx)
 *
 * Also, configure the idle timeout delay in number of characters.
 * When the line remains idle for 'numWaitChar' characters after receiving the last
 * character, the driver invokes the receive callback.
 *
 * Parameters:
 *
 *      numWaitChar - number of characters to wait. The actual duration is based on the
 *      current UART baud rate.
 *
 * Returns:
 *      True - If initialization was successful.
 *      Fase - If initialization failed.
 */
bool gps_module_init(uint8_t numWaitChar)
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
		.priority = GPS_UART_IRQ_PRIORITY
	};

	uart = uart_init(&pins, &config);
        ASSERT(uart != NO_PERIPH);

	dbg_printf("GNSS: reset Neo-6M, allow only $GPGGA\n");
        uint8_t buffer[20];

	gps_send_command(RCV_RESET);
	gps_receive_command(buffer);
	HAL_Delay(2000);
	//gps_send_command(setPSM);
        //gps_receive_command(buffer);
	gps_send_command(RMC_Off);
	gps_receive_command(buffer);
        gps_send_command(VTG_Off);
       	gps_receive_command(buffer);
        gps_send_command(GSA_Off);
        gps_receive_command(buffer);
        gps_send_command(GSV_Off);
        gps_receive_command(buffer);
        gps_send_command(GLL_Off);
        gps_receive_command(buffer);
        gps_send_command(ZDA_Off);
	gps_receive_command(buffer);
	recvdflag   = false;
	return true;
}

bool gps_tx(uint8_t *data, buf_sz size, uint16_t timeout_ms)
{
	if (!data)
		return false;

	if(uart_tx(uart, data, size, timeout_ms) != true)
		return false;

	return true;
}

bool gps_rx(uint8_t *data, buf_sz size, uint16_t timeout_ms)
{
        if (!data)
                return false;

        if(uart_rx(uart, data, size, timeout_ms) != true)
                return false;

        return true;
}
static int fastCeil(float num) 
{

	int inum = (int)num;
	if (num == (float)inum) {
		return inum;
	}
	return inum + 1;
}

static int fastFloor(double x)
{

	int xi = (int)x;
	return x < xi ? xi - 1 : xi;
}

static int fastTrunc(double d)
{ 
	return (d>0) ? fastFloor(d) : fastCeil(d) ; 
}

double floatmod(double a, double b)
{
	return (a - b * fastFloor(a / b));
}

uint8_t parseHex(char c)
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

bool gps_parse_nmea(char *nmea, struct parsed_nmea_t *parsedNMEA)
{
	/* Look for the nmea */
	if (nmea[strlen(nmea)-4] == '*') {
		uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
		sum += parseHex(nmea[strlen(nmea)-2]);

		/* Validate the checksum */
		for (uint8_t i=2; i < (strlen(nmea)-4); i++) {
			sum ^= nmea[i];
		}

		if (sum != 0) {
			dbg_printf("Bad NMEA Checksum");
			return false;
		}	
	}

	int32_t degree;
	long minutes;
	char degreebuff[10];

	/* Look for a few common sentences */
	if (strstr(nmea, "$GPGGA")) {

		/* Found GGA */
		char *p = nmea;

		/* Parse Time */
		p = strchr(p, ',')+1;
		float timef = atof(p);
		uint32_t time = timef;
		parsedNMEA->hour = time / 10000;
		parsedNMEA->minute = (time % 10000) / 100;
		parsedNMEA->seconds = (time % 100);

		parsedNMEA->milliseconds = floatmod((double) timef, 1.0) * 1000;

		/* Parse Latitude */
		p = strchr(p, ',')+1;
    		if (',' != *p) {
			strncpy(degreebuff, p, 2);
			p += 2;
			degreebuff[2] = '\0';
			degree = atol(degreebuff) * 10000000;
			strncpy(degreebuff, p, 2); // minutes
			p += 3; // skip decimal point
			strncpy(degreebuff + 2, p, 4);
			degreebuff[6] = '\0';
			minutes = 50 * atol(degreebuff) / 3;

			parsedNMEA->latitude_fixed = degree + minutes;
			parsedNMEA->latitude = degree / 100000 + minutes * 0.000006F;
			parsedNMEA->latitudeDegrees = (parsedNMEA->latitude
			-100* fastTrunc(parsedNMEA->latitude/100))/60.0;
			parsedNMEA->latitudeDegrees += fastTrunc(parsedNMEA->latitude/100);
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

			if (p[0] == 'S') parsedNMEA->latitudeDegrees *= -1.0;
			if (p[0] == 'N') parsedNMEA->lat = 'N';
			else if (p[0] == 'S') parsedNMEA->lat = 'S';
			else if (p[0] == ',') parsedNMEA->lat = 0;
			else return false;
		}

		/* Parse Longitude */
		p = strchr(p, ',')+1;
		if (',' != *p) {

			strncpy(degreebuff, p, 3);
			p += 3;
			degreebuff[3] = '\0';
			degree = atol(degreebuff) * 10000000;
			strncpy(degreebuff, p, 2); // minutes
			p += 3; // skip decimal point
			strncpy(degreebuff + 2, p, 4);
			degreebuff[6] = '\0';
			minutes = 50 * atol(degreebuff) / 3;

			parsedNMEA->longitude_fixed = degree + minutes;
			parsedNMEA->longitude = degree / 100000 + minutes * 0.000006F;
			parsedNMEA->longitudeDegrees = (parsedNMEA->longitude
			-100* fastTrunc(parsedNMEA->longitude/100))/60.0;
			parsedNMEA->longitudeDegrees += fastTrunc(parsedNMEA->longitude/100);

		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

			if (p[0] == 'W') parsedNMEA->longitudeDegrees *= -1.0;
			if (p[0] == 'W') parsedNMEA->lon = 'W';
			else if (p[0] == 'E') parsedNMEA->lon = 'E';
			else if (p[0] == ',') parsedNMEA->lon = 0;
			else return false;
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

			parsedNMEA->fixquality = atoi(p);
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

			parsedNMEA->satellites = atoi(p);
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

			parsedNMEA->HDOP = atof(p);
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {

		parsedNMEA->altitude = atof(p);

		}	

		p = strchr(p, ',')+1;
		p = strchr(p, ',')+1;

		if (',' != *p) {

			parsedNMEA->geoidheight = atof(p);

		}		
		return true;
	}
  	if (strstr(nmea, "$GPRMC")) {

		/* Found RMC */
		char *p = nmea;

		/* Parse Time */
		p = strchr(p, ',')+1;
		float timef = atof(p);
		uint32_t time = timef;

		parsedNMEA->hour = time / 10000;
		parsedNMEA->minute = (time % 10000) / 100;
		parsedNMEA->seconds = (time % 100);
		parsedNMEA->milliseconds = floatmod(timef, 1.0) * 1000;

		p = strchr(p, ',')+1;
    		if (p[0] == 'A') 
      			parsedNMEA->fix = true;
    		else if (p[0] == 'V')
			parsedNMEA->fix = false;
		else
			return false;

		/* Parse Latitude */
		p = strchr(p, ',')+1;
		if (',' != *p) {
			strncpy(degreebuff, p, 2);
			p += 2;
			degreebuff[2] = '\0';

			long degree = atol(degreebuff) * 10000000;
			strncpy(degreebuff, p, 2); // minutes
			p += 3; // skip decimal point
			strncpy(degreebuff + 2, p, 4);
			degreebuff[6] = '\0';

			long minutes = 50 * atol(degreebuff) / 3;
			parsedNMEA->latitude_fixed = degree + minutes;
			parsedNMEA->latitude = degree / 100000 + minutes * 0.000006F;
			parsedNMEA->latitudeDegrees = (parsedNMEA->latitude
			-100* fastTrunc(parsedNMEA->latitude/100))/60.0;
			parsedNMEA->latitudeDegrees += fastTrunc(parsedNMEA->latitude/100);
		}

		p = strchr(p, ',')+1;
		if (',' != *p) {
			if (p[0] == 'S') parsedNMEA->latitudeDegrees *= -1.0;
			if (p[0] == 'N') parsedNMEA->lat = 'N';
			else if (p[0] == 'S') parsedNMEA->lat = 'S';
			else if (p[0] == ',') parsedNMEA->lat = 0;
			else return false;
		}	

		/* Parse Longitude */
		p = strchr(p, ',')+1;
		if (',' != *p) {
			strncpy(degreebuff, p, 3);
			p += 3;
			degreebuff[3] = '\0';
			degree = atol(degreebuff) * 10000000;
			strncpy(degreebuff, p, 2); // minutes
			p += 3; // skip decimal point
			strncpy(degreebuff + 2, p, 4);
			degreebuff[6] = '\0';
			minutes = 50 * atol(degreebuff) / 3;

			parsedNMEA->longitude_fixed = degree + minutes;
			parsedNMEA->longitude = degree / 100000 + minutes * 0.000006F;
			parsedNMEA->longitudeDegrees = (parsedNMEA->longitude
			-100* fastTrunc(parsedNMEA->longitude/100))/60.0;
			parsedNMEA->longitudeDegrees += fastTrunc(parsedNMEA->longitude/100);
      		}

		p = strchr(p, ',')+1;
		if (',' != *p) {
			if (p[0] == 'W') parsedNMEA->longitudeDegrees *= -1.0;
			if (p[0] == 'W') parsedNMEA->lon = 'W';
			else if (p[0] == 'E') parsedNMEA->lon = 'E';
			else if (p[0] == ',') parsedNMEA->lon = 0;
			else return false;
      		}

		/* Parse Speed */
		p = strchr(p, ',')+1;
		if (',' != *p) {
			parsedNMEA->speed = atof(p);
		}

		/* Parse Angle */
		p = strchr(p, ',')+1;
		if (',' != *p) {
			parsedNMEA->angle = atof(p);
		}

		p = strchr(p, ',')+1;
		if (',' != *p)
		{
			uint32_t fulldate = atof(p);
			parsedNMEA->day = fulldate / 10000;
			parsedNMEA->month = (fulldate % 10000) / 100;
			parsedNMEA->year = (fulldate % 100);
		}

		/* Do not parse the remaining, yet! */
		return true;
	}

	return false;
}

bool gps_wait_for_sentence(const char *wait4me, uint8_t max)
{

	char str[20];
	uint8_t i=0;

	while (i < max) {

		if (gps_new_NMEA_received()) { 
			char *nmea = gps_last_NMEA();
			strncpy(str, nmea, 20);
			str[19] = 0;
			i++;

		if (strstr(str, wait4me))
			return true;
		}
	}

	return false;
}


void gps_send_command(const char *str) 
{

	ASSERT(gps_tx((uint8_t *)str, sizeof(str), GPS_SEND_TIMEOUT_MS) ==true);
}

void gps_receive_command(uint8_t *buffer)
{

        ASSERT(gps_rx(buffer, sizeof(buffer), GPS_SEND_TIMEOUT_MS) ==true);
}

void gps_read_NMEA(uint8_t *buffer)
{
	uart_rx(uart,buffer,GPS_RX_BUFFER_SIZE, GPS_RCV_TIMEOUT_MS);

	int bufferStartIndex = 0;
	int bufferEndIndex = 0;
	for (int i = 0; i < GPS_RX_BUFFER_SIZE; i++) {
		if (buffer[i] == '$') {
			bufferStartIndex = i;
		} else if (buffer[i] == '\n') {
			bufferEndIndex = i;
		}
	}

	int32_t sz = (&buffer[bufferEndIndex] - &buffer[bufferStartIndex]) +1;
	dbg_printf("sentenceSize calculate = %ld\r\n",sz);
	memcpy(sentenceBuffer, &buffer[bufferStartIndex], sz);

	recvdflag = true;
}

bool gps_new_NMEA_received(void) 
{
	gps_read_NMEA(gps_text);
	return recvdflag;
}


void gps_pause(bool p) 
{
	paused = p;
}

char* gps_last_NMEA(void) 
{

	recvdflag = false;
	//dbg_printf("sentence = %s\r\n",sentenceBuffer);
	return (char *)sentenceBuffer;
}

void gps_sleep(void)
{
	uint8_t buffer[20];
	uint32_t result = 0;
	uart_tx(uart, (uint8_t *)GGA_Off, sizeof(GGA_Off), GPS_SEND_TIMEOUT_MS);
	result = uart_rx(uart,buffer,20, GPS_SEND_TIMEOUT_MS);
	dbg_printf("GPS stopping reporting = %ld\r\n",result);
}

void gps_wake(void)
{
	uint8_t buffer[20];
	uint32_t result = 0;
	uart_tx(uart, (uint8_t *)GGA_On, sizeof(GGA_On), GPS_SEND_TIMEOUT_MS);
	result = uart_rx(uart,buffer,20, GPS_SEND_TIMEOUT_MS);
	dbg_printf("GPS starting reporting = %ld\r\n",result);
}

