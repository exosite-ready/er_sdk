/**
 * @file mutex.c
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
 * @brief System_mutex implementation for MQX
 **/
#include <stdint.h>
#include <assert.h>
#include <lib/error.h>
#include <lib/type.h>
#include <porting/thread_port.h>
#include <lib/sf_malloc.h>
#include <mqx.h>
#include <stdio.h>

int32_t system_mutex_create(system_mutex_t *mutex)
{
    LWSEM_STRUCT_PTR sem;
    int error;

    error = sf_mem_alloc((void **)&sem, sizeof(*sem));
    if (error != ERR_SUCCESS)
        return error;

    error = _lwsem_create(sem, 1);
    if (error != MQX_OK) {
        sf_mem_free(sem);
        return ERR_FAILURE;
    }

    *mutex = sem;
    return ERR_SUCCESS;
}

int32_t system_mutex_destroy(system_mutex_t mutex)
{
    assert(0);
    return ERR_NOT_IMPLEMENTED;
}

int32_t system_mutex_lock(system_mutex_t mutex)
{
    int error;
    LWSEM_STRUCT_PTR sem = mutex;

    error = _lwsem_wait(sem);
    if (error != MQX_OK)
        return ERR_FAILURE;
    else
        return ERR_SUCCESS;
}

int32_t system_mutex_trylock(system_mutex_t mutex, uint8_t *acquired)
{
    assert(0);
    return ERR_NOT_IMPLEMENTED;
}

int32_t system_mutex_unlock(system_mutex_t mutex)
{
    int error;
    LWSEM_STRUCT_PTR sem = mutex;

    error = _lwsem_post(sem);
    if (error != MQX_OK)
        return ERR_FAILURE;
    else
        return ERR_SUCCESS;
}

