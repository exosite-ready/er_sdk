/**
 * @file platform_lwip.h
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
 * @brief Platform interface for LWIP
 **/
#ifndef ER_SDK_PORTING_NETWORKING_LWIP_PLATFORM_LWIP_H_
#define ER_SDK_PORTING_NETWORKING_LWIP_PLATFORM_LWIP_H_

struct link_layer_if {
    int32_t unused;
    /* TODO */
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

#endif /* ER_SDK_PORTING_NETWORKING_LWIP_PLATFORM_LWIP_H_ */
