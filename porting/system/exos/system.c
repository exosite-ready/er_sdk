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
 * @brief Implementation of common System functions for EXOS
 **/
#include <stdio.h>
#include <assert.h>

#include <lib/type.h>
#include <lib/debug.h>
#include <lib/error.h>
#include <lib/pclb.h>
#include <lib/utils.h>
#include <lib/mem_check.h>

#include <porting/system_port.h>
#include "platform_exos.h"

#if defined(__APPLE__) || defined(__linux__)
#include <time.h>
#endif

static int32_t cs_int_status;
static uint8_t is_critical_section = FALSE;

inline const char *system_get_sn(void)
{
    return platform_get_sn();
}

int32_t system_get_cik(char *cik, size_t len)
{
    return platform_get_cik(cik, len);
}

int32_t system_set_cik(const char *cik)
{
    return platform_set_cik(cik);
}

void system_sleep_ms(timer_data_type delay_ms)
{
#if defined(__APPLE__) || defined(__linux__)
    struct timespec t;
    long long nanosec = delay_ms * 1000000;

    t.tv_sec = nanosec / 1000000000;
    t.tv_nsec = nanosec % 1000000000;

    nanosleep(&t, NULL);
#else
    sys_time_t start = platform_get_time_ms();
    sys_time_t end = start;

    while (start + delay_ms > end)
        end = platform_get_time_ms();
#endif
}

sys_time_t system_get_time(void)
{
    return platform_get_time_ms();
}

void system_reset(void)
{
    platform_reset();
}

/*The stack canary will be filled with this pattern*/
#define STACK_FILL_PATTERN (0xCDEDABCB)
/*This big chunk of the stack will be used to check overflow*/
#define STACK_CANARY_SIZE (384)

#ifndef DEBUG_MEM_MEASUREMENTS
static bool is_stack_limit_exceeded(void)
{
    uint32_t i;
    uint32_t *stack_limit = platform_get_stack_location();

    if (stack_limit == NULL)
        return FALSE;

    for (i = 0; i < STACK_CANARY_SIZE; i++) {
        if (stack_limit[i] != STACK_FILL_PATTERN) {
            debug_log(DEBUG_PLATFORM, ("Stack limit exceeded\n"));
            return TRUE;
        }
    }

    return FALSE;
}

static void init_stack_limit(void)
{
    uint32_t *stack_limit = platform_get_stack_location();
    uint32_t i;

    if (stack_limit == NULL)
        return;

    /*
     * The newlib module might not be initalized at this point, which can freeze the systme
     * debug_log(DEBUG_PLATFORM, ("Stack end %p; canary size: %d\n", stack_limit, STACK_CANARY_SIZE));
     */
    for (i = 0; i < STACK_CANARY_SIZE; i++)
        stack_limit[i] = STACK_FILL_PATTERN;
}

#else
static size_t peak_stack_usage; /*0*/
static bool is_stack_limit_exceeded(void)
{
    uint32_t i;
    uint32_t *stack_limit = platform_get_stack_location();
    uint32_t *stack_base = platform_get_stack_base();
    uint8_t *limit = (uint8_t *)stack_limit;
    size_t stack_size = (int32_t)stack_base - (int32_t)stack_limit;

    if (stack_limit == NULL)
    return FALSE;

    for (i = 0; i < stack_size; i++) {
        if (limit[i] != 0xcd) {
            peak_stack_usage = stack_size - i;
            break;
        }
    }

    return FALSE;
}

static void init_stack_limit(void)
{
    uint32_t *stack_limit = platform_get_stack_location();
    uint32_t *stack_base = platform_get_stack_base();
    uint8_t *limit = (uint8_t *)stack_limit;
    /*Don't fill from the start we would corrupt the stack then*/
    size_t stack_size = (int32_t)stack_base - (int32_t)stack_limit - 8000;
    uint32_t i;

    printf("Stack size: %x %x %d %x\n", limit, stack_base, stack_size, &i);
    if (stack_limit == NULL)
        return;

    for (i = 0; i < stack_size; i++)
        limit[i] = 0xCD;
}
#endif

size_t get_peak_stack_usage(void)
{
#if defined DEBUG_MEM_MEASUREMENTS
    return peak_stack_usage;
#else
    return 0;
#endif
}

void system_enter_critical_section(void)
{
    if (is_critical_section == FALSE)
        cs_int_status = platform_set_global_interrupts(FALSE);

    is_critical_section = TRUE;
}

void system_leave_critical_section(void)
{
    if (is_critical_section == TRUE) {
        is_critical_section = FALSE;
        platform_set_global_interrupts(cs_int_status);
    }
}

int32_t system_init(void)
{
    int32_t status;

    status = platform_exos_init();
    assert(status == ERR_SUCCESS);
    if (status != ERR_SUCCESS)
        return status;


    init_stack_limit();
    register_stack_checker(is_stack_limit_exceeded);

    return status;
}

