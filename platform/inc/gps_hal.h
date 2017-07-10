/**
 * \file gps_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for GPS
 * \details This header defines a platform independent API
 * to read and write over the GPS. All sending operations
 * are blocking.
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

typedef uint16_t buf_sz;

/**
 * \brief Initialize the GPS module
 * \details Configure and initialize the required drivers for GPS
 * Setup NMEA protocol by sending NMEA messages/sentances.
 *
 * \retval true Initialization was successful.
 * \retval false Initialization failed.
 */
bool gps_module_init();

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
bool gps_tx(uint8_t *data, buf_sz size, uint16_t timeout_ms);

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
bool gps_rx(uint8_t *data, buf_sz size, uint16_t timeout_ms);

/**
 * \brief Send GPS command over GPS. 
 *
 * \param[in] buffer command to be sent
 *\param[in] size Size of command 
 * 
 */
void gps_send_command(const uint8_t *buffer, uint8_t size);

/**
 * \brief Receive GPS command 
 *
 * \param[in] buffer Buffer to receive the data from GPS
 * \param[in] size Size of receive buffer 
 *
 */
void gps_receive_command(uint8_t *buffer, uint8_t size);

/**
 * \brief Wait for a perticular string in received number of NMEA 
 *
 * \param[in] wait string for which its waiting
 * \param[in] max number of NMEA 
 *
 * \retval true wait string was found within max NMEA
 * \retval false wait string was not found within max NMEA 
 */
bool gps_wait_for_sentence(const char *wait, uint8_t max);

/**
 * \brief Receive and read new GPS NMEA and set the recvd flag 
 *
 * \retval true New NMEA was received successfully.
 * \retval false No new NMEA received.
 *
 */
bool gps_new_NMEA_received(void);

/**
 * \brief Return the last GPS NMEA received 
 *
 * \retval last NMEA data pointer.
 *
 */
char* gps_last_NMEA(void);

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
bool gps_parse_nmea(char *nmea, struct parsed_nmea_t *parsedNEMA);

/**
 * \brief Stops GPS reporting. 
 *
 */
void gps_sleep(void);

/**
 * \brief Restart GPS reporting. 
 *
 */
void gps_wake(void);

#endif
