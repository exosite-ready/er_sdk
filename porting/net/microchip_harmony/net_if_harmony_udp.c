/**
 * @file net_if_harmony_udp.c
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
 * @brief Net_dev_operations implementation for the posix network stack for UDP
 **/
#include <stdio.h>
#include <porting/net_port.h>
#include <lib/debug.h>

extern struct net_dev_operations udp_ops;
struct net_dev_operations *net_dev_udp_new()
{
    debug_log(DEBUG_NET, ("Init udp interface\n"));
    return &udp_ops;
}

