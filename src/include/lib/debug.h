/**
 * @file debug.h
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
 * @brief Defines the debug_log functions; debug levels and some other utility function
 *        interface used by the debug module
 *
 **/
#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#include <string.h>
#include <lib/type.h>
#include <lib/utils.h>
#include <lib/common_defines.h>
#include <porting/config_port.h>
#include <rtc_if.h>


/**
 * @def Defines the DEBUG_VERBOSITY level
 *
 * Currently it is a swich;
 *   - If it is defined the verbosity is on highest level
 *   - Otherwise it is on lowest level
 *
 *  This can be used to reduce the amount of information printed by the logger functions
 *  On systems with small amount of non-volatile memory(flash) it could be crucial not to store kBytes of
 *  strings in the non-volatile memory
 *
 *  If the verbosity is low then a much smaller amount should be printed; what is printed exacly is implementation
 *  dependent;
 *    - in the current implementation:
 *      - in verbose mode: the timestamp, the filename, the function name and the message is included in
 *        the debug message
 *      - in silent mode: only the module number and the message is included
 **/
#ifndef DEBUG_VERBOSITY
#define DEBUG_VERBOSITY   1
#endif

/**
 * @def Defines the debug modules
 * Debug modules are binary encoded
 * */
#define DEBUG_NET           (1 << 0)
#define DEBUG_PLATFORM      (1 << 1)
#define DEBUG_UPDATE        (1 << 2)
#define DEBUG_HTTP          (1 << 3)
#define DEBUG_MEM           (1 << 4)
#define DEBUG_EXOAPI        (1 << 5)
#define DEBUG_PRC_SM        (1 << 6)
#define DEBUG_APPLICATION   (1 << 7)
#define DEBUG_COMMON        (1 << 8)
#define DEBUG_SSL           (1 << 9)
#define DEBUG_CONFIGURATOR  (1 << 10)

/* We don't want to include exosite_api.h so define them here locally*/
/**
 * @brief A logger function that will be called by the debug module when printing a message
 **/
typedef int (*deb_log_function)(const char * format, ... );

/**
 * @brief A callback function which will be called instead of printing the message; currently this
 *        feature is not available
 **/
typedef int (*deb_debug_callback)(uint32_t level, uint32_t module, int32_t error, char* message);

/**
 * @brief With this function a logger function and a debug_callback can be provided to the debug module
 *
 *        The debug module requires a logging function to print out messages
 *        If it is not provided it will not print out anything
 *
 * @param[in] log_fn          The logger function; This will be called when any message is printed
 * @param[in] debug_callback  Currently unused
 **/
void debug_init(deb_log_function log_fn, deb_debug_callback debug_callback);
/**
 * Returns whether a module is enabled or nut
 *
 * @param[in] module The module to be queried
 * @return TRUE if enabled FALSE otherwise
 */
uint32_t is_module_enabled(uint32_t module);

/**
 * Returns whether the specified level should be printed
 * A level should be printed if it is lower or equal
 * then the system debug level
 *
 * @param[in] module The level to be checked
 *
 * @return TRUE if printable FALSE otherwise
 */
uint32_t is_level_printable(uint32_t level);

/**
 * Print buffer in
 * <address: data>
 * format
 * e.g
 * 0x0: 0x5 0x5 0x6 0x6
 * One line contains 8 bytes
 *
 * @param[in] buf     the buffer to be printed
 * @param[in] length  the length of the buffer
 */
void print_array(char *buf, int32_t length);

/**
 * Print buffer in
 * <address: data>
 * format. The bytes are displayed as characters
 * e.g
 * 0x0: c a a h
 * One line contains 8 bytes
 *
 * @param[in] buf     the buffer to be printed
 * @param[in] length  the length of the buffer
 */
void print_array_ch(char *buf, int32_t length);

/**
 * @def
 *
 * On PC systems there should be a flush after printf,
 * otherwise messages are only printed after the program has
 * been finished or when the buffer limit is reached
 *
 * This might not be the expected behaviour so if DEBUG_MEM_MEASUREMENTS is turned on
 * then the logger will use a flush command as well
 */
#if defined DEBUG_MEM_MEASUREMENTS && (defined (__APPLE__) || defined (__linux__))
#define flush_command fflush(stdout)
#else
#define flush_command
#endif

/*
 * These are used by the debug macros
 * */
extern deb_log_function g_log_fn;
extern deb_debug_callback g_debug_callback;

/**
 * @def Defines The filename macro; this macro puts the filename
 *      into the debug message
 **/
#define __FILENAME__ debug_utils_get_filename(__FILE__)
/**
 * @def debug_log definition
 *
 * @param[in] level    The debug level
 * @param[in] module   The module
 * @param[in] message  The message in ("formatted message", ...)
 *                     format
 *
 * A debug log is only printed if
 *   - There is a logger function registered
 *   - the module is enabled
 *   - the level is lower or equal then the system debug level
 *
 */
#ifdef DEBUG_VERBOSITY
#define sdk_log(level, module, message) \
do { \
  if (is_level_printable(level) && is_module_enabled(module) && g_log_fn) \
  {\
    g_log_fn("%d: %-25s (%-30s): ", rtc_get(), __FILENAME__, __FUNCTION__);\
    g_log_fn message ; \
    flush_command;\
  }\
} while (0)
#else
#define sdk_log(level, module, message) \
do { \
  if (is_level_printable(level) && is_module_enabled(module) && g_log_fn) \
  {\
    g_log_fn("%x: ", module);\
    g_log_fn message ; \
    flush_command;\
  }\
} while (0)
#endif

#ifdef DEBUG_VERBOSITY
#define sdk_error(level, module, message, error_code) \
do { \
    if (g_log_fn) {\
      g_log_fn("%d: %-25s (%-30s): Error [%d]: ", rtc_get(), __FILENAME__, __FUNCTION__, error_code);\
      g_log_fn message ; \
      flush_command;\
    }\
} \
  while (0)
#else
#define sdk_error(level, module, message, error_code) \
do { \
    if (g_log_fn) {\
      g_log_fn("%x E:%d\n", module, error_code);\
      flush_command;\
    }\
} \
  while (0)
#endif

#if COMPILE_TIME_DEBUG_LEVEL >= DEBUG_ERROR
    #define error_log(module, message, error_code) sdk_error(DEBUG_ERROR, module, message, error_code)
    #define EASDK_DEBUG_LEVEL_ERROR
#endif
#if COMPILE_TIME_DEBUG_LEVEL >= DEBUG_WARNING
    #define warning_log(module, message, error_code) sdk_error(DEBUG_WARNING, module, message, error_code)
    #define EASDK_DEBUG_LEVEL_WARNING
#endif
#if COMPILE_TIME_DEBUG_LEVEL >= DEBUG_INFO
    #define info_log(module, message) sdk_log(DEBUG_INFO, module, message)
    #define EASDK_DEBUG_LEVEL_INFO
#endif
#if COMPILE_TIME_DEBUG_LEVEL >= DEBUG_DEBUG
    #define debug_log(module, message) sdk_log(DEBUG_DEBUG, module, message)
    #define EASDK_DEBUG_LEVEL_DEBUG
#endif

#ifndef error_log
#define error_log(module, message, error_code)
#endif
#ifndef warning_log
#define warning_log(module, message, error_code)
#endif
#ifndef info_log
#define info_log(module, message)
#endif
#ifndef debug_log
#define debug_log(module, message)
#endif
#ifndef trace_log
#define trace_log(module, message)
#endif

#ifdef DEBUG_EASDK
/**
 * Low level debug message interface
 *
 * This is to be used when printf (newlib, etc) subsystems are
 * not yet available
 * The implementation usually simply puts characters to a serial
 * peripheral (e.g uart)
 *
 * @param[in] data     Null terminated message (c-string)
 *
 */
void print_debug_message(char *data);

/**
 * Low level debug message interface, with one argument
 * The debug message is not a format string, just a plain message
 * That's why the argument might be needed
 *
 * This is to be used when printf (newlib, etc) subsystems are
 * not yet available
 * The implementation usually simply puts characters to a serial
 * peripheral (e.g uart)
 *
 * @param[in] data     Null terminated message (c-string)
 * @param[in] arg      An argument to be printed as uint32_t
 *
 */
void print_debug_message_arg(char *data, uint32_t arg);
#else
static inline void print_debug_message(char *data)
{
    (void)data;
}
static inline void print_debug_message_arg(char *data, uint32_t arg)
{
    (void)data;
    (void)arg;
}
#endif

/**
 * _Assert interface
 *
 * @param[in] A message
 * @param[in] An argument
 */
void assert_impl(const char *, unsigned);
#ifdef DEBUG_EASDK
/**
 * @def Assert interface
 *
 * @param in a bool true or false
 * If false the _Assert function is called
 */
#define ASSERT(f)\
  if (f) \
      {}\
  else\
      assert_impl(__FILE__, __LINE__)
#else
#define ASSERT(f)
#endif

/**
 * Prints the memory statistics:
 *  heap consumed
 *  heap usage calculated by sf_memory subsystem
 *  peak heap usage
 *  peak stack usage
 * */
void print_used_memory(void);

/**
 * Prints heap usage calculated by the sf_memory subsystem
 * */
void print_memory_stats(void);

/**
 * Gets the peak stack usage in bytes
 * */
size_t get_peak_stack_usage(void);

/**
 * Gets the peak heap usage in bytes
 * */
size_t get_peak_heap_usage(void);
#endif /* INC_DEBUG_H_ */
