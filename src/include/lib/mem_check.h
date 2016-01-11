#ifndef EXOSITE_ACTIVATOR_SDK_INC_MEM_CHECK_H_
#define EXOSITE_ACTIVATOR_SDK_INC_MEM_CHECK_H_
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
 * @brief Provides the memory checking interface
 **/

#include <lib/type.h>
#include <src/lib/heap_space_internal.h>

/**
 * @brief A heap can be registered to memory checker with this function
 *
 * If it is not registered the heap will still be checked in allocator calls
 *
 * @param[in] heap    The heap to be registered
 *
 **/
void mem_check_register_heap(struct heap_space_class *heap);

/**
 * @brief This function has to be called to force a memory check
 *
 * It shall assert if any problem is found
 */
void check_memory(void);


/**
 * @brief A stack checker function that returns TRUE if everything is OK false otherwise
 **/
typedef bool (*stack_checker)(void);

/**
 * @brief With this function a stack checker can be registered to the memory checker
 *
 * This function allows to register a checker function which should
 * return TRUE if the stack canary is safe and sane and false otherwise
 */
void register_stack_checker(stack_checker checker);

#endif /* EXOSITE_ACTIVATOR_SDK_INC_MEM_CHECK_H_ */
