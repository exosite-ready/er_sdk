/**
 * @file string_class.c
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
 * @brief
 * A string implementation
 **/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/use_string_heap.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/string_class.h>
#include <lib/string_class_priv.h>
#include <lib/debug.h>
#include <assert.h>

struct string_class {
    char *buffer;
    size_t size;
    size_t bufsize;
    BOOL fixed_size;
};

/**
 * String's length is determined by the internal size variable they
 * can contain '\0' in them
 *
 * Strings are however terminated by a '\0' but this is only a convenience
 * when the string's buffer is requested as a C string. The callee will
 * then get a proper NULL terminated string
 * Why is that?
 * Sometimes we need the C string, e.g the pattern matching functions
 * takes the pattern as a NULL terminated string: we could ask for a
 * copy of the string but then we would have to allocate buffers and
 * we would lose the simplicity
 *
 * Otherwise the string class only relies on the size of the string
 *
 */
int32_t string_new(struct string_class **new_string, const char *init_string)
{
    struct string_class *str;

    *new_string = NULL;

    str = sf_malloc(sizeof(*str));
    if (!str)
        return ERR_NO_MEMORY;

    if (init_string != NULL)
        str->size = strlen(init_string);
    else
        str->size = 0;

    /* TODO WORKAROUND
     * PUT back realloc workaround until dlmalloc is fixed for microchip
     * If --wrap is used on WCM it seems to interfere with nvm_config_write
     * Workaround: increase buffer size to avoid realloc
     **/
    str->bufsize = str->size + 1 + 256; /* Allocate buffer space for the '\0' as well */
    str->buffer = sf_malloc(str->bufsize);
    if (!str->buffer) {
        sf_free(str);
        return ERR_NO_MEMORY;
    }

    if (str->size != 0)
        sf_memcpy(str->buffer, init_string, str->size);

    str->buffer[str->size] = '\0';
    str->fixed_size = FALSE;

    *new_string = str;
    SET_STRING_CHECKER(*new_string, 0);
    check_memory();
    return ERR_SUCCESS;
}

int32_t string_new_const(struct string_class **new_string, const char *init_string, size_t size)
{
    struct string_class *str;

    *new_string = NULL;

    str = sf_malloc(sizeof(*str));
    if (!str)
        return ERR_NO_MEMORY;

    str->size = size;
    str->bufsize = str->size + 1; /* Allocate buffer space for the '\0' as well */
    str->buffer = sf_malloc(str->bufsize);
    if (!str->buffer) {
        sf_free(str);
        return ERR_NO_MEMORY;
    }

    if (str->size != 0)
        sf_memcpy(str->buffer, init_string, str->size);

    str->buffer[str->size] = '\0';
    str->fixed_size = TRUE;

    *new_string = str;

    SET_STRING_CHECKER(*new_string, 0);
    check_memory();
    return ERR_SUCCESS;
}

int32_t string_catcn(struct string_class *str,
                     const char *new_chunk,
                     size_t new_chunk_size)
{
    if (new_chunk_size == 0)
        return 0;

    if (str->fixed_size)
        return ERR_FAILURE;

    if (new_chunk_size + str->size > (str->bufsize - 1)) {
        char *new_buf = sf_realloc(str->buffer, str->bufsize + new_chunk_size);

        if (new_buf)
            str->buffer = new_buf;
        else
            return ERR_NO_MEMORY;

        str->bufsize = str->bufsize + new_chunk_size;
    }

    check_memory();
    sf_memcpy(str->buffer + str->size, new_chunk, new_chunk_size);
    str->size = str->size + new_chunk_size;
    str->buffer[str->size] = '\0';
    check_memory();
    return ERR_SUCCESS;
}

int32_t string_catc(struct string_class *str, const char *str2)
{
    return string_catcn(str, str2, strlen(str2));
}

int32_t string_cat(struct string_class *str, struct string_class *str2)
{
    return string_catcn(str, str2->buffer, str2->size);
}

int32_t string_replace(struct string_class *str, const char *substr,
        const char *replacement)
{
    char *tok = NULL;
    char *oldstr = NULL;
    char *head = NULL;

    /* if either substr or replacement is NULL, then there is nothing to do */
    if (substr == NULL || replacement == NULL)
        return ERR_SUCCESS;

    head = str->buffer;
    while ((tok = strstr(head, substr))) {
        oldstr = str->buffer;
        str->buffer = sf_malloc(str->size - strlen(substr) + strlen(replacement) + 1);
        if (!str->buffer)
            return ERR_NO_MEMORY;

        str->size = str->size - strlen(substr) + strlen(replacement);
        str->bufsize = str->size + 1;

        sf_memcpy(str->buffer, oldstr, (size_t)(tok - oldstr));
        sf_memcpy(str->buffer + (tok - oldstr), replacement, strlen(replacement));
        sf_memcpy(str->buffer + (tok - oldstr) + strlen(replacement),
               tok + strlen(substr),
               strlen(oldstr) - strlen(substr) - (size_t)(tok - oldstr));
        memset(
                str->buffer + strlen(oldstr) - strlen(substr)
                        + strlen(replacement), 0, 1);
        /* move back head right after the last replacement */
        head = str->buffer + (tok - oldstr) + strlen(replacement);
        free(oldstr);
    }

    return ERR_SUCCESS;
}

bool string_is_equal(struct string_class *str, const char *cstring)
{
    if (!strcmp(str->buffer, cstring))
        return TRUE;
    else
        return FALSE;
}

void string_empty(struct string_class *str)
{
    str->size = 0;
}

char *string_get_cstring(struct string_class *str)
{
    if (str == NULL)
        return NULL;

    return str->buffer;
}

char *string_get_buffer(struct string_class *str)
{
    if (str == NULL)
        return NULL;

    return str->buffer;
}

size_t string_get_size(struct string_class *str)
{
    if (str == NULL)
        return 0;

    return str->size;
}

#ifdef BUILD_UNITTEST
size_t string_get_buffer_size(struct string_class *str)
{
    return str->bufsize;
}
#endif

void string_delete(struct string_class *str)
{
    if (str == NULL)
        return;

    sf_free(str->buffer);
    sf_free(str);
}

void string_print(struct string_class *str)
{
    char *buf;

    if (str->size == 0) {
        debug_log(DEBUG_MEM, ("String is empty:\n"));
        return;
    }

    buf = sf_malloc(str->size + 1);
    if (!buf)
        return;

    sf_memcpy(buf, str->buffer, str->size);
    buf[str->size] = '\0';
    debug_log(DEBUG_MEM, ("String: %s\n", buf));
    sf_free(buf);
}

#ifdef DEBUG_EASDK
#if 0
static int32_t string_mem_check(const sf_byte *ptr, size_t length,
                            const sf_byte *adv_info, size_t adv_info_length)
{
    struct string_class *str = (struct string_class *)ptr;
    uint32_t type = (uint32_t)adv_info;

    (void)length;
    /* Size checking */
    assert(adv_info_length == sizeof(type));

    if (str->bufsize != sizeof_block((sf_byte *)str->buffer)) {
        assert(0);
        return ERR_FAILURE;
    }

    if (str->bufsize < str->size + 1) {
        assert(0);
        return ERR_FAILURE;
    }

    if (str->size != strlen(str->buffer)) {
        assert(0);
        return ERR_FAILURE;
    }

    if (type & STR_CONSTANT) {
        ;; /* TODO */
    }
    if (type & STR_READABLE) {
        ;; /* TODO */
    }

    return ERR_SUCCESS;
}

void string_add_checker(struct string_class *str, uint32_t type)
{
    new_adv_block_info((sf_byte *)str, (sf_byte *)&type, sizeof(type), string_mem_check);
}
#endif
#endif
