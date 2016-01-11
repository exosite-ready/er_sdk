/**
 * @file string_class_priv.h
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
 * @brief Private methods for the string_class
 **/

#ifndef INC_STRING_CLASS_PRIV_H_
#define INC_STRING_CLASS_PRIV_H_

/*
 * Private functions
 *
 * */
#ifdef BUILD_UNITTEST
size_t string_get_buffer_size(struct string_class *str);

#endif

#endif /* INC_STRING_CLASS_PRIV_H_ */
