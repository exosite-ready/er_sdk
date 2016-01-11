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
 * @brief System_mutex implementation for posix
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <porting/thread_port.h>
#include <lib/mem_check.h>
#include <lib/sf_malloc.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

int32_t system_mutex_create(system_mutex_t *mutex)
{
    pthread_mutexattr_t Attr;
    pthread_mutex_t *new_mutex = sf_malloc(sizeof(pthread_mutex_t));
    *mutex = NULL;

    assert(new_mutex);

    if (new_mutex == NULL)
        return ERR_NO_MEMORY;

    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);

    if (pthread_mutex_init(new_mutex, &Attr))
        return ERR_FAILURE;

    *mutex = new_mutex;

    check_memory();

    return ERR_SUCCESS;
}

int32_t system_mutex_destroy(system_mutex_t mutex)
{
    assert(mutex);

    pthread_mutex_destroy((pthread_mutex_t *) mutex);
    sf_free(mutex);

    check_memory();

    return ERR_SUCCESS;
}

int32_t system_mutex_lock(system_mutex_t mutex)
{
    if (pthread_mutex_lock((pthread_mutex_t *) mutex)) {
        assert(0);
        return ERR_FAILURE;
    }

    return ERR_SUCCESS;
}

int32_t system_mutex_trylock(system_mutex_t mutex, uint8_t *acquired)
{
    int32_t status = pthread_mutex_trylock((pthread_mutex_t *) mutex);

    switch (status) {

    /* Success */
    case 0:
        *acquired = TRUE;
        break;

        /* The mutex is locked */
    case EBUSY:
        *acquired = FALSE;
        break;

        /* Error */
    default:
        *acquired = FALSE;
        assert(0);
        return ERR_FAILURE;
    }

    return ERR_SUCCESS;
}

int32_t system_mutex_unlock(system_mutex_t mutex)
{
    if (pthread_mutex_unlock((pthread_mutex_t *) mutex)) {
        assert(0);
        return ERR_FAILURE;
    }

    return ERR_SUCCESS;
}

