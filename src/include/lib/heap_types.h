/**
 * @file heap_types.h
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
 * @brief This file contains the available heap types
 *
 * One of these types can be included before sf_malloc to change the HEAP used by
 * sf_malloc
 **/

#ifndef ER_SDK_SRC_INCLUDE_HEAP_TYPES_H_
#define ER_SDK_SRC_INCLUDE_HEAP_TYPES_H_

/**
 * Maximum number of heaps must be the number of config entries + 1,
 * because the main heap does not have a configuration entry here
 **/
#define MAX_NUMBER_OF_HEAPS 5

#define MAIN_HEAP   12120001
#define STRING_HEAP 12120002
#define SSL_HEAP    12120003




#endif /* ER_SDK_SRC_INCLUDE_HEAP_TYPES_H_ */
