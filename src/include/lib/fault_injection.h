/**
 * @file fault_injecton.h
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
 * @brief This file contains the system wide fault injection types
 *
 **/

#ifndef ER_SDK_SRC_LIB_FAULT_INJECTION_H_
#define ER_SDK_SRC_LIB_FAULT_INJECTION_H_


enum fault_type {
    FT_NO_ERROR,
    FT_STANDARD_ERROR,
    FT_ERROR,
    FT_ERROR_AFTER_INIT,
    FT_TEMPORARY_ERROR,
    FT_TIMEOUT,
    FT_TIMEOUT_AFTER_INIT,
};


#endif /* ER_SDK_SRC_LIB_FAULT_INJECTION_H_ */
