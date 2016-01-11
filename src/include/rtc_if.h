/**
 * @file rtc_if
 *
 * This is the interface to set the RTC on a platform
 * */

#ifndef OT_LUA_APPS_GW_MODULES_RTC_IF_H_
#define OT_LUA_APPS_GW_MODULES_RTC_IF_H_

/*TODO change it to 64 bit*/
/**
 * Sets the RTC to the given time
 *
 * When time is synchronized, this has to be called
 *
 * @param[in] The time to be set
 *
 * */
void rtc_set(sys_time_t time);

/**
 * Get real time timestamp
 * The format is: unix time
 *
 * @return the current unix time
 */
sys_time_t rtc_get(void);

/**
 * This function should be called periodically: it will update the unix time stamp
 * Argument is unused but the prototype has to follow the pclb convention
 * @return the current unix time
 */
void rtc_periodic(void* unused);

#endif /* OT_LUA_APPS_GW_MODULES_RTC_IF_H_ */
