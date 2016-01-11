/**
 * @file rtc.c
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
 * This module implements a very simple, not very accurate Real time clock
 **/
#include <time.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <porting/system_port.h>
#include <rtc_if.h>

/*
 * This function returns the linux time of the system
 */
static sys_time_t system_time_linux;
static sys_time_t previous_system_time;

void rtc_set(sys_time_t new_time)
{
    previous_system_time = system_get_time();
    system_time_linux = new_time;
}

sys_time_t rtc_get(void)
{
    return system_time_linux;
}

void rtc_periodic(void *data)
{
    sys_time_t system_time;

    (void)data;
    system_time = system_get_time();

    system_time_linux += (system_time - previous_system_time) / 1000;

    /*Take into account the residual time after the division*/
    previous_system_time = system_time - (system_time - previous_system_time) % 1000;
}

extern time_t XTIME(time_t *timer);
time_t XTIME(time_t *timer)
{
    time_t sec = 0;

    sec = (time_t) system_time_linux;
    if (timer != NULL)
        *timer = sec;

    return sec;

}

