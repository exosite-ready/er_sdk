/**
 * @file debug.c
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
 * The debug module
 **/
#include <stdio.h>
#include <stdlib.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <lib/mem_check.h>

#if defined __APPLE__
#include "malloc/malloc.h"
#elif defined __linux__
#include "malloc.h"
#endif

static uint32_t system_debug_level = DEBUG_DEBUG;
static uint32_t enabled_modules = 0xffffffff;
deb_log_function g_log_fn;
deb_debug_callback g_debug_callback;

void debug_init(deb_log_function log_fn, deb_debug_callback debug_callback)
{
    g_log_fn = log_fn;
    g_debug_callback = debug_callback;
}

void assert_impl(const char *strFile, unsigned uLine)
{
    error_log(DEBUG_MEM, ("Assertion failed %s, line %u\n", strFile, uLine), -1);
    error_log(DEBUG_MEM, ("----------------------------\n"), -1);
    abort();
}

uint32_t is_module_enabled(uint32_t module)
{
    return module & enabled_modules;
}

uint32_t is_level_printable(uint32_t level)
{
    return level <= system_debug_level;
}

void print_array(char *buf, int32_t length)
{
    int32_t i;

#ifdef EASDK_DEBUG_LEVEL_DEBUG
    unsigned char *buffer = (unsigned char *)buf;
#else
    (void)buf;
#endif

    for (i = 0; i < length; i++) {
        if (i % 16 == 0) {
            debug_log(DEBUG_NET, ("\r\n0x%05x: ", i));
        }

        debug_log(DEBUG_NET, ("0x%03x ", *buffer++));
    } debug_log(DEBUG_NET, ("\r\n"));
}

void print_array_ch(char *buf, int32_t length)
{
    int32_t i;

#ifdef EASDK_DEBUG_LEVEL_DEBUG
    unsigned char *buffer = (unsigned char *)buf;
#else
    (void)buf;
#endif


    for (i = 0; i < length; i++) {
        if (i % 16 == 0) {
            debug_log(DEBUG_NET, ("\r\n0x%05x: ", i));
        }

        debug_log(DEBUG_NET, ("%c", *buffer++));
    } debug_log(DEBUG_NET, ("\r\n"));
}

#if defined DEBUG_EASDK && defined EASDK_DEBUG_LEVEL_DEBUG
static void print_peak_memory_usage(void)
{
#ifdef DEBUG_MEM_MEASUREMENTS
    debug_log(DEBUG_MEM, ("Peak stack usage: %d\n", get_peak_stack_usage()));
    debug_log(DEBUG_MEM, ("Peak heap  usage: %d\n", get_peak_heap_usage()));
#endif
}
#endif

void print_used_memory(void)
{
#if defined DEBUG_EASDK && defined EASDK_DEBUG_LEVEL_DEBUG
#if defined __APPLE__
    struct mstats stat;

    stat = mstats();
    debug_log(DEBUG_MEM, ("Memstat: %d\n", stat.bytes_used));
#elif defined __linux__
    struct mallinfo mi;

    mi = mallinfo();
    debug_log(DEBUG_MEM, ("Memstat: %d(/%d)\n", mi.uordblks, mi.fordblks));
#endif
    print_peak_memory_usage();
    check_memory();
    print_memory_stats();
#endif
}

