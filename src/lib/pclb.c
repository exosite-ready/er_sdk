/**
 * @file pclb.c
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
 * This file implements the Exosite Server specific HTTP API
 **/
#include <assert.h>
#include <lib/error.h>
#include <lib/type.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/pclb.h>
#include <lib/debug.h>

#include <system_utils.h>
#include <porting/thread_port.h>
#include <porting/system_port.h>


struct periodic_clb_class {
    void (*clb)(void *);
    timer_data_type period;
    timer_data_type cnt;
    int32_t id;
    void *param;
    struct periodic_clb_class *next;
    struct periodic_clb_class *shadow_next;
};

static struct periodic_clb_class *periodic_clb_head;
static struct periodic_clb_class *shadow_periodic_clb_head;
static int32_t clb_obj_cnt;
static system_mutex_t periodic_clb_mutex;
static int32_t clb_num;
static BOOL has_thread;
static system_thread_attr_t t_attr = {
    "Pclb thread",
    E_THREAD_DEFAULT_STACK_SIZE,
    E_THREAD_DEFAULT_STACK_LOC,
    NormalThreadPriority
};
static system_thread_t pclb_thread;

/*
 *  DEFINE CHECK_PCLB_STACK if you'd like to monitor the stack usage of the pcbl stack
 *  NOTE! For this system_get_thread_stack_freesize must be implemented on the platform you uses
 */
/* #define CHECK_PCLB_STACK */

#ifdef CHECK_PCLB_STACK
static void pclb_get_stack_usedsize(void)
{
    debug_log(DEBUG_MEM, ("Stack usage %d\n", t_attr.stack_size - system_get_thread_stack_freesize(pclb_thread)));
}
#endif

int32_t pclb_register(timer_data_type period, void (*fn)(void *), void *param, int32_t *id)
{

    struct periodic_clb_class *last_obj;
    struct periodic_clb_class *new_clb_obj = sf_malloc(sizeof(struct periodic_clb_class));

    ASSERT(fn);

    if (new_clb_obj == NULL)
        return ERR_NO_MEMORY;

    new_clb_obj->period = period;
    new_clb_obj->cnt = 0;
    new_clb_obj->clb = fn;
    new_clb_obj->param = param;
    new_clb_obj->next = NULL;
    new_clb_obj->shadow_next = NULL;

    system_mutex_lock(periodic_clb_mutex);

    /* Empty list */
    if (shadow_periodic_clb_head == NULL)
        shadow_periodic_clb_head = new_clb_obj;

    /* Not empty list */
    else {
        last_obj = shadow_periodic_clb_head;
        while (last_obj->shadow_next != NULL)
            last_obj = last_obj->shadow_next;
        last_obj->shadow_next = new_clb_obj;
    }

    new_clb_obj->id = clb_obj_cnt++;

    system_mutex_unlock(periodic_clb_mutex);

    if (id)
        *id = new_clb_obj->id;

    check_memory();

    clb_num++;
    ASSERT(clb_num > 0);

    return ERR_SUCCESS;
}

int32_t pclb_unregister(int32_t id)
{
    struct periodic_clb_class *clb_obj;
    struct periodic_clb_class *prev_clb_obj;
    int32_t status = ERR_FAILURE;

    system_mutex_lock(periodic_clb_mutex);

    clb_obj = periodic_clb_head;

    /* The list is empty -> error */
    if (clb_obj == NULL)
        status = ERR_FAILURE;

    /* We have to remove the head item */
    else if (clb_obj->id == id) {
        periodic_clb_head = clb_obj->next;
        sf_free(clb_obj);
        clb_num--;
        status = ERR_SUCCESS;
    }

    /* We have to remove an item that is not the first one. */
    else {
        while (clb_obj->next != NULL) {
            prev_clb_obj = clb_obj;
            clb_obj = clb_obj->next;
            if (clb_obj->id == id) {
                prev_clb_obj->next = clb_obj->next;
                sf_free(clb_obj);
                clb_num--;
                status = ERR_SUCCESS;
                break;
            }
        }
    }

    system_mutex_unlock(periodic_clb_mutex);

    check_memory();
    assert(clb_num >= 0);

    return status;
}

int32_t pclb_get_clb_num(void)
{
    int32_t ret;

    system_mutex_lock(periodic_clb_mutex);
    ret = clb_num;
    system_mutex_unlock(periodic_clb_mutex);

    return ret;
}

void pclb_task(uint32_t ms)
{
    static sys_time_t last_system_time;
    sys_time_t dtime;
    struct periodic_clb_class *clb_obj;

    system_mutex_lock(periodic_clb_mutex);

#ifdef CHECK_PCLB_STACK
    pclb_get_stack_usedsize();
#endif
    periodic_clb_head = shadow_periodic_clb_head;
    clb_obj = periodic_clb_head;
    if (clb_obj == NULL) {
        system_mutex_unlock(periodic_clb_mutex);
        return;
    }

    while (clb_obj->shadow_next != NULL) {
        clb_obj->next = clb_obj->shadow_next;
        clb_obj = clb_obj->shadow_next;
    }

    system_mutex_unlock(periodic_clb_mutex);

    if (!ms) {
        dtime = system_get_diff_time(last_system_time);
        last_system_time = system_get_time();
    } else {
        dtime = ms;
        last_system_time += ms;
    }

    clb_obj = periodic_clb_head;
    while (clb_obj != NULL) {
        clb_obj->cnt += dtime;
        if (clb_obj->period < clb_obj->cnt) {
            ASSERT(clb_obj->clb);
            clb_obj->clb(clb_obj->param);
            clb_obj->cnt = 0;
        }
        clb_obj = clb_obj->next;
    }

}

static void *periodic_clb_thread(void *arg)
{
    (void)arg;

    while (1) {
        system_sleep_ms(10);
        pclb_task(10);
    }

    return NULL;
}

BOOL pcld_has_thread(void)
{
    return has_thread;
}

int32_t pclb_init(uint32_t stack_size)
{
    int32_t status;

    if (stack_size != 0)
        t_attr.stack_size = stack_size;

    status = system_mutex_create(&periodic_clb_mutex);
    if (status != ERR_SUCCESS)
        return status;

    status = system_thread_create(&pclb_thread, &t_attr, periodic_clb_thread, NULL);
    if (status == ERR_SUCCESS)
        has_thread = TRUE;
    else
        has_thread = FALSE;

    return ERR_SUCCESS;
}
