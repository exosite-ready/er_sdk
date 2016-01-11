
/**
 * @defgroup client_config Client Configuration
 * The client configuration module contains private settings
 * of the SDK.
 *
 * @warning It is not part of the user API. All of the settings
 *          can be modified by the users are located in the
 *          config_port.h
 * @{
 */

#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <lib/type.h>
#include <sdk_config.h>
#include <protocol_client.h>
#include <protocol_client_common.h>
#include <exosite_api_internal.h>

#define NO_REQUEST_LIMIT          (0)
#define MAX_PAYLOAD_SIZE          (512)

/**
 * @brief Transmission parameters
 *
 * This structure contains the transmission configuration parameters
 * These can be set in client_config.c
 **/
struct transmission_config_class {
    int32_t    connect_retries;                /*!< How many times to retry connect before the whole connect process is restarted */
    int32_t    transmission_retries;           /*!< How many times to retry send/receive before the connection is closed/reopened */
    sys_time_t connect_timeout;                /*!< Try to connect for this long */
    sys_time_t transmission_timeout;           /*!< If there is no response for a request this long the SM will report an error */
    size_t     max_request_queue_size;         /*!< The maximum number of requests in the queue, if a new request arrives when this
                                                    is reached it will be dropped*/
    size_t     protocol_client_sm_queue_size;  /*!< The maximum number of requests in the protocol client sm queue,
                                                    it is a private setting; the user should only configure the request queue size*/
    int32_t parallel_request_limit;            /*!< Maximum number of requests that can be processed in parallel*/
    enum security_mode security;               /*!< The security mode for transmission */
    BOOL forced_activation;                    /*!< Set to TRUE if activation has to be forced */
};

/**
 * @brief Timing parameters
 *
 * This structure contains the periodic configuration parameters
 * These can be set in client_config.c
 **/
struct periodic_config_class {
    timer_data_type rtc_period;       /*!< The period of the Real Time clock task*/
    timer_data_type sm_period;        /*!< The period of the Protocol client state machine task */
};

/**
 * @brief Server info
 *
 * This structure contains the server configuration data for an
 * application_protocol_type/security pair
 *
 * The server configuration data consists of a server name and a server port number
 **/
struct server_data_class {
    enum application_protocol_type protocol_type;  /*!< Protocol type (HTTP, Co-AP, etc.) */
    enum security_mode security;                   /*!< Security mode */
    const char *server_name;                       /*!< Server host name */
    uint16_t server_port;                          /*!< Server port */
};

/**
 * @brief Protocol configuration
 *
 * This structure contains the protocol class configuration
 * A structure of this type has to be passed to the protocol_client_sm_new
 * */
struct protocol_client_config_class {
    enum application_protocol_type appproto_type;
    const char *name;
    void (*new)(struct protocol_client_class **obj, struct net_dev_operations *net_if);
    enum transport_protocol_type tproto_type;
    enum state_machine_type stype;
    int32_t rq_limit;
    struct protocol_client_class *exo_api;
};

/**
 * @brief Server API class
 */
struct server_api_class {
    enum application_protocol_type appproto_type;
    const char *name;
    void (*new)(struct server_api_request_ops **obj);
    struct server_api_request_ops *api_ops;
};


struct heap_config {
    size_t size;
    const char *name;
    int32_t id;
};


/**
 *  @brief Get transmission config object
 *
 *  @note The transmission config object is unique for a system
 *
 *  @return the transmission config object on SUCCESS or NULL on FAILURE
 **/
struct transmission_config_class *client_config_get_transmission_config(void);

/**
 *  @brief Get periodic config object
 *
 *  @note The periodic config object is unique for a system
 *
 *  @return the periodic config object on SUCCESS or NULL on FAILURE
 **/
struct periodic_config_class *client_config_get_periodic_config(void);

/**
 *  @brief Get server config object
 *
 *  @note The server config object is unique for a system
 *
 *  @return the server config object on SUCCESS or NULL on FAILURE
 **/
struct server_data_class **client_config_get_server_data(void);

/**
 *  @brief Get protocol client config object
 *
 *  The protocol client config will contain a newly allocated protocol client object
 *
 *  @param[out] client_config          Client configuration object
 *  @param[in]  type                   Protocol type
 *  @param[in]  security               Security mode
 *
 *  @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t client_config_get_protocol_client_config(struct protocol_client_config_class **client_config,
                                                 enum application_protocol_type type,
                                                 enum security_mode security);

/**
 *  @brief Get server_api_request object
 *
 *  @param[out] client_config          Server API request object
 *  @param[in]  type                   Protocol type
 *
 *  @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t client_config_get_server_api(struct server_api_request_ops **new_request_ops,
                                     enum application_protocol_type type);

#ifdef BUILD_HTTP
/**
 *  @brief HTTP client Constructor
 *
 *  @param[out]   obj                  The newly created object
 *  @param[in]    net_dev              The network device which is used for communication
 **/
void http_client_new(struct protocol_client_class **obj, struct net_dev_operations *net_dev);

/**
 *  @brief Exosite HTTP API request Constructor
 *
 *  @param[out]   obj                  The newly created object
 **/
void exosite_http_api_request_new(struct server_api_request_ops **obj);
#else
static inline void http_client_new(struct protocol_client_class **obj,
        struct net_dev_operations *net_if)
{ (void)(obj); (void)(net_if); }
static inline void exosite_http_api_request_new(struct server_api_request_ops **obj)
{ (void)(obj); }
#endif

#ifdef BUILD_COAP
/**
 *  @brief Libcoap client Constructor
 *
 *  @param[out]   obj                  The newly created object
 *  @param[in]    net_dev              The network device which is used for communication
 *  @param[in]    response_callback    A callback to be called when a response is received
 **/
void coap_client_new(struct protocol_client_class **obj,
        struct net_dev_operations *net_if);

/**
 *  @brief Exosite COAP API request Constructor
 *
 *  @param[out]   obj                  The newly created object
 **/
void exosite_coap_new(struct server_api_request_ops **obj);
#else

static inline void coap_client_new(struct protocol_client_class **obj, struct net_dev_operations *net_if)
{ (void)(obj); (void)(net_if); }
static inline void exosite_coap_new(struct server_api_request_ops **obj)
{ (void)(obj); }

#endif


struct heap_config **client_config_get_heap_config(void);

#endif

/** @} end of client_config group */
