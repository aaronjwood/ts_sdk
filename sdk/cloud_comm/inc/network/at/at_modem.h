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

#endif
