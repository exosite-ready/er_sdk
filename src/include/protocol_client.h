/**
 * @file protocol_client.h
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
 * @defgroup protocol_client Protocol Client interface
 * @{
 * @brief
 * Protocol clients used by the @ref protocol_client_sm have to implement @ref protocol_client_ops
 *
 * Additionally protocol clients have to implement a constructor that creates a @ref protocol_client_class,
 * and initalizes the @ref protocol_client_class::ops and net_if members
 *
 **/

#ifndef ER_SDK_SRC_INCLUDE_PROTOCOL_CLIENT_H_
#define ER_SDK_SRC_INCLUDE_PROTOCOL_CLIENT_H_

struct protocol_client_class {
    /** @brief The methods of this class
     *
     * The ops field should be set by the constructor
     * */
    struct protocol_client_ops *ops;
    /** @brief The network interface used by this protocol client
     *
     * The network interface should be set by the constructor
     *  */
    struct net_dev_operations *net_if;
    /** @brief The socket used by this class
     *
     *  This will be set by the set_socket call
     **/
    struct net_socket *socket;
    /** @brief Internal context pointer*/
    void *ctx;
};

struct protocol_client_ops {
    /**
     * @brief Sets a socket for this rest object
     *
     * The socket represents the communication channel, so until this is done
     * the object won't be able to communicate
     * */

    void (*set_socket)(struct protocol_client_class *self, struct net_socket *socket);
    /**
     * @brief Clears the socket, thus closing the communication channel
     *
     * */
    void (*clear_socket)(struct protocol_client_class *self);

    /**
     * @brief Send a RESTful request
     *
     * @param[in] self - the protocol client object (HTTP or COAP currently)
     * @param[in] request  The request: this is defined by the underlying implementation, that's why
     *                     it is void - the request has to be created with create_request, see below
     * @return ERR_SUCCESS on success ERR_* error code otherwise
     * */
    int32_t (*send_request)(struct protocol_client_class *self, void *request);

    /**
     * @brief Try to read a response to the previous request
     *
     *  - It returns ERR_SUCCESS if it successfully read and parsed a response. In that case it also
     * sets the response code, the message id copies the payload to data and sets the payload size.
     *  - It returns ERR_WOULD_BLOCK if it could not read a response.
     *  - It returns other ERR_ error code if it could read a response but it could not parse it
     *
     * @param[in] self -         the protocol object (HTTP or COAP currently)
     * @param[out] response code The response code
     * @param[out] message_id    The token that identifies the request - response pair
     * @param[out] data          The response itself in null terminated format (c string)
     * @param[in]  len           The length of the ouput buffer

     * @return ERR_SUCCESS on success ERR_* error code otherwise
     * */
    int32_t (*recv_response)(struct protocol_client_class *self, int32_t *response_code,
            int32_t *message_id, char *data, size_t max_len, size_t *payload_size);

    /**
     * @brief Close the protocol client context
     *
     * A protocol client implementation might need some cleanup; e.g there can be internal queues, etc
     * that need to be cleaned up between socket context changes
     * E.g this can as a separator between requests if the request/response was not successful
     * Otherwise it can happen that a previous request is retransmitted and we get a response
     * to that
     * @return ERR_SUCCESS on success ERR_* error code otherwise
     **/
    void (*close)(struct protocol_client_class *self);

    /**
     * @brief Returns the status of the response
     *
     * @param[in] self -         the protocol client object (HTTP or COAP currently)
     * @param[out] response code The response code
     *
     * @return TRUE if the response code is not considered an error FALSE otherwise
     **/
    BOOL (*get_response_status)(struct protocol_client_class *self, int32_t response_code);
};

#endif /* ER_SDK_SRC_INCLUDE_PROTOCOL_CLIENT_H_ */

/** @} end of protocol_client group */
