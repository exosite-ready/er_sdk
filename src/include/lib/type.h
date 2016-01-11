/**
 * @file type.h
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
 * @brief System wide type definitions
 **/
#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef HANDLE
#define HANDLE    void*
#endif

typedef unsigned int BOOL;

/* High resolution timer data type */
typedef unsigned long long timer_data_type;

/* Maximum values of the system timer, using timer_data_type */
#define PLATFORM_TIMER_SYS_MAX                ( ( 1LL << 52 ) - 2 )

/* Type for storing the system time with 1 sec resolution */
typedef uint32_t sys_time_t;

/* Macro to turn a DEFINE value to a string*/
#define STRINGIFY(x) #x
#define MAKE_STRING(x) STRINGIFY(x)

#endif /* TYPE_H */
