/**
 * @file net_if.h
 * This is the network interface for OpenTether; every network device shall use
 * this interface
 *
 * Currently this only supports client operation
 *
 * */

#ifndef NET_PORT_H
#define NET_PORT_H

#include <lib/type.h>

/** @defgroup net_port_interface Network Port Interface
 *  This interface has to be implemented by the user code
 *  to provide the necessary network services for the SDK.
 *
 *  @image html ER_SDK_network_if.jpg
 *
 *  @{
 */

struct net_socket;
struct net_dev_operations;
struct link_layer_if;

/** Storing of the MAC address */
struct mac_addr {
    char addr_bytes[6];
};

/** Transport protocol identifiers */
enum transport_protocol_type {
    TRANSPORT_PROTO_TCP,    /**< TCP Protocol */
    TRANSPORT_PROTO_UDP     /**< UDP Protocol */
};

/**
 * Constructor for a TCP network interface class
 *
 * @returns A pointer to struct net_dev_operations on success
 *          NULL on error
 */
struct net_dev_operations *net_dev_tcp_new(void);

/**
 * Constructor for a UDP network interface class
 *
 * @returns A pointer to struct net_dev_operations on success
 *          NULL on error
 */
struct net_dev_operations *net_dev_udp_new(void);

/**
 * Constructor for a HTTP server interface class
 *
 * @returns A pointer to struct net_dev_operations on success
 *          NULL on error
 */
struct net_dev_operations *net_dev_http_server_new(void);

/**
 * This function has to be implemented if a link layer device
 * is to be used. The SDK will call this function.
 *
 * @return none
 */
void net_dev_init(void);

/**
 * Net_dev operations is a mix of socket operations (connect, recv, send, etc)
 * and network operations (like lookup)
 * This is done so for simplicity
 */
struct net_dev_operations {
    /**
     * Creates a client side network socket object; this object will be used with all socket
     * operations
     * @TODO this should take some more parameters to define non blocking, blocking, etc
     * currently all implementations are NON-blocking
     *
     * @param[in]  self   The pointer to the object itself
     * @param[out] socket The new net socket object
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*socket)(struct net_dev_operations *self, struct net_socket **socket);

    /**
     * Creates a server side network socket object; this object will be used with all socket
     * operations
     * @TODO this should take some more parameters to define non blocking, blocking, etc
     * currently all implementations are NON-blocking
     *
     * @param[in]  self          The pointer to the object itself
     * @param[out] server_socket The new server side net socket object
     * @param[in] ip             IP address in character format
     * @param[in] port           Port number
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*server_socket)(struct net_dev_operations *self,
                             struct net_socket **server_socket,
                             const char *ip,
                             unsigned short port);

    /**
     * Connects the socket to the ip/port given
     *
     * @param[in] socket The socket object that needs to be connected, it must exist
     * @param[in] ip in character format
     * @param[in] port
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*connect)(struct net_socket *sock, const char* ip, uint16_t port);

    /**
     * The accept() function shall extract the first connection on the queue of pending
     * connections, create a new socket with the same socket type protocol and address
     * family as the specified socket.
     *
     * @param[in] socket The socket object that needs to be connected, it must exist
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*accept)(struct net_socket *server_socket, struct net_socket **socket);

    /**
     * Closes the socket
     *
     * @param[in] socket The socket object that needs to be closed, it must exist
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*close)(struct net_socket *sock);

    /**
     * Tries to receive on the socket
     *
     * @param[in] socket The socket object from which we try to read, it must exist
     * @param[out] buf The buffer for the data to be received
     * @param[in] sz   The size of the buffer (the maximum size to be copied into the buffer)
     * @param[out] nbytes_received The number of bytes received
     *
     * @return ERR_SUCCESS for success ERR_* error code otherwise
     */
    int32_t (*recv)(struct net_socket *sock, char *buf, size_t sz, size_t *nbytes_received);

    /**
     * Tries to send on the socket
     *
     * @param[in] socket The socket object to which we try to write, it must exist
     * @param[out] buf The buffer for the data to be sent
     * @param[in] sz   The size of the buffer
     * @param[out] nbytes_sent  The number of bytes sent
     *
     * @return ERR_SUCCESS for success ERR_* error code otherwise
     */
    int32_t (*send)(struct net_socket *sock, const char *buf, size_t sz, size_t *nbytes_sent);

    /**
     * Tries to get an IP for the host name given; this is not a socket operation
     *
     * @param[in]  self   The pointer to the object itself
     * @param[in] hostname The host that we try to resolve
     * @param[out] ip_addr_str The ip of the host given in character format
     * @param[in] sz   The size of the buffer for the IP (the maximum size to be copied into the buffer)
     *
     * @return error code (0 for success, negative number otherwise)
     */
    int32_t (*lookup)(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len);

    /**
     * Currentl unused
     */
    int32_t (*config)(struct mac_addr* mac);

    /**
     * Sets the identity for this Interface
     *
     * @TODO this is a workaround: the current DTLS scheme requires the Device's RID to be set as
     * identity for PSK
     */
    int32_t (*set_identity)(const char *identity, const char *passphrase);

    /**
     * This is not a socket operation; this is basically a destructor for network devices
     *
     * @TODO once the socket operations and net_dev operations are separated this will be moved
     * from here see TODO above
     */
    void (*deinit)(void);
};

/**
 * This is the socket object
 *
 * @TODO this is basically a private object, should be moved to a private header
 * and just provide forward declarations
 */
struct net_socket {
    int32_t sockfd;                 /**< Socket ID Identifier */
    struct net_dev_operations *ops; /**< The operations for this sockt*/
    int32_t rflags;
    int32_t wflags;
    int32_t dtls;
    int32_t non_blocking;
    int32_t connected;
    int32_t already_started;        /**< For platforms that have only one function
                                         to create and connect a socket like Microchip WCM*/
    int32_t addr;
    int32_t port;
};

/** @} end of net_port_interface group */

#endif /* NET_PORT_H */

