/**
 * @file dictionary.h
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
 * @brief The dictionary interface
 *
 * This interface uses C strings as keys
 *
 **/
#ifndef DICTIONARY_H
#define DICTIONARY_H

struct dictionary_class;
struct dict_iterator;

/**
 * @brief Creates a new dictionary
 *
 * @param[in] *dict  The newly created dictionary
 *
 * @return ERR_SUCCESS on success ERR_* failure otherwise
 **/
int32_t dictionary_new(struct dictionary_class **new_dict);

/**
 * @brief Looks up the value with the key provided
 *
 * @param[in] dict   The dictionary
 * @param[in] key    The key
 **/
void *dictionary_lookup(struct dictionary_class *dict, const char *key);

/**
 * @brief Add a new key/value pair to the dictionary
 *
 * @param[in] dict  The dictionary itself
 * @param[in] key   The key
 * @param[in] value The value to the key
 *
 * @return ERR_SUCCESS on success ERR_* failure otherwise
 **/
int32_t dictionary_add(struct dictionary_class *dict, const char *key, void *value);

/**
 * @brief Remove a key/value pair from the dictionary
 *
 * @param[in] dict  The dictionary itself
 * @param[in] key   The key which should be remove from the dictionary
 *
 * @return ERR_SUCCESS on success ERR_* failure otherwise
 **/
int32_t dictionary_remove(struct dictionary_class *dict, char *key);

/**
 * @brief Prints out the dictionary
 *
 * @param[in] dict  The dictionary itself
 *
 **/
void dictionary_print(struct dictionary_class *dict);

/**
 * @brief Gets a pointer to the beginning of the dictionary
 *
 * @param[in] dict  The dictionary itself
 *
 * @return a dict_iterator* pointing to this value
 **/
struct dict_iterator *dictionary_get_iterator(struct dictionary_class *dict);

/**
 * @brief Returns an iterator pointing to the next element
 *
 * @param[in] it    A dictonary iterator
 *
 * @return a dict_iterator* pointing to this value
 **/
struct dict_iterator *dictionary_iterate(struct dict_iterator *it);

/**
 * @brief Gets the value for the key/value pair pointed by the iterator provided
 *
 * @param[in] it    A dictonary iterator
 *
 * @return a pointer to the value or NULL if the iterator was NULL
 **/
void *dictionary_get_value(struct dict_iterator *it);

#endif /* DICTIONARY_H */
