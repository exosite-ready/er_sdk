/**
 * @file system_thread.h
 *
 * The exosite thread API
 *
 * OS specific thread API should be wrapped to system_thread* calls
 * so that the SDK can use it
 */

#ifndef THREAD_PORT_H
#define THREAD_PORT_H

#include <lib/type.h>

/** @defgroup thread_port_interface Thread Port Interface
 *  This interface has to be implemented by the user code
 *  to provide the necessary concurrent programming tools
 *  for the SDK.
 *
 *  @image html ER_SDK_thread_if.jpg
 *
 *  @{
 */

typedef void* system_thread_t;
typedef void* system_mutex_t;
typedef void* e_semaphore_t;

/** Thread priorities */
typedef enum _system_thread_priority {
    DefaultThreadPriority = 0,
    MaxThreadPriority,
    HighThreadPriority,
    NormalThreadPriority,
    LowThreadPriority,
    MinThreadPriority
} system_thread_priority;

#define E_THREAD_DEFAULT_STACK_SIZE       0
#define E_THREAD_DEFAULT_STACK_LOC        NULL

#define E_THREAD_NO_WAIT                  0
#define E_THREAD_INFINITE_WAIT           -1

/** Thread attribute structure */
typedef struct _system_thread_attr_t {
    const char* name;                   /**< Name of the thread */
    size_t stack_size;                  /**< Stack size of the thread */
    void* stack_loc;                    /**< Start address of the thread's stack */
    system_thread_priority priority;    /**< Thread priority */
} system_thread_attr_t;

/**
 * Creates a new thread and makes it executable. This routine can be called any
 * number of times from anywhere within your code.
 *
 * @param[out] thread          - An opaque, unique identifier for the new thread returned
 *                               by the subroutine.
 * @param[in]  attr            - An opaque attribute object that may be used to set thread
 *                               attributes. You can specify a thread attributes object, or NULL
 *                               for the default values.
 * @param[in]  start_routine   - The C routine that the thread will execute once
 *                               it is created.
 * @param[in]  arg             - A single argument that may be passed to start_routine. It
 *                               must be passed by reference as a pointer cast of type void. NULL
 *                               may be used if no argument is to be passed.
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_thread_create(system_thread_t* thread,
                             const system_thread_attr_t* attr,
                             void*(*start_routine)(void*),
                             void* param);

/**
 * Creates a new mutex
 *
 * @param[out] mutex   - A valid pointer to an system_mutex structure; it will be initalize by this call
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_mutex_create(system_mutex_t* mutex);

/**
 * Destroys a mutex
 *
 * @param[in] mutex   - A valid pointer to an system_mutex structure
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_mutex_destroy(system_mutex_t mutex);

/**
 * Locks the given mutex; blocks if it is already locked
 *
 * @param[in] mutex   - A valid pointer to an system_mutex structure;
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_mutex_lock(system_mutex_t mutex);

/**
 * Tries lock the given mutex; It returns immediately; it does not block
 *
 * @param[in]  mutex      - A valid pointer to an system_mutex structure;
 * @param[out] acquired   - Returns whether it could acquire the lock or not
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_mutex_trylock(system_mutex_t mutex, uint8_t* acquired);

/**
 * Unlocks the given mutex
 *
 * @param[in] mutex   - A valid pointer to an system_mutex structure;
 *
 * @return Error code. (No error = 0)
 **/
int32_t system_mutex_unlock(system_mutex_t mutex);

/**
 * Optional! Return the free space left on the stack of this thread
 *
 * By default it is not used by the SDK; but the option is added to be
 * able to check the stack usage
 *
 * @return The free space left in bytes
 **/
size_t system_get_thread_stack_freesize(system_thread_t *thread);
/** @} end of thread_port_interface group */
#endif /* THREAD_PORT_H */
