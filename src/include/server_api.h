/**
 * @file server_api.h
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
 * @brief This file define the interface to create server-requests
 *
 * The interface itself is function pointer table
 * These requests are server dependent; that's where the name comes from
 *
 * E.g exosite_http_api.c implements this interface; then this interface can
 * be used to create requests (in a server api idependent manner)
 *
 * This interface is used in exosite_api.c to create the requests
 **/

#ifndef ER_SDK_SRC_INCLUDE_SERVER_API_H_
#define ER_SDK_SRC_INCLUDE_SERVER_API_H_

#include <exosite_api_internal.h>

struct server_api_request_ops {
    /**
     * Creates new server api request
     *
     * @param[out] pdu    The new request
     *
     * @return ERR_SUCCESS on success ERR_* error code otherwise
     * */
    int32_t (*request_new)(void **pdu, void *request_args);

    /**
     * Deletes a request - This is only to be called if an error happens, otherwise
     * the rest implementation should delete the request
     * TODO this is not very clean, but the current coap implementation does that so
     * we had to adapt to that
     *
     * @param[in] pdu    The request to be deleted
     *
     * @return ERR_SUCCESS on success ERR_* error code otherwise
     * */
    int32_t (*request_delete)(void *pdu);

    /**
     * @brief This callback function is called when the response is received
     *        It converts the server api response/response code to a unique format which is
     *        understood by the exosite_api layer
     *
     * @param[in/out] payload      Response payload
     * @param[in] size          Actual size of the payload in the buffer
     * @param[in] max_size      The size of the buffer where the payload is
     * @param[in] response_code The response code from the header
     * @param[in] request_args The request parameters; so that parsing can be done based on request_args->methods
     */
    int32_t (*response_callback)(struct exo_request_args *request_args,
                                 char *payload,
                                 size_t size,
                                 size_t max_size,
                                 int32_t response_code
                                 );
};




#endif /* ER_SDK_SRC_INCLUDE_SERVER_API_H_ */
