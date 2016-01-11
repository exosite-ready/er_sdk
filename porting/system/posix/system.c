/**
 * @file system.c
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
 * @brief Implementation of common System functions for POSIX
 **/
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <lib/type.h>
#include <lib/pclb.h>
#include <lib/mem_check.h>
#include <lib/error.h>
#include "platform_posix.h"
#include <porting/system_port.h>
#include <system_utils.h>

static pthread_mutex_t cs_lock;
static volatile uint8_t cs_initialized; /*NULL*/

static char platform_sn_num[13] = "000000";

const char *system_get_sn(void)
{
    /* TODO */
    return platform_sn_num;
}

int32_t system_get_cik(char *cik, size_t len)
{
    FILE *pFile;
    char *dummy;  /* For Linux build */

    if (cik == NULL || len < CIK_LENGTH+1)
        return ERR_FAILURE;

    pFile = fopen("cik.store", "r");
    if (pFile != NULL) {
        dummy = fgets(cik, CIK_LENGTH+1, pFile);
        (void)dummy;

        fclose(pFile);
        cik[40] = '\0';
        return ERR_SUCCESS;
    }
    return ERR_FAILURE;
}

int32_t system_set_cik(const char *cik)
{
    FILE *pFile;

    pFile = fopen("cik.store", "w");
    if (pFile != NULL) {
        fputs(cik, pFile);
        fclose(pFile);
        return ERR_SUCCESS;
    }
    return ERR_FAILURE;
}

void system_sleep_ms(timer_data_type delay_ms)
{
    struct timespec t;
    unsigned long long nanosec = delay_ms * 1000000;

    t.tv_sec = (unsigned int)nanosec / 1000000000;
    t.tv_nsec = nanosec % 1000000000;

    nanosleep(&t, NULL);
}

sys_time_t system_get_time(void)
{
    struct timeval start;

    gettimeofday(&start, 0);
    return (sys_time_t)(start.tv_sec * 1000 + start.tv_usec / 1000);
}

void system_reset(void)
{
}

void system_enter_critical_section(void)
{
    assert(cs_initialized);

    if (pthread_mutex_lock(&cs_lock))
        assert(0);
}

void system_leave_critical_section(void)
{
    assert(cs_initialized);

    if (pthread_mutex_unlock(&cs_lock))
        assert(0);
}

int32_t system_init(void)
{
    pthread_mutexattr_t cs_attr;

    pthread_mutexattr_init(&cs_attr);
    pthread_mutexattr_settype(&cs_attr, PTHREAD_MUTEX_RECURSIVE);

    if (pthread_mutex_init(&cs_lock, &cs_attr))
        return ERR_FAILURE;

    cs_initialized = TRUE;

    check_memory();

    return ERR_SUCCESS;
}

