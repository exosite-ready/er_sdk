/**
 * @file platform_exos.h
 *
 * This is the platform interface for Exos
 *
 * Exos is the internal name for a system without a real RTOS
 * Exos does not offer threading, it provides only a minimal OS layer
 * that is absolutely needed to start a system
 *
 * These functions shall be implemented for ExOS to work
 *
 * */

#ifndef INC_PLATFORM_EXOS_H_
#define INC_PLATFORM_EXOS_H_

#include <lib/type.h>
#include <porting/system_port.h>

/*
 * Initalize Exos specific resources
 *
 * All the resource initalization that needs to be done for
 * ExOS has to be done here
 *
 * This might include starting some emulator resources on
 * a PC platform
 * DO NOT initalize drivers and HW at this point, it has
 * to be done independent of the OS
 *
 * */
int32_t platform_exos_init(void);

/*
 * Returns the system time in ms
 *
 * @return the system time in msec
 * */
sys_time_t platform_get_time_ms(void);

/*
 * Returns a pointer the 'end' of the stack
 * Terminology: stack base means where the stack is started to
 * be used, and grow toward the stack end
 *
 * This pointer will be used to check whether there is a
 * stack overflow. The ExOS is a one threaded system, so there is
 * only one stack
 *
 * @return A pointer to the last valid stack location
 * */
uint32_t *platform_get_stack_location(void);

/*
 * Returns a pointer the 'beginning' of the stack
 * Terminology: stack base means where the stack is started to
 * be used, and grow toward the stack end
 *
 * This pointer will be used to check stack usage if support is compiled in
 * with DEBUG_MEM_MEASUREMENTS
 * The ExOS is a one threaded system, so there is only one stack
 *
 * @return A pointer to the first valid stack location
 * */
uint32_t *platform_get_stack_base(void);

/**
 * Set the global interrupt flag to the specified value
 * The possible values are: FALSE, TRUE
 *
 * @param[in] The required Interrupt state
 * @return    The previous Interrupt state
 * */
int32_t platform_set_global_interrupts(int32_t status);

/**
 * Return the serial number of the platform
 *
 * @return Serial number as a string
 **/
const char* platform_get_sn(void);

/**
 * Returns the CIK stored in the non-volatile storage of this platform
 *
 * @param[out] cik The CIK stored in non-volatile storage
 * @param[in] len  The length of the output buffer for CIK
 *
 * @return ERR_SUCCESS on success ERR_FAILURE otherwise
 **/
int32_t platform_get_cik(char *cik, size_t len);

/**
 * Return the CIK stored in the non-volatile storage of this platform
 * param[in] cik The cik in C-string format
 *
 * @return ERR_SUCCESS on success ERR_FAILURE otherwise
 **/
int32_t platform_set_cik(const char *cik);

/**
 * Reset the board
 * */
void platform_reset(void);

#endif /* INC_PLATFORM_EXOS_H_ */
