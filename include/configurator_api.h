/**
 * @file exosite_api.h
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
 * @brief This file contains the User API of the Configurator
 *        Server
 *
 * @defgroup Configurator_API Configurator API
 * This interface provides access to the Configurator Server
 * services.
 *
 * The Configurator Server is a very light weight
 * web server with limited functionality. It provides a html
 * based configuration interface for the wireless network
 * settings.
 *
 * @{
 */

#ifndef ER_SDK_INCLUDE_CONFIGURATOR_API_H_
#define ER_SDK_INCLUDE_CONFIGURATOR_API_H_

#include <stdint.h>


#define MAX_SSID_LENGTH              32
#define MAX_PASSPHRASE_LENGTH        64

struct wifi_settings {
    enum {OPEN=0, WPA2} security;
    char ssid[MAX_SSID_LENGTH + 1];
    char passpharase[MAX_PASSPHRASE_LENGTH + 1];
};

/**
 * @brief Start Configurator Server
 *
 * @param[in] ip        - ip address
 * @param[in] port      - port number
 * @param[in] sn        - Serial number of the device that you want to show
 *                        on the configuration page. (It is optional. If
 *                        this parameter is set to NULL, the return value
 *                        of the system_get_sn function will be shown by default)
 *
 *                        @warning If you set the serial number of the device through
 *                                 optional_sn parameter of the exosite_new function
 *                                 you may want to set the same value here as well!
 *
 * @param[in] on_set    - Callback function that is called if the wifi
 *                        configuration changes
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t start_configurator_server(const char* ip,
                                  uint16_t port,
                                  const char *sn,
                                  int32_t (*on_set)(struct wifi_settings *));

/**
 * @brief Stop Configurator Server
 *
 * @param none
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t stop_configurator_server(void);

/** @} end of Configurator_API group */

#endif /* ER_SDK_INCLUDE_CONFIGURATOR_API_H_ */
