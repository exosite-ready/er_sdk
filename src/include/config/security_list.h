/**
 * @file security_list.h
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
 * @brief Identifiers for the security libraries
 **/
#ifndef ER_SDK_SRC_INCLUDE_CONFIG_SECURITY_LIST_H_
#define ER_SDK_SRC_INCLUDE_CONFIG_SECURITY_LIST_H_

#define cfg_no_security    (0x00)
#define cfg_cyassl         (0x01)
#define cfg_mbedtls        (0x02)
#define cfg_external       (0x03)

#endif /* ER_SDK_SRC_INCLUDE_CONFIG_SECURITY_LIST_H_ */
