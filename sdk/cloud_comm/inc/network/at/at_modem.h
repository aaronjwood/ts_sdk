/* Copyright (C) 2017 Verizon. All rights reserved. */

/**
 * \file at_modem.h
 * \brief API that performs modem specific functions.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 */
#ifndef AT_MODEM_H
#define AT_MODEM_H

/**
 * \brief Initialize the modem module and associated hardware such as reset pins.
 * \note This routine must be called before all others in this module.
 */
bool at_modem_init(void);

/**
 * \brief Perform a hardware reset of the modem.
 */
void at_modem_hw_reset(void);

/**
 * \brief Perform a software reset of the modem.
 * \retval true Software reset was successful
 * \retval false Software reset failed
 */
bool at_modem_sw_reset(void);

/**
 * \brief Configure the modem. This involves performing any non-communications
 * initialization.
 */
bool at_modem_configure(void);

/**
 * \brief Query the modem for proper registration with the network.
 * \retval true If network registration was successful
 * \retval false On failure
 */
bool at_modem_query_network(void);

/**
 * \brief Process a NULL terminated URC string.
 *
 * \details This routine should be used to attempt to process URCs that are not
 * handled by the communication layers. For example, SMS and TCP related URCs
 * are processed in the respective modules whereas calls to this routine could
 * handle LTE network connection related URCs.
 *
 * \param[in] urc NULL terminated URC string
 *
 * \retval true If URC was processed successfully
 * \retval false If URC was not relevant and has not been processed
 */
bool at_modem_process_urc(const char *urc);

/**
 * \brief Retreieve the IMEI of the modem as a NULL terminated string.
 * \details The IMEI string retrieved is guaranteed to be 16 bytes long (15
 * digits + 1 terminating NULL byte). For example: "123456789012345"
 *
 * \param[out] imei Pointer to NULL character buffer to hold the retrieved IMEI.
 * Must be at least 16 bytes long.
 *
 * \retval true IMEI retrieved successfully
 * \retval false IMEI retrieval failed
 */
bool at_modem_get_imei(char *imei);

/**
 * \brief Retrieve the signal strength as a NULL terminated string.
 * \details The string is expected to be at most 11 bytes long (10 bytes
 * describing the signal strength + 1 terminating NULL byte). For example:
 * "<=-113 dBm"
 *
 * \param[out] ss Pointer to a NULL character buffer to hold the retrieved signal
 * strength. Must be at least 11 bytes long. If signal strength is unknown,
 * "SIGUNKWN" is returned.
 *
 * \retval true Signal strength retrieved successfully
 * \retval false Failed to retrieve signal strength
 */
bool at_modem_get_ss(char *ss);
#endif
