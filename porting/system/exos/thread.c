/**
 * @file thread.c
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
 * @brief System_thread implementation for EXOS
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <assert.h>
#include <porting/thread_port.h>

int32_t system_thread_create(system_thread_t *thread, const system_thread_attr_t *attr,
        void*(*start_routine)(void *), void *param)
{
    return ERR_NOT_IMPLEMENTED;
}

