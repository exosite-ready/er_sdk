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
 * @brief System_thread implementation for MQX
 **/
#include <stdint.h>
#include <assert.h>
#include <lib/error.h>
#include <lib/type.h>
#include <porting/thread_port.h>
#include <lib/sf_malloc.h>

#include <mqx.h>
#include <stdio.h>

int32_t system_thread_create(system_thread_t *thread, const system_thread_attr_t *attr,
        void *(*start_routine)(void *), void *param)
{
    bool error;
    TASK_TEMPLATE_STRUCT *task_template;
    /*Allocate space for two template entry, the 2nd is the filled with 0s to mark the end*/
    error = sf_mem_alloc((void **)&task_template,
            sizeof(TASK_TEMPLATE_STRUCT) * 2);
    if (error != ERR_SUCCESS)
        return error;

    if (attr->stack_size != E_THREAD_DEFAULT_STACK_SIZE)
        task_template[0].TASK_STACKSIZE = attr->stack_size;
    else
        task_template[0].TASK_STACKSIZE = 0x4000; /*Default stack size picked arbitrarily for MQX*/

    task_template[0].TASK_TEMPLATE_INDEX = 1; /*This means it will use the first entry from the template list*/
    task_template[0].TASK_ADDRESS = start_routine;
    task_template[0].TASK_PRIORITY = 8;
/*TODO The real thread name should be duplicated and the pointer passed*/
    task_template[0].TASK_NAME = "Dynamic thread";
    task_template[0].TASK_ATTRIBUTES = MQX_AUTO_START_TASK;
    task_template[0].CREATION_PARAMETER = param;
    task_template[0].DEFAULT_TIME_SLICE = 0;
    memset(&task_template[1], 0, sizeof(TASK_TEMPLATE_STRUCT));

    error = _task_create(0, 0, task_template);
    if (error == MQX_NULL_TASK_ID)
        return ERR_FAILURE;

    return ERR_SUCCESS;
}

