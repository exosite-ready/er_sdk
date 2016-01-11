#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lib/type.h>
#include <errno.h>
#include <porting/system_port.h>
#include <porting/net_port.h>

#include <ssl_client.h>

#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/error.h>
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
    int sockfd;
    int32_t error;
    struct net_socket *socket;

    (void)self;
    sockfd = uip_net_socket(UIP_NET_SOCK_STREAM, 1);
    if (sockfd < 0) {
        error_log(DEBUG_NET, ("Netif socket failed\n"), sockfd);
        return ERR_FAILURE;
    }

    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;

    if (socket) {
        socket->sockfd = sockfd;
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
    int ret;
    int error;

    if (socket->connected)
    return 0;

    ret = packip(ip, &addr);
    if (ret < 0) {
        error_log(DEBUG_NET, ("Netif connect Invalid address\n"), ret);
        return -1;
    }

    trace_log(DEBUG_NET, ("Netif trying to connect\n"));
    ret = uip_net_connect(socket->sockfd, addr, port);
    error = uip_net_get_last_err(socket->sockfd);
    trace_log(DEBUG_NET, ("Netif connect returned %d %d\n", ret, error));
    if (ret < 0) {
        if (error == ERR_WOULD_BLOCK)
        return ERR_WOULD_BLOCK;
    } else {
        debug_log(DEBUG_NET, ("Netif connected\n"));
        socket->connected = 1;
    }

    return ret;
}

/* The receive embedded callback
 *  return : nb bytes read, or error
 *
 *
 */
static int32_t net_if_receive(struct net_socket *socket, char *buf, size_t sz, size_t *nbytes_received)
{
    int sd = socket->sockfd;
    unsigned short lastchar = UIP_NET_NO_LASTCHAR;
    unsigned timer_id = 0; /*Currently not used */
    timer_data_type timeout = 0; /*Currently not used */
    int error = 0xcc;
    int recv;

    trace_log(DEBUG_NET, ("Netif recv %p\n", buf));
    recv = uip_net_recv(sd, buf, sz, lastchar, timer_id, timeout);
    error = uip_net_get_last_err(sd);
    trace_log(DEBUG_NET, ("Netif recv returned %d\n", recv, error));

    *nbytes_received = recv;

    if (recv < 0) {
        if (socket->non_blocking && error == ERR_WOULD_BLOCK) {
            return ERR_WOULD_BLOCK;
        } else {
            error_log(DEBUG_NET, ("Net_if receive error %d\n", recv), error);
            return ERR_FAILURE;
        }
    }

    check_memory();
    return ERR_SUCCESS;
}

static int32_t net_if_close(struct net_socket *socket)
{
    int ret;

    ret = uip_net_close(socket->sockfd);
    sf_free(socket);
    return ret;
}
/* The send embedded callback
 *  return : nb bytes sent, or error
 */
static int32_t net_if_send(struct net_socket *socket, const char *buf, size_t sz, size_t *nbytes_sent)
{
    int ss;
    int error;
    int sd = socket->sockfd;

    trace_log(DEBUG_NET, ("Netif send %p\n", buf));
    ss = uip_net_send(sd, buf, sz);
    error = uip_net_get_last_err(sd);
    trace_log(DEBUG_NET, ("Netif send returned %d %d\n", ss, error));
    if (socket->non_blocking && error == ERR_WOULD_BLOCK) {
        *nbytes_sent = ss;
        return ERR_WOULD_BLOCK;
    }
    trace_log(DEBUG_NET, ("Netif send end\n"));
    *nbytes_sent = ss;
    return ERR_SUCCESS;
}

static int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    uip_net_ip ip;
    int error;

    (void)self;
    error = uip_net_lookup(hostname, &ip);
    if (error == ERR_SUCCESS) {
        assert(ip.ipaddr);
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

struct net_dev_operations *net_dev_tcp_new()
{
    return &eth_ops;
}
