/**
 * @file net_if_uip_udp.c
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
 * @brief Net_dev_operations implementation for the UIP network stack for UDP
 **/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <lib/type.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/error.h>

#include <porting/net_port.h>

#include <ssl_client.h>

#include <uip.h>
#include <udp-socket.h>
#include <assert.h>
#include "uip_net.h"
#include "uip_port.h"

static struct net_dev_operations eth_ops;
/*
 * Lua: data = packip( ip0, ip1, ip2, ip3 ), or
 * Lua: data = packip( "ip" )
 * Returns an internal representation for the given IP address
 */
static int packip(const char *pip, uip_net_ip *ip)
{
    unsigned int i, len, temp[4];

    if (sscanf(pip, "%u.%u.%u.%u%n", temp, temp + 1, temp + 2, temp + 3, &len) != 4 ||
        len != strlen(pip))
    return -1;
    for (i = 0; i < 4; i++) {
        if (temp[i] > 255)
        return -1;
        ip->ipbytes[i] = (u8)temp[i];
    }

    return 0;
}

static int unpackip(uip_net_ip ip, char *addr, int size)
{
    snprintf(addr, size, "%d.%d.%d.%d", (int)ip.ipbytes[0], (int)ip.ipbytes[1],
            (int)ip.ipbytes[2], (int)ip.ipbytes[3]);
    return 0;
}

static int32_t net_if_socket(struct net_dev_operations *self, struct net_socket **new_socket)
{
    int error;
    struct net_socket *socket;
    struct udp_socket *usocket;

    (void)self;
    error = udp_socket(&usocket);
    if (error != ERR_SUCCESS)
        return error;

    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;

    if (socket) {
        socket->sockfd = (int)usocket;
        socket->ops = &eth_ops;
        socket->non_blocking = 1;
        socket->dtls = 0;
        socket->connected = 0;
    }

    *new_socket = socket;
    return ERR_SUCCESS;
}

static int32_t net_if_connect(struct net_socket *socket, const char *ip, unsigned short port)
{
    uip_net_ip addr;
    uip_ipaddr_t ipaddr;
    struct udp_socket *usocket = (struct udp_socket *)socket->sockfd;
    int error;

    if (socket->connected)
        return 0;

    error = packip(ip, &addr);
    if (error != ERR_SUCCESS) {
        error_log(DEBUG_NET, ("Netif connect Invalid address\n"), error);
        return -1;
    }

    uip_ipaddr(ipaddr, addr.ipbytes[0], addr.ipbytes[1], addr.ipbytes[2], addr.ipbytes[3]);
    error = udp_socket_connect(usocket, &ipaddr, port);

    return error;
}

/* The receive embedded callback
 *  return : nb bytes read, or error
 *
 *
 */
static int32_t net_if_receive(struct net_socket *socket, char *buf, size_t sz, size_t *nbytes_received)
{
    int32_t error;
    int recvd;
    struct udp_socket *usocket = (struct udp_socket *)socket->sockfd;

    error = udp_socket_recv(usocket, (uint8_t *)buf, sz, &recvd);
    *nbytes_received = recvd;

    return error;
}

static int32_t net_if_close(struct net_socket *socket)
{
    struct udp_socket *usocket = (struct udp_socket *)socket->sockfd;

    udp_socket_close(usocket);
    sf_free(socket);
    return 0;
}
/* The send embedded callback
 *  return : nb bytes sent, or error
 */
static int32_t net_if_send(struct net_socket *socket, const char *buf, size_t sz, size_t *nbytes_sent)
{
    int sent;
    struct udp_socket *usocket = (struct udp_socket *)socket->sockfd;

    sent = udp_socket_send(usocket, buf, sz);
    *nbytes_sent = sent;

    if (sent < 0)
        return sent;

    return ERR_SUCCESS;
}

static int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    uip_net_ip ip;
    int error;

    (void)self;
    error = uip_net_lookup(hostname, &ip);
    if (error == ERR_SUCCESS) {
        assert(ip.ipaddr == 0);
        unpackip(ip, ipaddr_str, len);
    }
    return error;
}

static int32_t net_if_config(struct mac_addr *mac)
{
    uip_get_eth_addr((struct uip_eth_addr *)mac);
    return 0;
}

static struct net_dev_operations eth_ops = {
    net_if_socket,
    NULL,
    net_if_connect,
    NULL,
    net_if_close,
    net_if_receive,
    net_if_send,
    net_if_lookup,
    net_if_config,
    NULL
};

struct net_dev_operations *net_dev_udp_new()
{
    return &eth_ops;
}
