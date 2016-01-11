/**
 * @file system_utils.h The SDK system interface
 *
 * These system functions can be used by the SDK (including the ones that are
 * implemented during porting (system_port.h)
 * The SDK modules should only include this header to access the system interface
 */


#ifndef INC_SYSTEM_UTILS_H_
#define INC_SYSTEM_UTILS_H_

#include <sdk_config.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <porting/system_port.h>
#include <porting/net_port.h>

struct link_layer_if;

#define CIK_LENGTH       40

/**
 * Returns the elapsed time from start, in [ms]
 *
 * @param[in] start  The difference is calculated to this point
 *
 * @return The elapsed ms
 * SDK
 *  */
sys_time_t system_get_diff_time(sys_time_t start);

/**
 * Calls the internal statemachine and emulates non-preemptive multithreading.
 * This function shall be called on single-threaded systems. On multithreaded systems it does nothing
 *
 * param[in] ms  - Ellapsed time since the last call. If the caller does not know it exactly,
 *                 this value has to be zero.
 **/
void system_poll(sys_time_t ms);

/**
 *
 * On systems with non-preemptive multithreading this function has to be called
 * if the system would block
 *
 * This has to perform the sleep and all kind of activities
 * which could otherwise would be performed in an idle loop
 *
 * @param [in] ms  - Sleep period
 *
 **/
void system_delay_and_poll(uint32_t ms);

/**
 * Verify that the CIK format is valid.
 *
 * @return TRUE on success, FALSE if failed
 **/
BOOL system_is_cik_format_valid(const char* cik);

#ifdef PRINT_EXOSITE_VERSION
void system_print_version(void);
#else
static inline void system_print_version(void) {}
#endif

#endif /* INC_SYSTEM_COMMON_H_ */
