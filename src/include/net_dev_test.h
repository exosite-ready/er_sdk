/**
 * @file net_dev_test.h
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
 * @brief The interface to create a net_dev_test device
 *
 * This test device adds various fault injection capabilities to a
 * wrapped network device
 *
 **/

#ifndef NET_DEV_TEST_H_
#define NET_DEV_TEST_H_

#ifdef TEST_ERSDK
/**
 * @brief Creates a net_dev_test device
 *
 * @param[in] dev   A pointer to a network device which will be used to route the calls to
 *
 * The net_dev_test device only adds additional testing capabilities; but otherwise it uses the
 * underlying net device
 **/
struct net_dev_operations *net_dev_test_new(struct net_dev_operations *dev);
#else
static inline struct net_dev_operations *net_dev_test_new(struct net_dev_operations *dev)
{
    (void)dev;
    return NULL;
}
#endif



#endif /* NET_DEV_TEST_H_ */
