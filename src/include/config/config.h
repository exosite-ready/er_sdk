/**
 * @file config.h
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
 * @brief This header shall be included in SDK core files if access to config is needed
 *
 * This header includes all the public and private config files
 *
 **/

#ifndef ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_H_
#define ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_H_

#include "../../../porting/config_port.h"
#include "../sdk_config.h"

#ifdef HAVE_CONFIG_H

#if (CONFIG_SECURITY==cfg_cyassl)
#include "config_cyassl.h"
#endif

#endif

#if (CONFIG_SECURITY==cfg_external) && defined (CONFIG_EXTERNAL_CONFIG)
#define INCLUDE_PATH_OF_EXTERNAL_CONFIG    <CONFIG_EXTERNAL_CONFIG>
#include INCLUDE_PATH_OF_EXTERNAL_CONFIG
#endif


#if (CONFIG_SECURITY==cfg_mbedtls)
#include "config_mbedtls.h"
#endif

#endif /* ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_H_ */
