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
 * @brief System_thread implementation for POSIX
 **/
#include <lib/error.h>
#include <lib/type.h>
#include <porting/thread_port.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

int32_t system_thread_create(system_thread_t *thread, const system_thread_attr_t *attr,
        void*(*start_routine)(void *), void *param)
{
    int32_t status = ERR_SUCCESS;
    int32_t sched_policy;
    int32_t priority_max, priority_min;
    struct sched_param sched_param;
    pthread_attr_t pthread_attr;

    if (attr) {

        status = pthread_attr_init(&pthread_attr);

        /* Set priority */
        if (attr->priority && !status) {

            status = pthread_attr_getschedpolicy(&pthread_attr, &sched_policy);

            if (!status) {
                switch (attr->priority) {
                case MaxThreadPriority:
                    sched_param.sched_priority = sched_get_priority_max(
                            sched_policy);
                    break;
                case HighThreadPriority:
                    priority_max = sched_get_priority_max(sched_policy);
                    priority_min = sched_get_priority_min(sched_policy);
                    sched_param.sched_priority = (priority_min
                            + 3 * priority_max) / 4;
                    break;
                case NormalThreadPriority:
                    priority_max = sched_get_priority_max(sched_policy);
                    priority_min = sched_get_priority_min(sched_policy);
                    sched_param.sched_priority = (priority_min + priority_max)
                            / 2;
                    break;
                case LowThreadPriority:
                    priority_max = sched_get_priority_max(sched_policy);
                    priority_min = sched_get_priority_min(sched_policy);
                    sched_param.sched_priority = (priority_max
                            + 3 * priority_min) / 4;
                    break;
                case MinThreadPriority:
                    sched_param.sched_priority = sched_get_priority_min(
                            sched_policy);
                    break;
                default:
                    status = ERR_FAILURE;
                    break;
                }
            }

            if (!status)
                status = pthread_attr_setschedparam(&pthread_attr,
                        &sched_param);
        }

        /* Set stack address */
        if (attr->stack_loc && !status)
            status = pthread_attr_setstackaddr(&pthread_attr, attr->stack_loc);

        /* Set stack size */
        if (attr->stack_size && !status)
            status = pthread_attr_setstacksize(&pthread_attr, attr->stack_size);

        if (!status)
            status = pthread_create((pthread_t *) thread, &pthread_attr,
                    start_routine, param);

        pthread_attr_destroy(&pthread_attr);
    } else {
        status = pthread_create((pthread_t *) thread, NULL, start_routine,
                param);
    }

    if (status)
        return ERR_FAILURE;

    return ERR_SUCCESS;
}

