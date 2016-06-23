/**
 * @file protocol_client_common.h
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
 * @brief This file adds public types used by the protocol state machines
 **/
#ifndef INC_EXOSITE_API_DULL_H_
#define INC_EXOSITE_API_DULL_H_

enum application_protocol_type {
    APP_PROTO_HTTP,
    APP_PROTO_COAP,
    APP_PROTO_JSON
};

enum security_mode {
    SECURITY_OFF,
    SECURITY_TLS_DTLS,
    SECURITY_EXTERNAL
};

enum state_machine_type {
    SM_SYNCH,
    SM_ASYNCH
};

#endif /* INC_EXOSITE_API_DULL_H_ */
