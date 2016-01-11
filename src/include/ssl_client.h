/**
 * @file ssl_client.h Interface for ssl_client constructor
 *
 * */

#ifndef SSL_CLIENT_H
#define SSL_CLIENT_H

#include <porting/net_port.h>
#include <sdk_config.h>
/**
 * This constructs an ssl client in form of net_dev_operations
 * This is a decorator object, it takes a network device and returns another one
 * It uses the network device it got as a low level interface
 *
 * @param[in] net_dev The 'low level' network device
 * @param[in] type    The tranport protocol type
 *
 * @return The ssl network device or NULL if failed
 *
 * */
#ifdef BUILD_SSL
struct net_dev_operations *ssl_client_new(struct net_dev_operations *net_dev, enum transport_protocol_type type);

/**
 * @brief Initalizes the SSL library and the connecting wrappers
 *
 * It shall only be called once at system startup
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 * */
int32_t ssl_client_init(void);
#else
static inline struct net_dev_operations *ssl_client_new(struct net_dev_operations *net_dev, enum transport_protocol_type type)
{
    (void)net_dev;
    (void)type;
    return NULL;
}

static inline int32_t ssl_client_init(void)
{
    return ERR_SUCCESS;
}
#endif

#endif
