/**
 * @file exosite_api_internal.h
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
 * @brief Private types for exosite_api module
 **/
#ifndef INC_EXOSITE_API_INTERNAL_H_
#define INC_EXOSITE_API_INTERNAL_H_
#include <sdk_config.h>
#include <lib/type.h>
#include <porting/net_port.h>
#include <exosite_api.h>

struct exo_request_args;

/**
 * @brief Request types
 *
 * These are the request types that can be sent to the exosite servers
 *
 */
enum exosite_method {
    /** - Data Procedures */
    EXO_DP_WRITE, EXO_DP_READ, EXO_DP_READ_WRITE, EXO_DP_SUBSCRIBE,

    /**  - Provisioning Procedures */
    EXO_PP_ACTIVATE, EXO_PP_LIST_CONTENT, EXO_PP_DOWNLOAD_CONTENT,

    /** - Utility Procedures */
    EXO_UP_TIMESTAMP, EXO_UP_IP
};

/**
 * @brief Read callback
 *
 * see exosite_on_read
 */
typedef exosite_on_read on_read_clb;

/**
 * @brief Write callback
 *
 * see exosite_on_write
 */
typedef exosite_on_write on_write_clb;

/**
 * @brief Subscribe callback
 *
 * Subscribe callbacks are handled in 2 steps:
 *  - 1st the exosite_api will handle the callback and determine
 *    whether a new subscribe request needs to be initiated or not;
 *  - 2nd the exosite_api subscribe callback handler will call the callback
 *    provided by the user if necessary.
 *
 * @param[in] 1st  Status
 * @param[in] 2nd  Protocol type
 * @param[in] 3rd  Request arguments
 * @param[in] 4th  Value
 */
typedef void (*on_subscribe_clb)(int32_t,
                                 enum application_protocol_type,
                                 struct exo_request_args *,
                                 const char *);

/**
 * @brief Activation callback
 *
 * @param[in] 1st  Status
 * @param[in] 2nd
 * @param[in] 3rd  Request arguments
 */
typedef void (*on_activate_clb)(int32_t, const char *, struct exo_request_args *);

/**
 * @brief Timestamp callback
 *
 * @param[in] 1st  Status
 * @param[in] 2nd
 * @param[in] 3rd  Request arguments
 */
typedef void (*on_timestamp_clb)(int32_t, const char*, struct exo_request_args *);


struct exo_request_args {
    enum application_protocol_type app_type;    /*!< Stores the application type of this server_api */
    struct server_api_request_ops *server_api;  /*!< Server API object */
    union {
        on_read_clb read;               /*!< Read callback */
        on_write_clb write;             /*!< Write callback */
        on_subscribe_clb subscribe;     /*!< Subscribe callback */
        on_activate_clb activate;       /*!< Activate callback */
        on_timestamp_clb get_timestamp; /*!< Timestamp callback */

        void *no_type;                  /*!< The callback functions has different prototypes, so it is passed as
                                         *   void * to the lower layers. The callback handler will then figure out
                                         *   which to call based on the method. */

    } callback;                   /*!< Request callbacks. It is called when the request is processed */
    exosite_on_change subscribe_usr_callback;
    enum exosite_method method;   /*!< The method type, EXO_DP_WRITE, EXO_DP_READ, etc. */
    int32_t message_id;           /*!< This message ID is used to pair requests to responses*/
    void *clb_params;             /*!< This pointer can be passed to the callback function assigned to the request */
    void *exo;                    /*!< The exosite object */
    char cik[41];                 /*!< The CIK of the device to which the request is sent */
    const char *alias;            /*!< The resource alias */
    const char *value;            /*!< The value to be written to the resource (EXO_DP_WRITE) */
    const char *content;          /*!< The content to be sent */
    const char *timeout;          /*!< Timeout parameter of the "Long Polling" method */
    const char *timestamp;        /*!< Timestamp parameter of the "Long Polling" method */
    const char *vendor;           /*!< Vendor of the device */
    const char *model;            /*!< Model of te device */
    const char *sn;               /*!< Serial number of the device */
    const char *id;               /*!< Content Id */
};

/*
 * With this test function it is possible to emulate the receptions of result
 *
 * With this it is possible to test scenarios where late responses might have an effect

*/
#ifdef TEST_ERSDK
void exosite_sm_inject_result(struct exosite_class *exo, enum exosite_method, void *callback);
#endif
#endif /* INC_EXOSITE_API_INTERNAL_H_ */
