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
 * @brief System_mutex implementation for FreeRTOS
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <sdk_config.h>
#include <porting/thread_port.h>
#include <lib/sf_malloc.h>
#include <errno.h>
#include <assert.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <lib/mem_check.h>

int32_t system_mutex_create(system_mutex_t *mutex)
{
#ifdef FREERTOS_LEGACY
    xSemaphoreHandle xMutex;
#else
    SemaphoreHandle_t xMutex;
#endif

    *mutex = NULL;

    xMutex = xSemaphoreCreateRecursiveMutex();
    if (xMutex == NULL)
        return ERR_FAILURE;

    *mutex = xMutex;

    check_memory();

    return ERR_SUCCESS;
}

int32_t system_mutex_destroy(system_mutex_t mutex)
{
    assert(mutex);

    vSemaphoreDelete(mutex);

    check_memory();

    return ERR_SUCCESS;
}

int32_t system_mutex_lock(system_mutex_t mutex)
{
    assert(mutex);

    while (xSemaphoreTakeRecursive(mutex, 0xffffffff) == pdFALSE)
        ;

    return ERR_SUCCESS;
}

int32_t system_mutex_trylock(system_mutex_t mutex, uint8_t *acquired)
{
    assert(mutex);

    if (xSemaphoreTakeRecursive(mutex, 0) == pdTRUE)
        *acquired = TRUE;
    else
        *acquired = FALSE;

    return ERR_SUCCESS;
}

int32_t system_mutex_unlock(system_mutex_t mutex)
{
    assert(mutex);
    xSemaphoreGiveRecursive(mutex);
    return ERR_SUCCESS;
}

