/**
 * @file thread.c
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
 * @brief System_thread implementation for FreeRTOS
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <sdk_config.h>
#include <porting/thread_port.h>
#include <lib/sf_malloc.h>
#include <errno.h>
#include <assert.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lib/mem_check.h>

#define FREERTOS_DEFAULT_STACK_SIZE     4096

struct thread_instance_args {
    void *(*threadCode)(void *);
    void *threadParams;
};

static void thread_instance(void *args)
{
    struct thread_instance_args *thread_args = (struct thread_instance_args *)args;
    thread_args->threadCode(thread_args->threadParams);
    
    sf_free(args);
    vTaskDelete(NULL);
}

/*
 * ADD the following line to FreeRTOSConfig.h to be able to use the stack monitoring feature of FreeRTOS
 * #define INCLUDE_uxTaskGetStackHighWaterMark         (1)
 *
 */
size_t system_get_thread_stack_freesize(system_thread_t *thread)
{
#ifdef FREERTOS_LEGACY
    xTaskHandle task = thread;

    return uxTaskGetStackHighWaterMark(task) * sizeof(portSTACK_TYPE);
#else
    TaskHandle_t task = thread;

    return uxTaskGetStackHighWaterMark(task) * sizeof(StackType_t);
#endif
}

int32_t system_thread_create(system_thread_t *thread, const system_thread_attr_t *attr,
        void *(*start_routine)(void *), void *param)
{
#ifdef FREERTOS_LEGACY
    signed portBASE_TYPE status;
    xTaskHandle new_thread;
    unsigned portBASE_TYPE priority;
#else
    BaseType_t status;
    TaskHandle_t new_thread;
    UBaseType_t priority;
#endif
    uint16_t usStackDepth = FREERTOS_DEFAULT_STACK_SIZE;
    struct thread_instance_args *args = (struct thread_instance_args *)sf_malloc(sizeof(struct thread_instance_args));
    if (!args)
        return ERR_NO_MEMORY;

    priority = NormalThreadPriority;

    *thread = NULL;

    if (attr) {

        /* Set priority */
        if (attr->priority) {

            switch (attr->priority) {
            case MaxThreadPriority:
                priority = configMAX_PRIORITIES - 1;
                break;
            case HighThreadPriority:
                priority = (3 * (configMAX_PRIORITIES - 1)) / 4;
                break;
            case DefaultThreadPriority:
            case NormalThreadPriority:
                priority = (configMAX_PRIORITIES - 1) / 2;
                break;
            case LowThreadPriority:
                priority = (configMAX_PRIORITIES - 1) / 4;
                break;
            case MinThreadPriority:
                priority = 0;
                break;
            default:
                return ERR_FAILURE;
                break;
            }
        }
    }

    args->threadCode = start_routine;
    args->threadParams = param;

    if (attr->stack_size != E_THREAD_DEFAULT_STACK_SIZE) {
        if (attr->stack_size / sizeof(portSTACK_TYPE) > 0xffff) {
            /* Required stack size is too large! It can be max 0xffff */
            assert(1);
            return ERR_FAILURE;
        }
        usStackDepth = (uint16_t)(attr->stack_size / sizeof(portSTACK_TYPE));
    }

    /* Stack location can not be configured with FreeRTOS */
    assert(attr->stack_loc == E_THREAD_DEFAULT_STACK_LOC);

#ifdef FREERTOS_LEGACY
    status = xTaskCreate(thread_instance,
                         (const signed char *)attr->name,
                         usStackDepth,
                         &args,
                         priority,
                         &new_thread);
#else
    status = xTaskCreate(thread_instance,
                         attr->name,
                         usStackDepth,
                         args,
                         priority,
                         &new_thread);
#endif

    check_memory();

    if (status != pdPASS)
        return ERR_FAILURE;

    *thread = new_thread;
    return ERR_SUCCESS;
}

