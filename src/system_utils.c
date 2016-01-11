/**
 * @file system_utils.c
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
 * This file implements common system calls
 **/
#include <ctype.h>
#include <sdk_config.h>
#include <lib/error.h>
#include <lib/type.h>
#include <lib/pclb.h>
#include <lib/utils.h>
#include <assert.h>
#include <system_utils.h>
#ifdef PRINT_EXOSITE_VERSION
#include <version.h>
#endif
#include "../porting/system_port.h"

inline void system_poll(sys_time_t ms)
{
    if (pcld_has_thread() == FALSE)
        pclb_task(ms);
}

void system_delay_and_poll(sys_time_t ms)
{
    sys_time_t start_time;
    sys_time_t diff_time = 0;

    start_time = system_get_time();
    system_poll(0);

    if (pcld_has_thread())
        system_sleep_ms(ms);
    else {
        while ((diff_time = system_get_diff_time(start_time)) < ms) {
            system_poll(diff_time);
            system_sleep_ms(UMIN(ms, 10));
        }
    }
}

sys_time_t system_get_diff_time(sys_time_t start)
{
    sys_time_t end = system_get_time();

    return end - start;
}

BOOL system_is_cik_format_valid(const char *cik)
{
    int32_t i = 0;

    while (cik[i]) {

        if (i >= CIK_LENGTH)
            return FALSE;

        if (!isalnum(cik[i]))
            return FALSE;
        i++;
    }

    if (i != CIK_LENGTH)
        return FALSE;

    return TRUE;
}

#ifdef PRINT_EXOSITE_VERSION
void system_print_version(void)
{
    info_log(DEBUG_EXOAPI, ("Firmware hash: %s\n", hash));
    info_log(DEBUG_EXOAPI, ("Firmware version: %s\n", version));
}
#endif
