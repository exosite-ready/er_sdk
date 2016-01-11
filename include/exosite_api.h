/**
 * @file exosite_api.h
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
 * @brief This file contains the Non-blocking User APIs
 *
 * @defgroup Exosite_API Exosite API
 * This interface provides access to the Exosite
 * SDK services for the user application.
 *
 * @image html ER_SDK_user_if.jpg
 *
 * It is a non-blocking API. The function calls are not blocking
 * the caller thread. When the operation is finished, a callback
 * function is called.
 * @{
 */

#ifndef INC_EXOSITE_API_H_
#define INC_EXOSITE_API_H_

#include <stdint.h>
#include <protocol_client_common.h>

/**
 */

struct exosite_class;

enum exosite_device_status {
    DEVICE_STATUS_UNKNOWN,
    DEVICE_STATUS_NOT_ACTIVATED,
    DEVICE_STATUS_ACTIVATED,
    DEVICE_STATUS_EXPIRED,
    DEVICE_STATUS_REVOKED,
};

/**
 * The SDK will use a function registered with this prototype when printing logs
 **/
typedef int (*log_function)(const char * format, ... );

/**
 * FOR FUTURE USE
 **/
typedef int (*debug_callback)(uint32_t level, uint32_t module, int32_t error, char* message);

/**
 * @brief Initalizes the exosite SDK.
 *
 * @warning The API is only usable after this was called
 *
 * @param[in] log_function    Sets the log function for the SDK, this function is used during logging
 * @param[in] debug callback  -- For FUTURE USE
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t exosite_sdk_init(log_function log_fn, debug_callback debug_cb);


/**
 * @brief Creates an exosite object
 *
 * This object represent an Exosite client device and will be used for all subsequent
 * communication with the Exosite server
 *
 * @note It starts provisioning asinchronously and reprovision if CIK is revoked
 *
 * @param[out] exo     - The new exosite object created by this call
 * @param[in] vendor   - The vendor of the device this software is running on
 * @param[in] model    - The model of the device this software is running on
 * @param[in] optional_sn - optionally the the serial number can be provided
 *                          If it's not provided the MAC address will be used as SN
 * @param[in] app_type - The protocol type (HTTP or COAP)
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t exosite_new(struct exosite_class **exo,
                    const char *vendor,
                    const char *model,
                    const char *optional_sn,
                    enum application_protocol_type app_type);

/**
 * Callback that will be called if a response is received for a read request or when it times out
 **/
typedef void (*exosite_on_read)(int32_t status, const char *alias, const char *value);

/**
 * @brief Sends a read request to the Exosite server
 *
 * If a response is received or
 * if there is no response until the pre-specified timeout
 * the on_read callback will be called
 *
 * @param[in] exo      - The exosite object
 * @param[in] alias    - The alias of the Data resource that will be read
 * @param[in] on_read  - The callback to be called when a response is received or at timeout
 *
 * @return ERR_SUCCESS if the request has been successfully created ERR_* error code otherwise
 **/
int32_t exosite_read(struct exosite_class *exo, const char *alias, exosite_on_read on_read);

/**
 * Callback that will be called if an acknowledge is received for a
 * write request or when it times out
 **/
typedef void (*exosite_on_write)(int32_t status, const char *alias);

/**
 * @brief Writes data to the Exosite server
 *
 * If the value is successfully sent or there is no acknowledge until the pre-specified timeout the on_write
 * callback will be called
 *
 * @param[in] exo       - The exosite object
 * @param[in] alias     - The alias of the Data resource that will be written
 * @param[in] value     - The value to be written
 * @param[in] on_write  - The callback to be called when an acknowledge is received or at timeout*
 *
 * @return ERR_SUCCESS if the write request has been successfully created ERR_* error code otherwise
 **/
int32_t exosite_write(struct exosite_class *exo,
                      const char *alias,
                      const char *value,
                      exosite_on_write on_write);

/**
 * Callback that will be called if there is change on the Data resource
 * specified
 **/
typedef void (*exosite_on_change)(int32_t status, const char *alias, const char *value);

/**
 * @brief Subscribe to a data source
 *
 * If the data source is changed, the on_change callback function
 * will be called.
 *
 * @param[in] exo        - The exosite object
 * @param[in] alias      - The alias of the Data resource that is subscribed to
 * @param[in] since      - Optional: if specified then changes are watched since
 *                         this time (in linux time)
 * @param[in] on_change  - The callback to be called when the Data resource
 *                         changed or at timeout
 *
 * @return ERR_SUCCESS if the write request has been successfully created or
 *         ERR_* error code otherwise
 **/
int32_t exosite_subscribe(struct exosite_class *exo,
                          const char *alias,
                          int32_t since,
                          exosite_on_change on_change);

/**
 * @brief Returns the status of the exosite object specified
 *
 * @param[in] exo   - The exosite object
 *
 * @return Status of the exosite object specified
 **/
enum exosite_device_status exosite_get_status(struct exosite_class *exo);

/**
 * @brief Wait and call the internal state machine periodically
 *
 * This function does not return until the specified time period
 * is expired. While it is waiting, the exosite_poll function is
 * called periodically.
 *
 * @param[in] exo   - The exosite object
 * @param[in] ms    - Length of delay
 *
 * @return none
 **/
void exosite_delay_and_poll(struct exosite_class *exo, sys_time_t ms);

/**
 * @brief Call the internal state machine
 *
 * Calls the internal state machine and emulates non-preemptive
 * multithreading. This function shall be called on single-threaded
 * systems. On multithreaded systems it does nothing.
 *
 * @param[in] exo   - The exosite object
 *
 * @return none
 **/
void exosite_poll(struct exosite_class *exo);

/**
 * @brief Register a periodic function
 *
 * @param[in] platform_periodic_fn   - The function to be called periodically
 * @param[in] period                 - The time period
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t exosite_sdk_register_periodic_fn(void (*platform_periodic_fn)(void *), timer_data_type period);

/**
 * @brief Register a memory buffer to the SDK
 *        The SDK will only use this memory for allocating objects
 *
 * This has to be called before the exosite_sdk_init call
 *
 * @param[in] buf   - The pointer to the buffer
 * @param[in] size  - The size of the buffer
 *
 **/
void exosite_sdk_register_memory_buffer(void *buf, size_t size);

/**
 * Allocator type definitions
 **/
typedef void *(*exosite_malloc_func)(size_t);
typedef void  (*exosite_free_func)(void *);
typedef void *(*exosite_realloc_func)(void *, size_t);

/**
 * @brief Register memory allocators to the SDK
 *
 * This has to be called before the exosite_sdk_init call
 *
 * The SDK will use these allocators instead of the default malloc, free, etc
 *
 * Note! The SDK will only use these allocators if a memory buffer was not registered
 * If a memory buffer is registered then the SDK will use its internal allocator to allocate memory
 * from this buffer
 *
 * @param[in] malloc_func   - The malloc function to be registered
 * @param[in] free_func     - The free function to be registered
 * @param[in] realloc_func  - The realloc function to be registered
 *
 **/
void exosite_sdk_register_memory_allocators(exosite_malloc_func, exosite_free_func, exosite_realloc_func);

/** @} end of Exosite_API group */

#endif /* INC_EXOSITE_API_H_ */
