/**
 * @file mem_check.h
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
 * @brief Provides the memory checking routines
 *
 *  In this implementation check_memory will check whether
 *  1. All HEAP canaries are safe and sane (only in DEBUG mode)
 *  2. Stack canary is safe and sane       (If a checker function registered)
 *
 *  It either condition is not met it will assert
 **/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/mem_check.h>
#include <lib/heap_types.h>
#include <porting/system_port.h>


static struct heap_space_class *heap_array[MAX_NUMBER_OF_HEAPS];
static stack_checker is_st_limit_exceeded;

void mem_check_register_heap(struct heap_space_class *heap)
{
    int32_t i;

    for (i = 0; i < MAX_NUMBER_OF_HEAPS; i++)
        if (heap_array[i] == NULL) {
            heap_array[i] = heap;
            break;
        }

}
void register_stack_checker(stack_checker checker)
{
    is_st_limit_exceeded = checker;
}

void check_memory(void)
{
#ifdef DEBUG_MEMORY_MANAGER
    int32_t i;
#endif

    /*Check the stack limiter, even if not in debug mode*/
    if (is_st_limit_exceeded && is_st_limit_exceeded()) {
        error_log(DEBUG_MEM, ("Stack limit exceeded\n"), ERR_NO_MEMORY);
        abort();
    }

#ifdef DEBUG_MEMORY_MANAGER
    for (i = 0; heap_array[i] != NULL && i < MAX_NUMBER_OF_HEAPS; i++)
        hs_check_memory(heap_array[i]);
#endif
}
