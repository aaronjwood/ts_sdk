/*
 * file gps_hal.h
 * Copyright (c) 2017 Verizon. All rights reserved.
 */

#ifndef GPS_HAL_H
#define GPS_HAL_H

#include <stdbool.h>
#include <stdint.h>

struct parsed_nmea_t {
  uint8_t hour; 
  uint8_t minute; 
  uint8_t seconds; 
  uint8_t year; 
  uint8_t month; 
  uint8_t day;
  uint16_t milliseconds;
  float latitude;
  float longitude;
  int32_t latitude_fixed;
  int32_t longitude_fixed;
  float latitudeDegrees;
  float longitudeDegrees;
  float geoidheight;
  float altitude;
  float speed;
  float angle;
  float magvariation;
  float HDOP;
  char lat;
  char lon;
  char mag;
  bool fix;
  uint8_t fixquality;
  uint8_t satellites;
};

#define GPS_RX_BUFFER_SIZE	2048
#define GPS_SENTENCE_BUFFER_SIZE 128

typedef uint16_t buf_sz;

#define GPS_INV_PARAM	-1	/* Invalid parameter. */

#define GPS_EN_HW_CTRL	true	/* Enable hardware flow control. */
#define GPS_DIS_HW_CTRL	false	/* Disable hardware flow control. */

#define GPS_READ_ERR	0x80
//#define GPS_EVENT_RX_OVERFLOW  0x40
#define GPS_SEND_TIMEOUT_MS	2000
#define GPS_RCV_TIMEOUT_MS      5000
#define MAXWAITSENTENCE	5

/*
 * Initialize the UART hardware
 * Configure the idle timeout delay in number of characters.
 * When the line remains idle for 'numWaitChar' characters after receiving the last
 * character, the driver invokes the receive callback.
 *
 * Parameters:
 *
 * 	numWaitChar - number of characters to wait. The actual duration is based on the
 * 	current UART baud rate.
 *
 * Returns:
 * 	True - If initialization was successful.
 * 	Fase - If initialization failed.
 */
bool gps_module_init(uint8_t numWaitChar);

/*
 * Send data over the UART. This is a blocking send. In case the modem blocks
 * the flow via hardware flow control, the call will block for at most 'timeout_ms'
 * milliseconds.
 *
 * Parameters:
 * 	data - Pointer to the data to be sent.
 * 	size - Number of bytes to send
 * 	timeout_ms - Total number of milliseconds to wait before giving up in
 * 	case of a busy channel.
 *
 * Returns:
 * 	True - If the data was sent out successfully.
 * 	False - Send aborted due to timeout or null pointer provided for data.
 */
bool gps_tx(uint8_t *data, buf_sz size, uint16_t timeout_ms);
bool gps_rx(uint8_t *data, buf_sz size, uint16_t timeout_ms);

void gps_send_command(const char *str);

void gps_receive_command(uint8_t *buffer);

void gps_pause(bool p);

char* gps_last_NMEA(void) ;

bool gps_wait_for_sentance(const char *wait, uint8_t max);

bool gps_new_NMEA_received(void);

char* gps_last_NMEA(void);

bool gps_parse_nmea(char *nmea, struct parsed_nmea_t *parsedNEMA);

void gps_sleep(void);

void gps_wake(void);

#endif
