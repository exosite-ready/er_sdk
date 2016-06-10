/**
 * @file net_if_harmony_tcp.c
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
 * @brief Net_dev_operations implementation for the Posix network stack for TCP
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lib/error.h>
#include <lib/debug.h>
#include <porting/net_port.h>
#include <lib/sf_malloc.h>
#include "tcpip/tcpip.h"

static struct net_dev_operations eth_ops;
extern int h_errno;

static int32_t _APP_ParseUrl(char *uri, char **host, char **path, uint16_t * port)
{
    char * pos;
    pos = strstr(uri, "//"); //Check to see if its a proper URL
    *port = 80;


    if ( pos )
        *host = pos + 2; // This is where the host should start
    else
        *host = uri;

    pos = strchr( * host, ':');

    if ( !pos ) {
        pos = strchr(*host, '/');
        if (!pos) {
            *path = NULL;
        } else {
            *pos = '\0';
            *path = pos + 1;
        }
    }
    else {
        *pos = '\0';
        char * portc = pos + 1;

        pos = strchr(portc, '/');
        if (!pos) {
            *path = NULL;
        }
        else {
            *pos = '\0';
            *path = pos + 1;
        }
        *port = atoi(portc);
    }
    return ERR_SUCCESS;
}

static int32_t net_if_socket_internal(struct net_socket **new_socket,
                                      enum transport_protocol_type type,
                                      const int *posix_socket)
{
    int sockfd = 0;
    struct net_socket *nsocket = NULL;

    switch (type) {
    case TRANSPORT_PROTO_UDP:
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        break;
    case TRANSPORT_PROTO_TCP:
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        break;
    default:
        ASSERT(0);
        break;
    }

    if (sockfd == SOCKET_ERROR) {
        error_log(DEBUG_NET, ("Failed to create socket %d\n", sockfd), ERR_FAILURE);
        return ERR_FAILURE;
    }

    nsocket = malloc(sizeof(*nsocket));
    if (nsocket) {
        nsocket->sockfd = sockfd;
        nsocket->ops = &eth_ops;
        nsocket->rflags = 0;
        nsocket->wflags = 0;
        nsocket->dtls = 0;
        nsocket->non_blocking = 1;
        nsocket->connected = 0;
        nsocket->already_started = 0;
    } else {
        error_log(DEBUG_NET, ("Failed to allocate memory for socket\n"), ERR_NO_MEMORY);
        return ERR_NO_MEMORY;
    }

    *new_socket = nsocket;

    return ERR_SUCCESS;
}

static int32_t net_if_socket_tcp(struct net_dev_operations *self, struct net_socket **socket)
{
    (void)self;
    debug_log(DEBUG_NET, ("Create TCP socket\n"));
    return net_if_socket_internal(socket, TRANSPORT_PROTO_TCP, NULL);
}

static int32_t net_if_socket_udp(struct net_dev_operations *self, struct net_socket **socket)
{
    /*
    (void)self;
    debug_log(DEBUG_NET, ("Create UDP socket\n"));
    return net_if_socket_internal(socket, TRANSPORT_PROTO_UDP, NULL);
    */
    return 0;
}

static int32_t net_if_connect(struct net_socket *socket, 
                              const char *ip,
                              unsigned short port)
{
    debug_log(DEBUG_NET, ("net_if_connect(%d, %s, %d)\n", socket->sockfd, ip, port));
    /*
    struct sockaddr_in addr;
    int error;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (socket->connected)
        return ERR_SUCCESS;

    error = connect(socket->sockfd, (const struct sockaddr *) &addr,
            sizeof(addr));

    if (error)
        switch (errno) {
        case EINPROGRESS:
        case EALREADY:
            return ERR_WOULD_BLOCK;
        case EISCONN:
            socket->connected = TRUE;
            return ERR_SUCCESS;
        default:
            error_log(DEBUG_NET, ("Connect failed\n"), error);
            return ERR_FAILURE;
        }

    socket->connected = TRUE;
    return error;
    */
    return 0;
}

static int net_if_receive(struct net_socket *net_if, char *buf, size_t size, size_t *nbytes_recvd)
{
    /*
    int recvd;
    int sd = net_if->sockfd;

    recvd = (int) recv(sd, buf, size, net_if->rflags);

    if (recvd < 0) {
        int err;

        err = errno;
        if (err == EWOULDBLOCK || err == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return recvd;
    }

    if (recvd == 0) {
        error_log(DEBUG_NET, ("Remote host closed the connection\n"), ERR_FAILURE);
        return ERR_FAILURE;
    }

    *nbytes_recvd = (size_t)recvd;
    */
    return ERR_SUCCESS;
}

static int net_if_send(struct net_socket *net_if, const char *buf, size_t size, size_t *nbytes_sent)
{
    /*
    int sd = net_if->sockfd;
    int sent;
    size_t len = size;

    sent = (int) send(sd, &buf[size - len], len, net_if->wflags);

    if (sent < 0) {
        int err;

        err = errno;
        if (err == EWOULDBLOCK || err == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return sent;
    }

    *nbytes_sent = (size_t)sent;
    */
    return ERR_SUCCESS;
}

static int unpackip(struct hostent * hostInfo, char *addr, int size)
{
    char* ip = *(hostInfo->h_addr_list);
    snprintf(addr, size, "%d.%d.%d.%d",
            (uint8_t)ip[0], (uint8_t)ip[1],
            (uint8_t)ip[2], (uint8_t)ip[3]);
    return 0;
}

static int net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    static char *      hostname_cpy = NULL;
    static char *      host = NULL;
    static char *      path = NULL;
    static uint16_t    port = 80;

    struct hostent *   hostInfo;
    static uint8_t     lookup_in_progress = 0;
    
    int32_t status = ERR_WOULD_BLOCK;

    (void)self;

    if (!lookup_in_progress) {

        hostname_cpy = sf_malloc(strlen(hostname) +1);
        strcpy(hostname_cpy, hostname);

        debug_log(DEBUG_NET, ("DNS lookup: %s\n", hostname));
        lookup_in_progress = 1;
        if (_APP_ParseUrl(hostname, &host, &path, &port)) {
            error_log(DEBUG_NET, ("Could not parse URL '%s'\r\n", hostname), ERR_INVALID_FORMAT);
            status = ERR_INVALID_FORMAT;
        }
        debug_log(DEBUG_NET, ("host: %s, path: %s, port: %d\n",
                host ? host : "Unknown",
                path ? path : "Unknown",
                port));

        check_memory();
    }

    while (status == ERR_WOULD_BLOCK) {

        hostInfo = gethostbyname(host);

        if (hostInfo != NULL) {
            
//          memcpy(&addr.sin_addr.S_un.S_addr, *(hostInfo->h_addr_list), sizeof(IPV4_ADDR));
/*
            printf("%d:%d:%d:%d", addr.sin_addr.S_un.S_un_b.s_b1,
                                  addr.sin_addr.S_un.S_un_b.s_b2,
                                  addr.sin_addr.S_un.S_un_b.s_b3,
                                  addr.sin_addr.S_un.S_un_b.s_b4);
*/           
            unpackip(hostInfo, ipaddr_str, len);
            debug_log(DEBUG_NET, ("DNS resolution: %s\n", ipaddr_str));
            status = ERR_SUCCESS;

        } else if (h_errno == TRY_AGAIN) {
            status = ERR_WOULD_BLOCK;;
        } else {
            error_log(DEBUG_NET, ("DNS error while resolving %s\n", hostname), h_errno);
            status = ERR_NET_DNS_ERROR;
        }
    }

    if (status != ERR_WOULD_BLOCK) {
        sf_free(hostname_cpy);
        hostname_cpy = NULL;
        host = NULL;
        path = NULL;
        port = 80;
        lookup_in_progress = 0;
        check_memory();
    }

    return status;
}

static int net_if_close(struct net_socket *net_if)
{
    debug_log(DEBUG_NET, ("net_if_lookup(%d)\n", net_if->sockfd));
            
    /*
    close(net_if->sockfd);
    free(net_if);
    */
    return ERR_SUCCESS;
}

static struct net_dev_operations tcp_ops = {
        net_if_socket_tcp,          /* socket */
        NULL,                       /* server_socket */
        net_if_connect,             /* connect */
        NULL,                       /* accept */
        net_if_close,               /* close */
        net_if_receive,             /* recv */
        net_if_send,                /* send */
        net_if_lookup,              /* lookup */
        NULL,                       /* config */
        NULL,                       /* set_identity */
        NULL                        /* deinit */
};

struct net_dev_operations udp_ops = {
        net_if_socket_udp,          /* socket */
        NULL,                       /* server_socket */
        net_if_connect,             /* connect */
        NULL,                       /* accept */
        net_if_close,               /* close */
        net_if_receive,             /* recv */
        net_if_send,                /* send */
        net_if_lookup,              /* lookup */
        NULL,                       /* config */
        NULL,                       /* set_identity */
        NULL                        /* deinit */
};

struct net_dev_operations *net_dev_tcp_new()
{
    return &tcp_ops;
}

