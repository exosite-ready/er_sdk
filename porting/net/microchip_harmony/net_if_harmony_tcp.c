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
#include <errno.h>
#include <sys/types.h>
#include <sys/errno.h>

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

    nsocket = sf_malloc(sizeof(*nsocket));
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

    check_memory();

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


uint32_t inet_addr(const char *cp) {

    struct in_addr val;

    if (inet_aton(cp, &val))
        return (val.s_addr);
    return (0);
}

/*
 * Check whether "cp" is a valid ASCII representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
int
inet_aton(const char *cp, struct in_addr *addr) {
    u_long parts[4];
    uint32_t val;
    char *c;
    char *endptr;
    int gotend, n;

    c = (char *)cp;
    n = 0;
    /*
     * Run through the string, grabbing numbers until
     * the end of the string, or some error
     */
    gotend = 0;
    while (!gotend) {
        errno = 0;
        val = strtoul(c, &endptr, 0);

        if (errno == ERANGE)    /* Fail completely if it overflowed. */
            return (0);

        /*
         * If the whole string is invalid, endptr will equal
         * c.. this way we can make sure someone hasn't
         * gone '.12' or something which would get past
         * the next check.
         */
        if (endptr == c)
            return (0);
        parts[n] = val;
        c = endptr;

        /* Check the next character past the previous number's end */
        switch (*c) {
        case '.' :
            /* Make sure we only do 3 dots .. */
            if (n == 3) /* Whoops. Quit. */
                return (0);
            n++;
            c++;
            break;

        case '\0':
            gotend = 1;
            break;

        default:
            if (isspace((unsigned char)*c)) {
                gotend = 1;
                break;
            } else
                return (0); /* Invalid character, so fail */
        }

    }

    /*
     * Concoct the address according to
     * the number of parts specified.
     */

    switch (n) {
    case 0:             /* a -- 32 bits */
        /*
         * Nothing is necessary here.  Overflow checking was
         * already done in strtoul().
         */
        break;
    case 1:             /* a.b -- 8.24 bits */
        if (val > 0xffffff || parts[0] > 0xff)
            return (0);
        val |= parts[0] << 24;
        break;

    case 2:             /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff || parts[0] > 0xff || parts[1] > 0xff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 3:             /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff || parts[0] > 0xff || parts[1] > 0xff ||
            parts[2] > 0xff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }

    if (addr != NULL)
        addr->s_addr = htonl(val);
    return (1);
}

static int32_t net_if_connect(struct net_socket *socket, 
                              const char *ip,
                              unsigned short port)
{
    static struct sockaddr_in addr;
    static BOOL in_progress = FALSE;
    uint32_t status = ERR_WOULD_BLOCK;;
    int socket_status;
    
    if (socket->connected)
        return ERR_SUCCESS;
    
    if (!in_progress) {
        debug_log(DEBUG_NET, ("Connect to %s:%d\n", ip, port));
        addr.sin_port = (uint16_t)port;
        addr.sin_addr.S_un.S_addr = inet_addr(ip);
        in_progress = TRUE;
    }
    
    do {
        socket_status = connect( socket->sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr));

        if (socket_status == SOCKET_ERROR) {
            switch (errno) {
            case EINPROGRESS:
                status = ERR_WOULD_BLOCK;
                break;
            default:
                error_log(DEBUG_NET, ("Connect failed\n"), errno);
                status = ERR_FAILURE;
                break;
            }

        } else {
            debug_log(DEBUG_NET, ("Connected to %s:%d\n", ip, port));
            status = ERR_SUCCESS;
            socket->connected = TRUE;
        }
    } while(status == ERR_WOULD_BLOCK && !socket->non_blocking);

    if (status != ERR_WOULD_BLOCK)
        in_progress = FALSE;

    return status;
}

static int net_if_receive(struct net_socket *net_if, char *buf, size_t size, size_t *nbytes_recvd)
{
    int recvd;
    int sd = net_if->sockfd;

    recvd = (int) recv(sd, buf, size, net_if->rflags);

    if (recvd == SOCKET_ERROR) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return ERR_FAILURE;
    }

    if (recvd == 0) {
        error_log(DEBUG_NET, ("Remote host closed the connection\n"), ERR_FAILURE);
        return ERR_FAILURE;
    }

    *nbytes_recvd = (size_t)recvd;

    return ERR_SUCCESS;
}

static int net_if_send(struct net_socket *net_if, const char *buf, size_t size, size_t *nbytes_sent)
{
    int sd = net_if->sockfd;
    int sent;

    sent = send(sd, buf, (int)size, net_if->wflags);

    if (sent == SOCKET_ERROR) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return ERR_FAILURE;
    }

    *nbytes_sent = (size_t)sent;
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
    static BOOL        in_progress = FALSE;
    
    int32_t status = ERR_WOULD_BLOCK;

    (void)self;

    if (!in_progress) {

        hostname_cpy = sf_malloc(strlen(hostname) +1);
        strcpy(hostname_cpy, hostname);

        debug_log(DEBUG_NET, ("DNS lookup: %s\n", hostname));
        in_progress = TRUE;
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
        in_progress = FALSE;
        check_memory();
    }

    return status;
}

static int net_if_close(struct net_socket *net_if)
{
    debug_log(DEBUG_NET, ("Close socket (%d)\n", net_if->sockfd));
            
    closesocket(net_if->sockfd);
    sf_free(net_if);

    check_memory();

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

