/**
 * @file Factory interface for network devices
 *
 * */

#ifndef NET_DEV_FACTORY_H_
#define NET_DEV_FACTORY_H_

#include <protocol_client_common.h>

/**
 * Initalizes the net device factory
 * This should be called from SDK init
 *
 * Always succeeds
 **/
void net_dev_factory_init(void);

/**
 * Factory method for getting a network client
 * This client can be used to send/receive data from a remote server
 *
 * @param[in] protocol_type Transport protocol type
 * @param[in] security
 * @return pointer to a network device (net_dev_operations) or NULL if failed
 *
 * */
int32_t net_dev_get(struct net_dev_operations **new_net_dev, enum transport_protocol_type type, enum security_mode security);
#endif /* NET_DEV_FACTORY_H_ */
