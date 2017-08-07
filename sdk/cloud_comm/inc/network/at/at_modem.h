/* Copyright (C) 2017 Verizon. All rights reserved. */

/**
 * \file at_modem.h
 * \brief API that performs modem specific functions.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \details Routines beginning with at_modem_get_* could potentially disrupt active
 * TCP connections. In the current version, they should be used sparingly while
 * a TCP connection is active.
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
 * \brief Query the modem for proper registration with the network.
 * \retval true Registered to the network
 * \retval false Not registered to the network / query failed
 */
bool at_modem_get_nstat(void);

/**
 * \brief Retreieve the IMEI of the modem as a NULL terminated string.
 *
 * \param[out] imei Pointer to NULL terminated buffer to hold the retrieved IMEI.
 * Buffer must be at least 16 bytes long (including terminating NULL character).
 *
 * \retval true IMEI retrieved successfully
 * \retval false IMEI retrieval failed. Buffer contents are invalid.
 */
bool at_modem_get_imei(char *imei);

/**
 * \brief Retrieve the signal strength as a NULL terminated string.
 * \details.
 *
 * \param[out] ss Pointer to a NULL terminated buffer to hold the retrieved signal
 * strength. Buffer must be at least 11 bytes long (including terminating NULL
 * character). For example, "<=-113 dBm". If signal strength is unknown,
 * "SIGUNKWN" is returned.
 *
 * \retval true Signal strength retrieved successfully
 * \retval false Failed to retrieve signal strength. Buffer contents are invalid.
 *
 * \note The signal strenght might show up as unknown right after the modem boots.
 */
bool at_modem_get_ss(char *ss);

/**
 * \brief Retrieve the IP address associated with the modem as a NULL terminated
 * string.
 *
 * \param[in] s_id Socket ID whose IP address needs to be retrieved
 * \param[out] ip Pointer to a NULL terminated buffer to hold the retrieved IP
 * address. Buffer must be at least 16 bytes long (including terminating NULL
 * character).
 *
 * \retval true IP address was successfully retrieved
 * \retval false Failed to retrieve the IP address. Buffer contents are invalid.
 */
bool at_modem_get_ip(int s_id, char *ip);

/**
 * \brief Retrieve the ICCID of the SIM card as a NULL terminated string.
 *
 * \param[out] iccid Pointer to a NULL terminated buffer to hold the retrieved
 * SIM ICCID. Buffer must be at least 23 bytes long (including the terminating
 * NULL character).
 *
 * \retval true ICCID was successfully retrieved
 * \retval false Failed to retrieve the ICCID. Buffer contents are invalid.
 */
bool at_modem_get_iccid(char *iccid);

/**
 * \brief Retrieve the time and timezone information as a NULL terminated string.
 * \details Format of the output is: yy/mm/dd,hh:mm:ss+TZ
 *
 * \param[out] ttz Pointer to a NULL terminated buffer to hold the retrieved
 * time and timezone information. Buffer must be at least 21 bytes long
 * (including the terminating NULL character).
 *
 * \retval true Time and Timezone information retrieved successfully.
 * \retval false Failed to retrieve the time and timezone information. Buffer
 * contents are invalid.
 */
bool at_modem_get_ttz(char *ttz);

/**
 * \brief Retrieve the manufacturer information as a NULL terminated string.
 *
 * \param[out] man Pointer to a NULL terminated buffer to hold the retrieved
 * manufacturer information.
 *
 * \retval true Manufacturer information successfully retrieved.
 * \retval false Failed to retrieved manufacturer information. Buffer contents
 * are invalid.
 */
bool at_modem_get_man_info(char *man);

/**
 * \brief Retrieve the model information as a NULL terminated string.
 *
 * \param[out] mod Pointer to a NULL terminated buffer to hold the retrieved
 * model information.
 *
 * \retval true Model information successfully retrieved.
 * \retval false Failed to retrieved model information. Buffer contents are
 * invalid.
 */
bool at_modem_get_mod_info(char *mod);

/**
 * \brief Retrieve the firmware version as a NULL terminated string.
 *
 * \param[out] fwver Pointer to a NULL terminated buffer to hold the retrieved
 * firmware version.
 *
 * \retval true Firmware version successfully retrieved.
 * \retval false Failed to retrieve firmware version. Buffer contents are invalid.
 */
bool at_modem_get_fwver(char *fwver);

/**
 * \brief Retrieve the IMSI as a NULL terminated string.
 *
 * \param[out] imsi Pointer to a NULL terminated buffer to hold the retrieved
 * IMSI. Buffer must be at least 17 bytes long (including the terminating NULL
 * character).
 *
 * \retval true IMSI was retrieved.
 * \retval false Failed to retrieve IMSI. Buffer contents are invalid.
 */
bool at_modem_get_imsi(char *imsi);

#endif
