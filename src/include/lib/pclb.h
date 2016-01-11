/**
 * @file pclb.h
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
 * @defgroup pclb Periodic Callback interface
 * This module provides timed function calling service.
 * Callback functions can be registered with a predefined
 * timing. These functions are called from the context
 * of this module periodically based on the given timing.
 * @{
 */

#ifndef INC_PCLB_H_
#define INC_PCLB_H_

/**
 * @brief Initialization of the periodic callback module
 *
 * This function has to be called from outside periodically
 *
 * param[in] stack_size Stack size of the pclb internal
 *                      thread. If the system does not use
 *                      threads, this parameter should
 *                      be zero.
 *
 * @return none
 **/
extern int32_t pclb_init(uint32_t stack_size);

/**
 * @brief Get status of the pclb thread
 *
 * Returns with TRUE if the pclb module has own thread.
 * In this case calling of the pclb_task() from outside is
 * not a must.
 *
 * param[in] stack_size Stack size of the pclb internal
 *                      thread. If the system does not use
 *                      threads, this parameter should
 *                      be zero.
 *
 * @return none
 **/
extern BOOL pcld_has_thread(void);

/**
 * @brief Task of the periodic callback module.
 *
 * This function has to be called from outside periodically, if
 * the system does not have thread support.
 *
 * param[in] ms Elapsed time since the previous call. [ms]
 *              If this parameter is 0, the module will try to
 *              calculate the correct value with system_get_time()
 *              function call
 *
 * @return none
 **/
extern void pclb_task(uint32_t ms);

/**
 * @brief Get number of the registered callback functions
 *
 * param none
 *
 * @return Number of the callback functions
 **/
extern int32_t pclb_get_clb_num(void);

/**
 * @brief  Register a function to be called periodically
 *
 * param[in] period Length of the callback period in ms.
 * param[in] fn     Callback function
 * param[in] param  Parameter that will be passed to the callback function
 * param[out] id    Id of the callback object. It can be used for unregister
 *                  the callback function by calling the
 *                  system_unregister_periodic_clb() function
 *
 * @return ERR_SUCESS on success
 **/
extern int32_t pclb_register(timer_data_type period, void (*fn)(void*), void* param,
        int32_t* id);

/**
 * @brief  Unregister a periodic callback function
 *
 * param[in] id   Id of the callback object. This parameter has been passed
 *                back by the system_register_periodic_clb() function.
 *
 * @return ERR_SUCESS on success
 **/
extern int32_t pclb_unregister(int32_t id);

#endif /* INC_PCLB_H_ */

/** @} end of pclb group */
