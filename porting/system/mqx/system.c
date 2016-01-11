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
 * @brief Implementation of common System functions for MQX
 **/
/* READ THIS !!!!!!!!
 *
 * DON'T USE STDIO ON FREESCALE UNTIL THE NEWLIB STUBS are FIXED, otherwise PRINTF WON'T WORK
 *
 */
#include <stdint.h>
#include <mqx.h>
#include <fio.h>
#include <assert.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/pclb.h>
#include <porting/thread_port.h>
#include <lib/mem_check.h>
#include <porting/system_port.h>

#include "platform_qmx.h"

static struct link_layer_if *ll;

/*TODO IMPLEMENT newlib stubs*/
_ssize_t _read_r(struct _reent *r, int file, void *ptr, size_t len)
{
}
_ssize_t _write_r(struct _reent *r, int file, const void *ptr, size_t len)
{
}
off_t _lseek_r(struct _reent *r, int file, off_t off, int whence)
{
}
struct stat;
int _fstat_r(struct _reent *r, int file, struct stat *st)
{
}
int _close_r(struct _reent *r, int file)
{
}
int _open_r(struct _reent *r, const char *name, int flags, int mode)
{
}
int _isatty_r(struct _reent *r, int fd)
{
}
int _kill(int pid, int sig)
{
}
pid_t _getpid(void)
{
}

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
    _time_delay(delay_ms);
}

sys_time_t system_get_time(void)
{
    TIME_STRUCT time;

    _time_get_elapsed(&time);
    return time.SECONDS * 1000 + time.MILLISECONDS;
}

void system_reset(void)
{
    platform_reset();
}

void system_enter_critical_section(void)
{
}

void system_leave_critical_section(void)
{
}

int32_t system_init(void)
{
    return ERR_SUCCESS;
}

