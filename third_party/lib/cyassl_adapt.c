/**
 * @file cyassl_adapt.c
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
 * @brief
 * This module implements the CyaSSL_set_quiet_shutdown function for cases when
 * OPENSSL_EXTRA is not turned on
 *
 * By default it is not turned on for the ER_SDK
 * If it is turned on then do not compile this file
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cyassl/internal.h>

void CyaSSL_set_quiet_shutdown(CYASSL* ssl, int mode)
{
    CYASSL_ENTER("CyaSSL_CTX_set_quiet_shutdown");
    if (mode)
        ssl->options.quietShutdown = 1;
}
