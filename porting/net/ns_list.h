/**
 * @file ns_list.h
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
 * @brief Identifiers for the networking stacks
 **/
#ifndef ER_SDK_PORTING_NETWORKING_NS_LIST_H_
#define ER_SDK_PORTING_NETWORKING_NS_LIST_H_

#define cfg_uip         (0x00)
#define cfg_atheros     (0x01)
#define cfg_lwip        (0x02)
#define cfg_posix       (0x03)
#define cfg_wcm         (0x04)
#define cfg_custom      (0xff)

#endif /* ER_SDK_PORTING_NETWORKING_NS_LIST_H_ */
