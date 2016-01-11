/**
 * @file Operating system adapter layer
 *
 * All these functions should be implemented on every OS it is ported to
 */

#ifndef SYSTEM_PORT_H
#define SYSTEM_PORT_H

#include <lib/type.h>
#include <porting/net_port.h>

/** @defgroup system_port_interface System Port Interface
 *  This interface has to be implemented by the user code
 *  to provide the necessary system services for the SDK.
 *
 *  @image html ER_SDK_system_if.jpg
 *
 *  @{
 */

/**
 * Initalizes the system layer, this layer is OS specific
 * This should contain the device / platform initalization for a specific OS
 *
 * @return Status code
 **/
int32_t system_init(void);

/**
 * Sleeps the specified number of miliseconds.
 * This should yield if the system that implements this has threading support
 * If there is no underlying OS then it will block
 *
 * @param[in]  ms Sleep for this amount of time
 *
 * @return none
 **/
void system_sleep_ms(timer_data_type ms);

/**
 * Returns the system time in [ms]
 *
 * @return The system time
 **/
sys_time_t system_get_time(void);

/**
 * Resets the device
 *
 * @return none
 **/
void system_reset(void);

/**
 * Enter critical section
 *
 * Piece of code, that is framed by system_enter_critical_section()
 * and system_leave_critical_section() functions, can not be
 * accessed by more than one thread execution.
 *
 * @return none
 **/
void system_enter_critical_section(void);

/**
 * Leave critical section
 *
 * Piece of code, that is framed by system_enter_critical_section()
 * and system_leave_critical_section() functions, can not be
 * accessed by more than one thread execution.
 *
 * @return none
 **/
void system_leave_critical_section(void);

/**
 * Get serial number
 *
 * @return Serial number as a string
 **/
const char* system_get_sn(void);

/**
 * Get the CIK stored in non-volatile storage of the current board
 *
 * @param[out] cik   - The CIK stored in non-volatile storage
 * @param[in]  len   - The length of the output buffer for CIK
 *
 * @return ERR_SUCESS on success, ERR_FAILURE if failed
 **/
int32_t system_get_cik(char *cik, size_t len);

/**
 * Get the CIK stored in non-volatile storage of the current board
 *
 * @param[in] cik   - The cik in C-string format
 *
 * @return ERR_SUCESS on success, ERR_FAILURE if failed to set
 **/
int32_t system_set_cik(const char *cik);

/** Abstract serial device class */
struct sdev {

    uint32_t base_address;

    /** Serial read */
    int32_t (*read)(struct sdev *self, char *data);

    /** Serial write */
    void (*write)(struct sdev *self, char data);

    /** If more is to be registered, they can be linked with this pointer*/
    struct sdev *next;
};

/**
 * This function has to be called if a debug device is to be used
 * The SDK will call this function
 *
 * @note OPTIONAL
 * Implementing this function is optional depending on how you
 * port the SDK
 *
 * @param[out]  debug   - The serial device object to be used by the SDK
 *
 * @return none
 */
void system_debug_device_init(struct sdev **debug);

/** @} end of system_port_interface group */

#endif /* SYSTEM_PORT_H */
