/**
 * @file sdk_config.h
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
 * @brief This header adds private configuration settings based on
 *        the public settings in config_port.h
 *
 **/
#ifndef ER_SDK_SRC_INCLUDE_SDK_CONFIG_H_
#define ER_SDK_SRC_INCLUDE_SDK_CONFIG_H_

#include "../../porting/config_port.h"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "er_sdk"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Exosite Ready SDK ?.?.?"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "er_sdk"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "?.?.?"

/* Private configuration for Heap Management */

 /*
  *  If USE_MEM_ID is defined the memory allocation subsytem will use
  * the filename as an identifier for a block for all calls from that file
  */
/*#define USE_MEM_ID*/

/*
 * If PRINT_MEM_BLOCKS is defined then the print_memory_stats function will also
 * print all the memory blocks that were allocated
 */
/*#define PRINT_MEM_BLOCKS*/

#define USE_MULTIPLE_ALLOCATOR


#ifndef HAVE_UNISTD_H
    #define LACKS_UNISTD_H
#else
    #if HAVE_UNISTD_H == 0
        #define LACKS_UNISTD_H
    #endif
#endif

#if (CONFIG_NETWORK_STACK==cfg_uip)
    #define BUILD_UIP
    #define BUILD_DNS
    #define BUILD_DHCPC
#elif (CONFIG_NETWORK_STACK==cfg_atheros)
    #define BUILD_ATHEROS_STACK
#elif (CONFIG_NETWORK_STACK==cfg_lwip)
    #define BUILD_LWIP
#elif (CONFIG_NETWORK_STACK==cfg_harmony)
    #define BUILD_HARMONY_NET
#elif (CONFIG_NETWORK_STACK==cfg_posix)
    #define BUILD_POSIX_NET
#endif

#if (CONFIG_HAS_TCP == 1)
    #define BUILD_TCP
#endif

#if (CONFIG_HAS_UDP == 1)
    #define BUILD_UDP
#endif

#if (CONFIG_HAS_HTTP == 1)
    #define BUILD_HTTP
#endif

#if (CONFIG_HAS_PICOCOAP == 1)
    #define BUILD_COAP
#endif

#if (CONFIG_HAS_LIBCOAP == 1)
    #define BUILD_COAP
    #define WITH_ERSDK
#endif

#if ((CONFIG_SECURITY == cfg_cyassl) || (CONFIG_SECURITY == cfg_mbedtls))
    #define BUILD_SSL
    /*
     * CYASSL specific defines are located in ./config/config.h
     * If HAVE_CONFIG_H is defined the SYASSL sources includes
     * <config.h>.
     * */
#endif

#undef RTOS_ExOS
#undef RTOS_FreeRTOS
#undef RTOS_Linux
#undef RTOS_OSX
#undef RTOS_MQX

#if (CONFIG_OS==cfg_exos)
    #define RTOS_ExOS
#elif (CONFIG_OS==cfg_freertos)
    #define RTOS_FreeRTOS
#elif (CONFIG_OS==cfg_linux)
    #define RTOS_Linux
#elif (CONFIG_OS==cfg_osx)
    #define RTOS_OSX
#elif (CONFIG_OS==cfg_mqx)
    #define RTOS_MQX
#endif

#if (CONFIG_FREERTOS_LEGACY == 1) && (CONFIG_OS == cfg_freertos)
    #define FREERTOS_LEGACY
#endif

#if (CONFIG_VERSION_INFO == 1)
    #define PRINT_EXOSITE_VERSION
#endif

#if (CONFIG_HAS_CONFIGURATOR_SERVER == 1)
    #define CONFIGURATOR_SERVER_PCLB_PERIOD        10
    #define MAX_CONFIGURATOR_CONNECTION             2
#endif

#endif /* ER_SDK_SRC_INCLUDE_SDK_CONFIG_H_ */
