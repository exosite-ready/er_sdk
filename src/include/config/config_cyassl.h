/**
 * @file config_cyassl.h
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
 * @brief This file adds the default cyassl configuration
 *
 * HAVE_CONFIG_H has to be defined so that all these configurations take effect in
 * cyassl
 *
 **/

#ifndef ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_CYASSL_H_
#define ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_CYASSL_H_

#include <system_utils.h>
#include <stdlib.h>

#define CYASSL_USER_IO
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_EVDEV
#define NO_WRITEV
#define USER_TIME
#define USE_FAST_MATH
#define CYASSL_SMALL_STACK
#define TFM_TIMING_RESISTANT
#define CYASSL_DTLS
#define HAVE_AESCCM
#define NO_HC128
#define NO_RABBIT
#define NO_DSA
#define NO_DES3
#define NO_MD4
#define NO_RC4
#define NO_PWDBASED
#define NO_CYASSL_SERVER

static int call_num;
inline static int cyassl_client_generate_seed(void)
{
    if (call_num == 0)
        srand(system_get_time());

    if ((call_num % 24) == 23) {
        call_num = 0;
    }

    call_num++;
    return rand() % 256;
}

#define NO_DEV_RANDOM
#define CUSTOM_RAND_GENERATE cyassl_client_generate_seed
#endif /* ER_SDK_SRC_INCLUDE_CONFIG_CONFIG_CYASSL_H_ */
