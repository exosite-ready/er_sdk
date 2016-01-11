/**
 * @file protocol_client_sm.h
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
 * @brief This is the interface to the protocol client state machine
 *
 * @defgroup protocol_client_sm Protocol Client State-Machine interface
 *
 * @brief The protocol client state machine handles the request sending and receiving of the responses
 * in a protocol independent manner.
 * @{
 */

#ifndef INC_EXOSITE_API_SM_H_
#define INC_EXOSITE_API_SM_H_

#include  <protocol_client_common.h>
#include <server_api.h>

struct protocol_client_sm_class;

enum request_type {
    REQUEST_NORMAL,
    REQUEST_SUBSCRIBE,
    REQUEST_SUBSCRIBE_ASYNCH
};

struct protocol_client_response {
    char payload[MAX_PAYLOAD_SIZE];
    size_t max_payload_size;
    size_t payload_size;
    int32_t response_code;
    int32_t id;
    enum request_type request_type;
    BOOL status;
    void *sm;
};

typedef void (*pc_sm_callback)(void *, void *, struct protocol_client_response *);

/**
 * @brief Initalizes the protocol client state machine class
 *
 * The protocol client SM sends requests and receive responses asynchronously and tries to match
 * them.
 *
 * @return None
 **/
void protocol_client_sm_init(void);

/**
 * @brief Creates a new protocol client SM
 *
 * @param[out] self         The new object
 * @param[in]  name         The name of the state machine
 * @param[in]  protocol     The protocol client config object
 * @param[in]  server_name  The name of the remote server (which the requests are sent to)
 * @param[in]  server_port  The port of the remote server (which the requests are sent to)
 *
 * @return ERR_SUCCESS on success or ERR_* error code otherwise
 **/
int32_t protocol_client_sm_new(struct protocol_client_sm_class **exo,
                               const char *name,
                               struct protocol_client_config_class *protocol,
                               const char *server_name,
                               uint16_t server_port);

/**
 * @brief Deletes a new protocol client SM
 *
 * @param[in] self         The object to be deleted
 *
 * @return ERR_SUCCESS on success or ERR_* error code otherwise
 **/
extern int32_t protocol_client_sm_delete(struct protocol_client_sm_class *exo);

/**
 * @brief Queues a request to the protocol client SM
 *
 * @param[in] self          The object itself
 * @param[in] type          The type of the request
 * @param]in] request       The request object: it will be passed to the protocol client
 * @param[in] request_args  The request_args object: it will be passed to the upper layer when the
 *                          callback is called
 * @param[in] message_id    The id of the request, based on which they will be matched to responses
 * @param[in] resp_clb      The callback to be called when there is a response/timeout
 *
 * @return ERR_SUCCESS on success or ERR_* error code otherwise
 **/
int32_t protocol_client_sm_transmit(struct protocol_client_sm_class *self, enum request_type type,
        void *request, void *request_args, int32_t message_id, pc_sm_callback resp_clb);


/**
 * @brief Queries the protocol client SM to figure out whether there is space for the new request
 *
 * @param[in] self          The object itself
 *
 * @return TRUE if there is no space available FALSE otherwise
 **/
BOOL protocol_client_sm_is_queue_full(struct protocol_client_sm_class *self);

/**
 * @brief Removes all queued requests from the protocol client SM
 *
 * The callbacks will be called for every requests with error code: ERR_REQUEST_ABORTED
 *
 * @param[in] self         The SM object
 *
 **/
void protocol_client_sm_clear(struct protocol_client_sm_class *self);

/**
 * @brief Check whether the SM contains any queued request
 *
 * @param[in] self         The SM object
 *
 * @return TRUE if there are no more queued requests FALSE otherwise
 **/
BOOL protocol_client_sm_is_empty(struct protocol_client_sm_class *self);

/**
 * TODO: it is used with COAP/DTLS; this is not yet released
 **/
void protocol_client_sm_set_identity(struct protocol_client_sm_class *exo,
                                     const char *identity,
                                     const char *passphrase);

#endif /* INC_EXOSITE_API_SM_H_ */

/** @} end of protocol_client_sm group */
