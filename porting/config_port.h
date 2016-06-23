/**
 * @file config_port.h
 *
 * @page sdk_config Configuration
 *
 * config_port.h header contains settings that are able to
 * tune the Exosite Ready SDK to be suitable to the application
 * and the environment.
 *
 * @image html ER_SDK_config.jpg
 *
 * There are a couple of groups of these settings:
 *
 * - @ref config_log             - Adjust log verbosity / turn on/off log level in compile time
 * - @ref config_transmission    - Adjust transmission queue size and response timeout
 * - @ref config_sdk_modules     - Include/Exclude SDK modules in/from compilation
 * - @ref configurator_config    - Configurator server settings
 * - @ref heap_config            - Heap configuration
 * - @ref config_wifi            - WiFi Network settings (SSID, Password)
 * - @ref config_provisioning    - Provisioning (device activation) settings
 * - @ref config_other           - Common settings
 * - @ref config_environment     - Environment dependent settings
 *
 * @warning
 * In most of the cases you do not need to modify these
 * settings.
 */

#ifndef ER_SDK_PORTING_CONFIG_PORT_H_
#define ER_SDK_PORTING_CONFIG_PORT_H_

#include "./system/os_list.h"
#include "./net/ns_list.h"
#include "config/security_list.h"

/**
 * @defgroup config_port Configuration
 *
 * It is a configuration header, that provides an easy way to
 * set compile time details of the SDK
 *
 * @{
 */

/*----------------------------------*/
/** @name Logging settings
 *  @anchor config_log
 *  Configures settings for logging */
/*----------------------------------*/
/**@{*/
/**
 * @brief Configures the verbosity of the debug logs
 *
 *  0 - Minimal
 *  1 - Verbose
 **/
#define DEBUG_VERBOSITY   1

/**
 * @brief Configures which debug level logs are compiled in
 *
 * By default info, warning, error are built in
 * - DEBUG_ERROR     - Only Error messages
 * - DEBUG_WARNING   - Error and Warning messages
 * - DEBUG_INFO      - Error, Warning and Info messages
 * - DEBUG_DEBUG     - All of the messages
 **/
#ifndef COMPILE_TIME_DEBUG_LEVEL
    #define COMPILE_TIME_DEBUG_LEVEL DEBUG_INFO
#endif

/**@}*/

/*----------------------------------*/
/** @name Transmission
 *  @anchor config_transmission
 *  Network transmission settings   */
/*----------------------------------*/
/**@{*/

/**
 * @brief This is the timeframe during which the request has to be sent and a response
 * has to be received successfully
 *
 * If the request could not be sent or no response was received for this long
 * there will be a timeout callback for the user and the SDK will stop trying
 **/
#define RESPONSE_TIMEOUT      (35000)

/**
 * @brief The queue size for the User requests
 *
 * Transmission USER API calls are asynchronous and put a request to a queue.
 * This setting sets the queue size
 * Adjust the queue size to the memory size of your system
 **/
#define MAX_REQUEST_QUEUE_SIZE    (10)

/**@}*/

/*-------------------------*/
/** @name Modules
 *  @anchor config_sdk_modules
 *  Compiled SDK modules   */
/*-------------------------*/
/**@{*/

/**
 * @brief Select the network stack solution used by the SDK
 *
 * By default it is set to posix, which means the SDK will communicate over POSIX sockets
 *
 * The ERSDK has built in porting layer for various network stacks (see the list below)
 * If you port the SDK to a different network stack,
 * then use cfg_custom or add your own config variable.
 * For details check out the @ref sdk_porting_guide
 *
 * Supported values:
 *  - cfg_uip      - uIP network stack
 *  - cfg_atheros  - Atheros network stack
 *  - cfg_lwip     - lwIP network stack
 *  - cfg_posix    - POSIX compatible network stack
 *  - cfg_harmony  - Microchip Harmony network stack
 *  - cfg_custom   - Check out @ref sdk_porting_guide
 */
#ifndef CONFIG_NETWORK_STACK
    #define CONFIG_NETWORK_STACK      cfg_posix
#endif

/**
 * @brief Select the operating system
 * Supported OSs:
 *  - cfg_exos      - ExOS mode; No operating system
 *  - cfg_freertos  - FreeRTOS (8.2.1)
 *  - cfg_linux     - Linux
 *  - cfg_osx       - OS X
 *  - cfg_mqx       - Freescale MQX
 *
 *  If you port the SDK to a different OS please check out @ref sdk_porting_guide
 */
#ifndef CONFIG_OS
    #define CONFIG_OS                cfg_linux
#endif

/**
 * @brief Makes SDK compatible with FreeRTOS 7.5.2
 **/
#ifndef CONFIG_FREERTOS_LEGACY
    #define CONFIG_FREERTOS_LEGACY   0
#endif

/**
 * @brief This sets whether the SDK compiles in TCP support
 *
 * The network stack used is configured by CONFIG_NETWORK_STACK
 **/
#ifndef CONFIG_HAS_TCP
    #define CONFIG_HAS_TCP            1
#endif

/**
 * @brief This sets whether the SDK compiles in UDP support
 *
 * The network stack used is configured by CONFIG_NETWORK_STACK
 **/
#ifndef CONFIG_HAS_UDP
    #define CONFIG_HAS_UDP            1
#endif

/**
 * @brief This sets whether the SDK compiles in HTTP client support
 **/
#ifndef CONFIG_HAS_HTTP
    #define CONFIG_HAS_HTTP           1
#endif

/**
 * @brief This sets whether the SDK compiles in Picocoap client support
 *
 * Picocoap is Exosite's ligthweight coap solution
 **/
#ifndef CONFIG_HAS_PICOCOAP
    #define CONFIG_HAS_PICOCOAP       0
#endif

/**
 * @brief This sets whether the SDK compiles in libcoap client support
 *
 * Libcoap is a third party module
 **/
#ifndef CONFIG_HAS_LIBCOAP
    #define CONFIG_HAS_LIBCOAP        0
#endif

/**
 * @brief This sets the packet size for coap
 *
 * The theoretical limit is the UDP packet size but for resource constrained
 * devices this has to be set to much lower
 **/
#ifndef CONFIG_LIBCOAP_PACKET_SIZE
    #define CONFIG_LIBCOAP_PACKET_SIZE        256
#endif

/**
 * @brief This sets which security library is compiled in
 *
 * By default it is cyassl
 **/
#ifndef CONFIG_SECURITY
    #define CONFIG_SECURITY                   cfg_cyassl
#endif

/**
 * @brief This sets the path of an external config file that
 * will be included by the include/config/config.h file if
 * the CONFIG_SECURITY is set to cfg_external. It can be very
 * helpful if the external security module uses its own
 * config.h file.
 *
 * By default it is 0 that means "There is no external config file"
 **/
//  //  #define CONFIG_EXTERNAL_SECURITY_CONFIG   <PATH/OF/THE/EXTERNAL/CONFIG>


/**
 * @brief This sets the configurator server
 *
 * Configurator server is a web server that provide a configuration
 * page for the WiFi settings.
 **/
#ifndef CONFIG_HAS_CONFIGURATOR_SERVER
    #define CONFIG_HAS_CONFIGURATOR_SERVER    1
#endif


/**@}*/


/*-----------------------------------*/
/** @name Configurator Server config
 *  @anchor configurator_config
 *  Configurator server settings */
/*-----------------------------------*/
/**@{*/

/**
 * @brief Input buffer size of the configurator server
 *
 * Size of the input buffer of the configurator server. Higher
 * values may result better response time, but increase the
 * memory consumption.
 **/
#ifndef CONFIGURATOR_BUFFER_SIZE
    #define CONFIGURATOR_BUFFER_SIZE          1024
#endif


/**@}*/


/*-----------------------------------*/
/** @name Heap config
 *  @anchor heap_config
 *  Configure heaps */
/*-----------------------------------*/
/**@{*/

/**
 * @brief Sets whether a separate heap for SSL is needed
 *
 * For systems with scarce RAM this HEAP fragmentation is a huge problem
 * This setting allows to allocate a dedicated heap for SSL
 * This is a compromise: this will use more memory than using just one heap, but will be more robust
 **/
#ifndef CONFIG_HAS_SSL_HEAP
    #define CONFIG_HAS_SSL_HEAP            1
#endif

/**
 * @brief This sets the size of the SSL heap, if CONFIG_HAS_SSL_HEAP is turned on
 **/
#ifndef CONFIG_SSL_HEAP_SIZE
    #define CONFIG_SSL_HEAP_SIZE           27000
#endif

/**
 * @brief Sets whether a separate heap for Strings is needed
 *
 * For systems with scarce RAM this HEAP fragmentation is a huge problem
 * This setting allows to allocate a dedicated heap for Strings
 * This is a compromise: this will use more memory than using just one heap, but will be more robust
 **/
#ifndef CONFIG_HAS_STRING_HEAP
    #define CONFIG_HAS_STRING_HEAP          1
#endif

/**
 * @brief This sets the size of the String heap, if CONFIG_HAS_STRING_HEAP is turned on
 **/
#ifndef CONFIG_STRING_HEAP_SIZE
    #define CONFIG_STRING_HEAP_SIZE         2000
#endif

/**@}*/

/*-----------------------------------*/
/** @name WiFi
 *  @anchor config_wifi
 *  Compiled WiFi network parameters */
/*-----------------------------------*/
/**@{*/

/** @brief WIFI SSID (for WiFi devices only) */
#ifndef WIFI_SSID
    #define WIFI_SSID       "MyNetwork"
#endif

/** @brief WIFI Passphrase (for WiFi devices only) */
#ifndef WIFI_PASSW
    #define WIFI_PASSW      "MyPassphrase"
#endif
/**@}*/

/*----------------------------*/
/** @name Provisioning
 *  @anchor config_provisioning
 *  Provisioning parameters   */
/*----------------------------*/
/**@{*/

/**
 *
 * @brief Sets the delay type after a failed activation
 * 1 - Linear delay
 * 2 - Exponential backoff
 *
 * By default we delay the new activation request with the same amount of time
 * which is can be configured With the ACTIVATION_DELAY
 *
 * If Exponential backoff is selected then the SDK will only try to activate
 * after every 2^n request, where n is a non negative number
**/
#ifndef ACTIVATION_RETRY_TYPE
    #define ACTIVATION_RETRY_TYPE   (1)
#endif

/**
 *  @brief Sets how often should the device try to activate after a failed activation in linear delay mode
 *
 *  By default it is 10 sec
 **/
#ifndef ACTIVATION_DELAY
    #define ACTIVATION_DELAY (10000)
#endif

/**@}*/

/*-------------------------*/
/** @name Other
 *  @anchor config_other
 *  General configuration  */
/*-------------------------*/
/**@{*/

/**
 * @brief Prints version info based on information provided in version.h
 *
 * If you turn on this setting you have to create a version.h in the er_sdk folder.
 * version.h must contain two char arrays:
 * const char *version
 * const char* hash
 *
 **/
#ifndef CONFIG_VERSION_INFO
    #define CONFIG_VERSION_INFO         0
#endif


/**
 *  Defines the UNLIMITED_STATE_MACHINES symbol
 **/
#define UNLIMITED_STATE_MACHINES 0

/**
 * @brief Sets the maximum number of Protocol Client State Machines
 *
 * In case of HTTP subscriptions are handled by their own statemachines due to the blocking
 * nature of long-poll.
 * Since this is very resource consuming this can be limited to a fixed number. In that case
 * subscribe request will return errors after reaching the limit
 **/
#ifndef CONFIG_NUMBER_OF_STATE_MACHINES
    #define CONFIG_NUMBER_OF_STATE_MACHINES         UNLIMITED_STATE_MACHINES
#endif

/**@}*/

/*-----------------------------*/
/** @name Environment
 *  @anchor config_environment
 *  Environment settings       */
/*-----------------------------*/
/**@{*/

/** Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/** Define to 1 if you have the <arpa/inet.h> header file. */
//#define HAVE_ARPA_INET_H 1

/** Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/** Define to 1 if you have the `getaddrinfo' function. */
//#define HAVE_GETADDRINFO 1

/** Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/** Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/** Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/** Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/** Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/** Define to 1 if you have the <netdb.h> header file. */
//#define HAVE_NETDB_H 1

/** Define to 1 if you have the <netinet/in.h> header file. */
//#define HAVE_NETINET_IN_H 1

/** Define to 1 if you have the `select' function. */
//#define HAVE_SELECT 1

/** Define to 1 if you have the `socket' function. */
//#define HAVE_SOCKET 1

/** Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/** Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/** Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/** Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/** Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/** Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/** Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/** Define to 1 if you have the <syslog.h> header file. */
//#define HAVE_SYSLOG_H 1

/** Define to 1 if you have the <sys/socket.h> header file. */
//#define HAVE_SYS_SOCKET_H 1

/** Define to 1 if you have the <sys/stat.h> header file. */
//#define HAVE_SYS_STAT_H 1

/** Define to 1 if you have the <sys/time.h> header file. */
//#define HAVE_SYS_TIME_H 1

/** Define to 1 if you have the <sys/types.h> header file. */
//#define HAVE_SYS_TYPES_H 1

/** Define to 1 if you have the <sys/unistd.h> header file. */
//#define HAVE_SYS_UNISTD_H 1

/** Define to 1 if you have the <time.h> header file. */
//#define HAVE_TIME_H 1

/** Define to 1 if you have the <unistd.h> header file. */
//#define HAVE_UNISTD_H 0

/** Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1
#define CONFIG_XXX  (1)
/** Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/**@}*/

/** @} end of system_port_interface group */
#endif /* ER_SDK_PORTING_CONFIG_PORT_H_ */
