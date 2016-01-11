/**
 * @file platform_uip.h
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
 * @brief Platform interface for uip
 **/
#ifndef ER_SDK_PORTING_NETWORKING_UIP_PLATFORM_UIP_H_
#define ER_SDK_PORTING_NETWORKING_UIP_PLATFORM_UIP_H_

#include <porting/net_port.h>

struct link_layer_if {
    void (*send_packet)(const void* buf, size_t size);
    size_t (*get_packet_nb)(void* buf, size_t max_len);
    int32_t (*get_mac)(struct mac_addr *mac);
};

/*
 * This function has to be implemented if a link layer device
 * is to be used
 * The SDK will call this function
 *
 * @param[out]  ll_if The link layer object to be used by the SDK
 *
 */
void platform_ll_device_init(struct link_layer_if **ll_if);

#endif /* ER_SDK_PORTING_NETWORKING_UIP_PLATFORM_UIP_H_ */
