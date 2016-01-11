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
 * @brief Implementation of common System functions for FreeRTOS
 **/
#include <stdio.h>
#include <lib/type.h>
#include <lib/mem_check.h>
#include <assert.h>
#include <lib/error.h>
#include <porting/system_port.h>
#include <FreeRTOS.h>
#include <task.h>
#include <FreeRTOSConfig.h>
#include <portmacro.h>
#include <lib/pclb.h>
#include "platform_freertos.h"

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

inline void system_sleep_ms(timer_data_type delay_ms)
{
    uint32_t remainder;

    if ((delay_ms / (1000 / configTICK_RATE_HZ)) != 0)
        vTaskDelay((portTickType) (delay_ms / (1000 / configTICK_RATE_HZ)));

    remainder = delay_ms % (1000 / configTICK_RATE_HZ);

    if (remainder != 0) {
        volatile uint32_t clock_in_kilohertz = (uint32_t)(configCPU_CLOCK_HZ / 1000);

        for (; clock_in_kilohertz != 0; clock_in_kilohertz--) {
            volatile uint32_t tmp_ms = remainder;

            for (; tmp_ms != 0; tmp_ms--)
                ; /* do nothing */
        }
    }
}

inline sys_time_t system_get_time(void)
{
    return (xTaskGetTickCount() * (1000 / configTICK_RATE_HZ));
}

inline void system_reset(void)
{
    platform_reset();
}

void system_enter_critical_section(void)
{
    taskENTER_CRITICAL()
        ;
}

inline void system_leave_critical_section(void)
{
    taskEXIT_CRITICAL()
         ;
}

int32_t system_init(void)
{
    return ERR_SUCCESS;
}

