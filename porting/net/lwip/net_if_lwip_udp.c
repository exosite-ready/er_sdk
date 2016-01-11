/**
 * @file net_if_lwip_udp.c
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
 * @brief Net_dev_operations implementation for the LWIP network stack for UDP
 **/
#include <stdio.h>
#include <porting/net_port.h>

extern struct net_dev_operations udp_ops;
struct net_dev_operations *net_dev_udp_new(void)
{
    printf("Init udp interface\n");
    return &udp_ops;
}
