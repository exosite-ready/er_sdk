/**
 * @file sdk_init.c
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
 * This file implements the initalization of the SDK
 **/
#include <assert.h>
#include <stdio.h>

#include <sdk_config.h>
#include <lib/error.h>
#include <src/lib/heap_space_internal.h>
#include <lib/mem_check.h>
#include <lib/string_class.h>
#include <lib/sf_malloc.h>
#include <lib/debug.h>
#include <lib/pclb.h>

#include <system_utils.h>
#include <client_config.h>

#include <net_dev_factory.h>
#include <rtc_if.h>
#include <protocol_client_sm.h>
#include <exosite_api.h>


#ifndef SYSTEM_THREAD_STACK_SIZE
#define SYSTEM_THREAD_STACK_SIZE    (0)
#endif

static char *memory_buffer;
static size_t memory_buffer_size;

void exosite_sdk_register_memory_allocators(exosite_malloc_func exo_malloc,
                                            exosite_free_func exo_free,
                                            exosite_realloc_func exo_realloc)
{
    internal_set_allocators(exo_malloc, exo_free, exo_realloc);
}

void exosite_sdk_register_memory_buffer(void *buf, size_t size)
{
    memory_buffer = buf;
    memory_buffer_size = size;
}

int32_t exosite_sdk_init(log_function log_fn, debug_callback debug_cb)
{
    int32_t error;
    struct periodic_config_class *periodic_config;

    /** Initializes the debug sub-system. */
    debug_init(log_fn, debug_cb);

    periodic_config = client_config_get_periodic_config();

    /** Initializes the @ref system_port_interface adapter layer. */
    error = system_init();
    if (error != ERR_SUCCESS) {
        ASSERT(0);
        return error;
    }

    error = hm_init(memory_buffer, memory_buffer_size);
    if (error != ERR_SUCCESS)
        return error;

    /** Initializes the @ref pclb */
    error = pclb_init(SYSTEM_THREAD_STACK_SIZE);
    if (error != ERR_SUCCESS) {
        ASSERT(0);
        return error;
    }

    /** Initializes the @ref net_port_interface adapter layer. */
    net_dev_factory_init();

    /** Initializes the @ref protocol_client_sm module. */
    protocol_client_sm_init();

    pclb_register(periodic_config->rtc_period, rtc_periodic, NULL, NULL);

    system_print_version();

    return ERR_SUCCESS;
}
