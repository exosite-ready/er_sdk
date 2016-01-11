/**
 * @file net_dev_factory.c
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
 * @brief
 * This file implements the network interface factory
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <porting/net_port.h>
#include <porting/system_port.h>
#include <system_utils.h>
#include <protocol_client_common.h>
#include <net_dev_factory.h>
#include <rtc_if.h>
#include <ssl_client.h>
#include <net_dev_test.h>

static struct net_dev_operations *tcp_client;
static struct net_dev_operations *udp_client;

void net_dev_factory_init(void)
{
    net_dev_init();

#ifdef BUILD_TCP
    tcp_client = net_dev_tcp_new();
#endif

#ifdef BUILD_UDP
    udp_client = net_dev_udp_new();
#endif

    debug_log(DEBUG_NET, ("Initalizing SSL\n"));
    ssl_client_init();
}

int32_t net_dev_get(struct net_dev_operations **new_net_dev,
                    enum transport_protocol_type type, enum security_mode security)
{
    struct net_dev_operations *net_dev_low = NULL;
    struct net_dev_operations *net_dev = NULL;
    struct net_dev_operations *net_dev_fault_inject = NULL;

    /** If that assert failed that means that no network device
     *  has been registered; and the possible causes are
     *  - you don't have UIP built (CONFIG_HAS_UIP = n)
     *  - you did register any network device,
     *  that should be done in the network stack adaptation layer
     **/
    ASSERT(tcp_client);

    switch (type) {
    case TRANSPORT_PROTO_TCP:
        ASSERT(tcp_client);
        net_dev_low = tcp_client;
        break;
    case TRANSPORT_PROTO_UDP:
        ASSERT(udp_client);
        net_dev_low = udp_client;
        break;
    default:
        ASSERT(0);
    }

    if (security == SECURITY_TLS_DTLS) {
        net_dev = ssl_client_new(net_dev_low, type);
        /** If this assert fails then either
         *  - the SSL support is not built in
         *  - there was an error during the initalization of the SSL library
         **/
        ASSERT(net_dev);
    } else {
        net_dev = net_dev_low;
    }

    net_dev_fault_inject = net_dev_test_new(net_dev);
    if (net_dev_fault_inject != NULL)
        net_dev = net_dev_fault_inject;

    if (net_dev == NULL)
        return ERR_NO_NET_DEV;

    *new_net_dev = net_dev;
    return ERR_SUCCESS;
}
