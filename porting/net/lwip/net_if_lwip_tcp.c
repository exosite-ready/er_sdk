/**
 * @file net_if_lwip_tcp.c
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
 * @brief Net_dev_operations implementation for the LWIP network stack for TCP
 **/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lib/type.h>
#include <lib/error.h>
#include <porting/system_port.h>
#include <lib/debug.h>

#include <lib/sf_malloc.h>
#include <lib/mem_check.h>

#include <lwip/sockets.h>
#include <lwip/api.h>

static struct net_dev_operations eth_ops;

static int unpackip(uint32_t ip, char *addr, size_t size)
{
    unsigned char ipbytes[4];

    memcpy(ipbytes, &ip, 4);
    snprintf(addr, size, "%u.%u.%u.%u", (int)ipbytes[0],
                                        (int)ipbytes[1],
                                        (int)ipbytes[2],
                                        (int)ipbytes[3]);
    return 0;
}

static struct net_socket *net_if_socket_internal(int udp)
{
    struct net_socket *socket;
    int fred = 1;
    int type;

    socket = sf_malloc(sizeof(*socket));
    if (!socket)
        return NULL;

    if (udp == 1)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    socket->sockfd = lwip_socket(AF_INET, type, 0);
    if (socket->sockfd < 0)
        return NULL;

    lwip_ioctl(socket->sockfd, (long)FIONBIO, &fred);
    socket->ops = &eth_ops;

    socket->non_blocking = 1;
    socket->dtls = 0;
    socket->connected = 0;
    return socket;
}

static int32_t net_if_socket_tcp(struct net_dev_operations *self, struct net_socket **socket)
{
    struct net_socket *s = net_if_socket_internal(0);

    (void)self;
    if (s == NULL)
        return ERR_FAILURE;

    *socket = s;

    return ERR_SUCCESS;
}

static int32_t net_if_socket_udp(struct net_dev_operations *self, struct net_socket **socket)
{
    struct net_socket *s = net_if_socket_internal(1);

    (void)self;
    if (s == NULL)
        return ERR_FAILURE;

    *socket = s;

    return ERR_SUCCESS;
}

static int32_t net_if_connect(struct net_socket *socket, const char *ip,
        unsigned short port)
{
    int32_t error;
    int32_t sockfd = socket->sockfd;
    struct sockaddr_in addr;

    if (socket->connected)
        return ERR_SUCCESS;
    /* set up address to connect to */
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = (u16_t)PP_HTONS(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    error = lwip_connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if (error != ERR_OK) {
        uint32_t len = sizeof(int);
        int status;

        status = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (status != ERR_OK)
            return ERR_FAILURE;

        if (error == EISCONN) {
            socket->connected = 1;
            return ERR_SUCCESS;
        }
        if (error == EINPROGRESS)
            return ERR_WOULD_BLOCK;
    }
    socket->connected = 1;
    return error;
}

/* The receive embedded callback
 *  return : nb bytes read, or error
 *
 *
 */
static int32_t net_if_receive(struct net_socket *socket, char *buf, size_t sz, size_t *nbytes_received)
{
    int32_t sockfd = socket->sockfd;
    int error;

    error = lwip_recv(sockfd, buf, sz, 0);
    if (error < ERR_OK) {
        uint32_t len = sizeof(int);
        int status;

        *nbytes_received = 0;
        status = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
        if (status != ERR_OK)
            return ERR_FAILURE;
        if (error == EINPROGRESS || error == EAGAIN)
            return ERR_WOULD_BLOCK;
        else
            return -error;
    }

    *nbytes_received = (size_t)error;
    return ERR_SUCCESS;
}

static int32_t net_if_close(struct net_socket *socket)
{
    int32_t sockfd = socket->sockfd;

    sf_free(socket);
    return lwip_close(sockfd);
}
/* The send embedded callback
 *  return : nb bytes sent, or error
 */
static int32_t net_if_send(struct net_socket *socket, const char *buf, size_t sz, size_t *nbytes_sent)
{
    int32_t sockfd = socket->sockfd;
    int32_t error;

    error = lwip_send(sockfd, buf, sz, 0);

    if (error < ERR_OK) {
        uint32_t len = sizeof(int);
        int status;

        *nbytes_sent = 0;
        status = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
        if (status != ERR_OK)
            return ERR_FAILURE;
        if (error == EINPROGRESS || error == EAGAIN)
            return ERR_WOULD_BLOCK;
        else
            return -error;
    }

    *nbytes_sent = (size_t)error;
    return ERR_SUCCESS;
}

static int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    int32_t error;
    ip_addr_t server;

    (void)self;
    error = netconn_gethostbyname(hostname, &server);
    unpackip(server.addr, ipaddr_str, len);
    return error;
}

static int32_t net_if_config(struct mac_addr *mac)
{
    (void)mac;
    return 0;
}

static struct net_dev_operations tcp_ops = {
    net_if_socket_tcp,
    NULL,
    net_if_connect,
    NULL,
    net_if_close,
    net_if_receive,
    net_if_send,
    net_if_lookup,
    net_if_config,
    NULL,
    NULL
};

struct net_dev_operations udp_ops = {
    net_if_socket_udp,
    NULL,
    net_if_connect,
    NULL,
    net_if_close,
    net_if_receive,
    net_if_send,
    net_if_lookup,
    net_if_config,
    NULL,
    NULL
};

struct net_dev_operations *net_dev_tcp_new(void)
{
    printf("Init tcp interface\n");
    return &tcp_ops;
}
