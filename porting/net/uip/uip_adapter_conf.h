/**
 * @file uip_adapter_conf.h
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
 * @brief Common defines for UIP config
 **/
#ifndef INC_UIP_ADAPTER_CONF_H_
#define INC_UIP_ADAPTER_CONF_H_

/* Configuration for element 'tcpip' */
#define UIP_CONF_NETMASK0               255
#define UIP_CONF_NETMASK1               255
#define UIP_CONF_NETMASK2               255
#define UIP_CONF_NETMASK3               0
#define UIP_CONF_DEFGW0                 192
#define UIP_CONF_DEFGW1                 168
#define UIP_CONF_DEFGW2                 2
#define UIP_CONF_DEFGW3                 1
#define UIP_CONF_DNS0                   192
#define UIP_CONF_DNS1                   168
#define UIP_CONF_DNS2                   2
#define UIP_CONF_DNS3                   1
#define UIP_CONF_IPADDR0                192
#define UIP_CONF_IPADDR1                168
#define UIP_CONF_IPADDR2                2
#define UIP_CONF_IPADDR3                130

#endif /* INC_UIP_ADAPTER_CONF_H_ */
