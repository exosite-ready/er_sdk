/** @file pmatch.h
 *
 * This is the interface for the pattern matching module;
 *
 */

#ifndef OT_SDK_INC_PMATCH_H_
#define OT_SDK_INC_PMATCH_H_

#include <stddef.h>

#define LUA_MAXCAPTURES 20

struct capture {
    const char *init;
    size_t len;
};

/*
 * Tries to match the pattern p to string s
 *
 * While the size of the pattern needs to be provided the pattern HAS to be a NULL terminated string
 *
 * @param[in] s        - The input string; need not to be NULL terminated
 * @param[in] l1       - The size of the input string
 * @param[in] p        - The pattern to match; Shall be NULL terminated
 * @param[in] l2       - The length of the pattern
 * @param[in] explicit - If True then this function does not pattern match just does a plain find
 * @param[in] init_pos - The starting position for the pattern match, if you want to match the whole string just put 0
 * @param[out] cap     - The captures
 * @param[out] max_cap - The maximum number of captures
 *
 * @returns FALSE if there is no matching. Otherwise it is TRUE
 * */
bool str_find(const char *s,
              size_t l1,
              const char *p,
              size_t l2,
              bool explicit,
              int32_t init_pos,
              size_t *num_of_cap,
              struct capture *cap,
              size_t max_cap);

#endif /* OT_SDK_INC_PMATCH_H_ */
