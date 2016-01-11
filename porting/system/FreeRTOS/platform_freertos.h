/**
 * @file platform_freertos.h
 *
 * This is the platform interface for freertos
 * Platforms that use freertos shall implement it
 *
 * */

#ifndef INC_PLATFORM_FREERTOS_H_
#define INC_PLATFORM_FREERTOS_H_

#include <lib/type.h>
#include <porting/system_port.h>

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

#endif /* INC_PLATFORM_FREERTOS_H_ */
