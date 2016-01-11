/**
 * @file platform_qmx.h
 *
 * @copyright
 * Please read the Exosite Copyright: @ref copyright
 *
 * @if Authors
 * @authors
 *   - Szilveszter Balogh (szilveszterbalogh@exosite.com)
 *   - Zoltan Ribi (zoltanribi@exosite.com)
 * @endif
 *
 * @brief Platform interface for MQX
 **/
#ifndef ER_SDK_PORTING_SYSTEM_MQX_PLATFORM_QMX_H_
#define ER_SDK_PORTING_SYSTEM_MQX_PLATFORM_QMX_H_

/**
 * Return the serial number of the platform
 *
 * @return Serial number as a string
 **/
const char* platform_get_sn(void);

/**
 * Returns the CIK stored in the non-volatile storage of this platform
 *
 * @param[out] cik The CIK stored in non-volatile storage
 * @param[in] len  The length of the output buffer for CIK
 *
 * @return ERR_SUCCESS on success ERR_FAILURE otherwise
 **/
int platform_get_cik(char *cik, size_t len);

/**
 * Return the CIK stored in the non-volatile storage of this platform
 * param[in] cik The cik in C-string format
 *
 * @return ERR_SUCCESS on success ERR_FAILURE otherwise
 **/
int platform_set_cik(const char *cik);

/**
 * Reset the board
 * */
void platform_reset(void);

#endif /* ER_SDK_PORTING_SYSTEM_MQX_PLATFORM_QMX_H_ */
