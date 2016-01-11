/**
 * @file string_class.h
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
 * @brief Provides the string interface
 **/

#ifndef EXOSITE_ACTIVATOR_SDK_INC_STRING_CLASS_H_
#define EXOSITE_ACTIVATOR_SDK_INC_STRING_CLASS_H_

struct string_class;

#ifdef DEBUG_EASDK

#define STR_READABLE    1
#define STR_CONSTANT    2

void string_add_checker(struct string_class* str, uint32_t type);

#if 0
#define SET_STRING_CHECKER(str, type) string_add_checker(str, type)
#else
#define SET_STRING_CHECKER(str, type)
#endif
#else
#define SET_STRING_CHECKER(str, type)
#endif

/**
 * This constructs a new string object
 *
 * @param[in] init_string The constructor initalizes the string
 *                        with this, it can be empty: ""
 *
 * @param[out] new_str    The freshly constructed string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_new(struct string_class **new_str, const char *init_string);


/**
 * This constructs a new const string object from a non C string
 *
 * @param[in] init_string The constructor initalizes the string with this, it does not have to be null terminated
 * @param[in] size        The size of the init string
 * @param[out] new_str    The freshly constructed string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_new_const(struct string_class **new_string, const char *init_string, size_t size);

/**
 * This concatenates a string object with a  C string of specified length
 *
 * @param[in] str            The string object to concatenate to
 * @param[in] new_chunk      The C string
 * @param[in] new_chunk size The size of the C string
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_catcn(struct string_class *str,
                     const char *new_chunk,
                     size_t new_chunk_size);

/**
 * This concatenates a string object with a  C string
 * The C string has to be null terminated
 *
 * @param[in] str            The string object to concatenate to
 * @param[in] new_chunk      The null terminated C string
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_catc(struct string_class *str, const char *str2);

/**
 * This concatenates two string objects
 *
 * @param[in] str       The string object to concatenate to
 * @param[in] str2      The second string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_cat(struct string_class *str, struct string_class *str2);

/**
 * This replaces a substr with replacement in the give string object
 *
 * While the string_class itself does not rely on terminating '\0'
 * characters the current implementation of string_replace
 * uses strtok so do not use this with strings that contains '\0' in
 * the middle
 *
 * @param[in] str       The string object to concatenate to
 * @param[in] str2      The second string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
int32_t string_replace(struct string_class *str, const char *substr,
        const char *replacement);

/**
 * Compares a string and a cstring
 *
 * Uses strcmp internally, so it is only useful for strings that do not contain '\0' in them
 *
 * @param[in] The string to be compared
 * @param[in] The cstring to be compared
 *
 * @return TRUE if they are equal FALSE if they are not
 **/
bool string_is_equal(struct string_class *str, const char *cstring);

/**
 * Clears the content of the string
 *
 * @param[in] The string to be cleared
 **/
void string_empty(struct string_class *str);

/**
 * This gets the string as a C string
 *
 * No copy is made, the internal representation makes this possible
 *
 * @param[in] str   The string object
 *
 * @return Pointer to the buffer as a C string
 **/
char * string_get_cstring(struct string_class *str);

/**
 * This gets the buffer of the string object
 * in form of a character array
 * The character array is not NULL terminated
 *
 * @param[in] str   The string object
 *
 * @return pointer to the string buffer
 **/
char * string_get_buffer(struct string_class *str);

/**
 * This gets the length of the string object
 *
 * @param[in] str  The string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
size_t string_get_size(struct string_class *str);

/**
 * This deletes the string object
 *
 * @param[in] str  The string object
 *
 * @return None
 **/
void string_delete(struct string_class *str);

/**
 * Prints the string object
 *
 * @param[in] str  The string object
 *
 * @return ERR_SUCCESS on success ERR_* error code otherwise
 **/
void string_print(struct string_class *str);


typedef void *(*ersdk_malloc)(size_t);
typedef void *(*ersdk_realloc)(void *, size_t);
typedef void (*ersdk_free)(void *);

void string_init(ersdk_malloc, ersdk_free, ersdk_realloc);


#endif /* EXOSITE_ACTIVATOR_SDK_INC_STRING_CLASS_H_ */
