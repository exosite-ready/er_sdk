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
 * @brief System mutex implementation for exos
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <porting/thread_port.h>
#include <assert.h>
#include <lib/sf_malloc.h>
#include <porting/system_port.h>

int32_t system_mutex_create(system_mutex_t *mutex)
{
    uint8_t *new_mutex = sf_malloc(sizeof(uint8_t));

    if (new_mutex == NULL)
        return ERR_NO_MEMORY;

    *new_mutex = 1;
    *mutex = new_mutex;

    return ERR_SUCCESS;
}

int32_t system_mutex_destroy(system_mutex_t mutex)
{
    sf_free(mutex);
    return ERR_SUCCESS;
}

int32_t system_mutex_lock(system_mutex_t mutex)
{
    volatile uint8_t *mutex_value = (uint8_t *) mutex;

    while (1) {
        system_enter_critical_section();
        if (*mutex_value) {
            *mutex_value = 0;
            system_leave_critical_section();
            break;
        }
        system_leave_critical_section();
        asm ("nop");
    }

    return ERR_SUCCESS;
}

int32_t system_mutex_trylock(system_mutex_t mutex, uint8_t *acquired)
{
    volatile uint8_t *mutex_value = (uint8_t *) mutex;

    system_enter_critical_section();

    if (*mutex_value) {
        *mutex_value = 0;
        *acquired = TRUE;
    } else
        *acquired = FALSE;

    system_leave_critical_section();

    return ERR_SUCCESS;
}

int32_t system_mutex_unlock(system_mutex_t mutex)
{
    volatile uint8_t *mutex_value = (uint8_t *) mutex;

    system_enter_critical_section();
    *mutex_value = 1;
    system_leave_critical_section();

    return ERR_SUCCESS;
}

