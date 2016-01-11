/**
 * @file use_ssl_heap.h
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
 * @brief Include this header to a file if you want to use ssl heap in the file
 **/
#ifndef ER_SDK_SRC_INCLUDE_LIB_USE_SSL_HEAP_H_
#define ER_SDK_SRC_INCLUDE_LIB_USE_SSL_HEAP_H_

#include <config.h>
/**
 * Include this header file before sf_malloc.h to use string heap in that module
 * */
#ifdef SF_MALLOC_H
#error "sf_malloc.h was included before this header file; this is probably not what you intend"
#endif

#if CONFIG_HAS_SSL_HEAP==1
#define HEAP_MODULE_ID SSL_HEAP
#endif



#endif /* ER_SDK_SRC_INCLUDE_LIB_USE_SSL_HEAP_H_ */
