/**
 * @file error.h
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
 * @brief This file contains the system wide error numbers
 *
 **/
#ifndef EXOSITE_INC_ERROR_H_
#define EXOSITE_INC_ERROR_H_

#include <stdint.h>
#include <stdlib.h>

enum {
    ERR_SUCCESS =          0,
    ERR_FAILURE =         -1, /* Unspecified error condition */
    ERR_WOULD_BLOCK =     -2, /* The operation would block */
    ERR_NO_MEMORY =       -3, /* Out of memory */
    ERR_NOT_IMPLEMENTED = -4, /* Not implemented function or feature */
    ERR_NOT_APPLICABLE =  -11, /* Not applicable function on the current platform. e.g missing hardware component */
    ERR_TIMEOUT =         -13,
    ERR_OVERFLOW =        -14,
    ERR_NOT_FOUND =       -15,
    ERR_BAD_ARGS =        -16,
    ERR_BUSY     =        -17, /* Device or resource busy */
    ERR_INVALID_FORMAT =  -18, /* Parsing a result resulted in some format error */
    ERR_COULD_NOT_SEND =  -19, /* Could not send request */
    ERR_NO_RESOURCE    =  -20, /* There is not enough resource to perform request */
    ERR_DUPLICATE      =  -21, /*  */
    ERR_NO_NET_DEV     =  -22, /* No network device */

    /* Exosite server - client communication error codes*/

    ERR_NO_CONTENT             = -101, /* There is no valid content. (e.g. Empty data source) */
    ERR_UNCHANGED              = -102,
    ERR_NO_CIK                 = -103, /* There is no valid CIK */
    ERR_CIK_NOT_VALID          = -104,
    ERR_REQUEST_ABORTED        = -105, /* There is no valid content. (e.g. Empty data source) */
    ERR_ACTIVATE_CONFLICT      = -106, /* The device is already activated or was disabled */
    ERR_ACTIVATE_NOT_FOUND     = -107, /* No such device exist on the server */

    /* Network error codes */
    ERR_NET_TIMEDOUT =    -205,   /*Network device should return the ERR_NET_* error codes*/
    ERR_NET_CLOSED =      -206,
    ERR_NET_ABORTED =     -207,
    ERR_NET_OVERFLOW =    -208,
    ERR_NET_DNS_ERROR =   -209,
};

enum {
    ERR_MODULE_CYASSL                 = 1
};

static inline int32_t map_error_code(int32_t module, int32_t error)
{
    int32_t status = -100000 * module - abs(error);
    return status;
}

#endif /* EXOSITE_INC_ERROR_H_ */
