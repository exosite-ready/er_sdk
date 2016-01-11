/* @file pmatch.c
 *
 * @brief This is a pattern matching module
 *
 * This is a rewrite of the lua pattern matching lib so that it can be
 * used with pure C code. It was reworked as part of Exosite's ER SDK
 *
 * Original note from lua's lstrlib.c follows:
 *
 * Id: lstrlib.c,v 1.132.1.5 2010/05/14 15:34:19 roberto Exp $
 * Standard library for string operations and pattern-matching
 * See Copyright Notice below
 *
 */

/******************************************************************************
* Copyright (C) 1994-2008 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <lib/debug.h>
#include <lib/type.h>
#include <lib/pmatch.h>
#include <lib/debug.h>

/* macro to `unsign' a character */
#define uchar(c)        ((unsigned char)(c))

static ptrdiff_t posrelat(ptrdiff_t pos, size_t len)
{
    /* relative string position: negative means back from end */
    if (pos < 0)
        pos += (ptrdiff_t) len + 1;
    return (pos >= 0) ? pos : 0;
}

/*
 ** {======================================================
 ** PATTERN MATCHING
 ** =======================================================
 */

#define CAP_UNFINISHED  (-1)
#define CAP_POSITION  (-2)

struct MatchState  {
    const char *src_init; /* init of source string */
    const char *src_end; /* end (`\0') of source string */
    int level; /* total number of captures (finished or unfinished) */
    struct {
        const char *init;
        ptrdiff_t len;
    } capture[LUA_MAXCAPTURES];
};

#define L_ESC   '%'
#define SPECIALS  "^$*+?.([%-"

static int check_capture(struct MatchState *ms, int l)
{
    l -= '1';
    if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
        return -1;
    return l;
}

static int capture_to_close(struct MatchState *ms)
{
    int level = ms->level;

    for (level--; level >= 0; level--)
        if (ms->capture[level].len == CAP_UNFINISHED)
            return level;

    return -1;
}

static const char *classend(struct MatchState *ms, const char *p)
{
    (void)ms;
    switch (*p++) {
    case L_ESC: {
        if (*p == '\0') {
            debug_log(DEBUG_MEM, ("Malformed pattern\n"));
        }
        return p + 1;
    }
    case '[': {
        if (*p == '^')
            p++;
        do { /* look for a `]' */
            if (*p == '\0') {
                debug_log(DEBUG_MEM, ("Malformed pattern\n"));
            }

            if (*(p++) == L_ESC && *p != '\0')
                p++; /* skip escapes (e.g. `%]') */
        } while (*p != ']');
        return p + 1;
    }
    default: {
        return p;
    }
    }
}

static int match_class(int c, int cl)
{
    int res;

    switch (tolower(cl)) {
    case 'a':
        res = isalpha(c);
        break;
    case 'c':
        res = iscntrl(c);
        break;
    case 'd':
        res = isdigit(c);
        break;
    case 'l':
        res = islower(c);
        break;
    case 'p':
        res = ispunct(c);
        break;
    case 's':
        res = isspace(c);
        break;
    case 'u':
        res = isupper(c);
        break;
    case 'w':
        res = isalnum(c);
        break;
    case 'x':
        res = isxdigit(c);
        break;
    case 'z':
        res = (c == 0);
        break;
    default:
        return cl == c;
    }
    return islower(cl) ? res : !res;
}

static int matchbracketclass(int c, const char *p, const char *ec)
{
    int sig = 1;

    if (*(p + 1) == '^') {
        sig = 0;
        p++; /* skip the `^' */
    }
    while (++p < ec) {
        if (*p == L_ESC) {
            p++;
            if (match_class(c, uchar(*p)))
                return sig;
        } else if ((*(p + 1) == '-') && (p + 2 < ec)) {
            p += 2;
            if (uchar(*(p-2)) <= c && c <= uchar(*p))
                return sig;
        } else if (uchar(*p) == c)
            return sig;
    }
    return !sig;
}

static int singlematch(int c, const char *p, const char *ep)
{
    switch (*p) {
    case '.':
        return 1; /* matches any char */
    case L_ESC:
        return match_class(c, uchar(*(p + 1)));
    case '[':
        return matchbracketclass(c, p, ep - 1);
    default:
        return (uchar(*p) == c);
    }
}

static const char *match(struct MatchState *ms, const char *s, const char *p);

static const char *matchbalance(struct MatchState *ms, const char *s, const char *p)
{
    if (*p == 0 || *(p + 1) == 0) {
        debug_log(DEBUG_MEM, ("Unbalanced pattern\n"));
        abort();
    }
    if (*s != *p)
        return NULL;
    else {
        int b = *p;
        int e = *(p + 1);
        int cont = 1;

        while (++s < ms->src_end) {
            if (*s == e) {
                if (--cont == 0)
                    return s + 1;
            } else if (*s == b)
                cont++;
        }
    }
    return NULL; /* string ends out of balance */
}

static const char *max_expand(struct MatchState *ms, const char *s, const char *p,
        const char *ep)
{
    ptrdiff_t i = 0; /* counts maximum expand for item */

    while ((s + i) < ms->src_end && singlematch(uchar(*(s + i)), p, ep))
        i++;
    /* keeps trying to match with the maximum repetitions */
    while (i >= 0) {
        const char *res = match(ms, (s + i), ep + 1);

        if (res)
            return res;
        i--; /* else didn't match; reduce 1 repetition to try again */
    }
    return NULL;
}

static const char *min_expand(struct MatchState *ms, const char *s, const char *p,
        const char *ep)
{
    for (;;) {
        const char *res = match(ms, s, ep + 1);

        if (res != NULL)
            return res;
        else if (s < ms->src_end && singlematch(uchar(*s), p, ep))
            s++; /* try with one more repetition */
        else
            return NULL;
    }
}

static const char *start_capture(struct MatchState *ms, const char *s, const char *p,
        int what)
{
    const char *res;
    int level = ms->level;

    if (level >= LUA_MAXCAPTURES) {
        debug_log(DEBUG_MEM, ("Too many captures\n"));
    }

    ms->capture[level].init = s;
    ms->capture[level].len = what;
    ms->level = level + 1;

    res = match(ms, s, p);
    if (res == NULL) /* match failed? */
        ms->level--; /* undo capture */
    return res;
}

static const char *end_capture(struct MatchState *ms, const char *s, const char *p)
{
    int l = capture_to_close(ms);
    const char *res;

    ms->capture[l].len = s - ms->capture[l].init; /* close capture */
    res = match(ms, s, p);
    if (res == NULL) /* match failed? */
        ms->capture[l].len = CAP_UNFINISHED; /* undo capture */
    return res;
}

static const char *match_capture(struct MatchState *ms, const char *s, int l)
{
    size_t len;

    l = check_capture(ms, l);

    ASSERT(ms->capture[l].len >= 0);

    len = (size_t)(ms->capture[l].len);
    if ((size_t) (ms->src_end - s) >= len
            && memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    else
        return NULL;
}

static const char *match(struct MatchState *ms, const char *s, const char *p)
{
init: /* using goto's to optimize tail recursion */
    switch (*p) {
    case '(': { /* start capture */
        if (*(p + 1) == ')') /* position capture? */
            return start_capture(ms, s, p + 2, CAP_POSITION);
        else
            return start_capture(ms, s, p + 1, CAP_UNFINISHED);
    }
    case ')': { /* end capture */
        return end_capture(ms, s, p + 1);
    }
    case L_ESC: {
        switch (*(p + 1)) {
        case 'b': { /* balanced string? */
            s = matchbalance(ms, s, p + 2);
            if (s == NULL)
                return NULL;
            p += 4;
            goto init;
            /* else return match(ms, s, p+4); */
        }
        case 'f': { /* frontier? */
            const char *ep;
            char previous;

            p += 2;
            if (*p != '[') {
                debug_log(DEBUG_MEM, ("Missing [\n"));
            }
            ep = classend(ms, p); /* points to what is next */
            previous = (s == ms->src_init) ? '\0' : *(s - 1);
            if (matchbracketclass(uchar(previous), p, ep - 1)
                    || !matchbracketclass(uchar(*s), p, ep - 1))
                return NULL;
            p = ep;
            goto init;
            /* else return match(ms, s, ep); */
        }
        default: {
            if (isdigit(uchar(*(p + 1)))) { /* capture results (%0-%9)? */
                s = match_capture(ms, s, uchar(*(p + 1)));
                if (s == NULL)
                    return NULL;
                p += 2;
                goto init;
                /* else return match(ms, s, p+2) */
            }
            goto dflt;
            /* case default */
        }
        }
    }
    case '\0': { /* end of pattern */
        return s; /* match succeeded */
    }
    case '$': {
        if (*(p + 1) == '\0') /* is the `$' the last char in pattern? */
            return (s == ms->src_end) ? s : NULL; /* check end of string */
        else
            goto dflt;
    }
    default:
dflt:
        { /* it is a pattern item */
            const char *ep = classend(ms, p); /* points to what is next */
            int m = s < ms->src_end && singlematch(uchar(*s), p, ep);

            switch (*ep) {
            case '?': { /* optional */
                const char *res = match(ms, s + 1, ep + 1);

                if (m && (res != NULL))
                    return res;
                p = ep + 1;
                goto init;
                /* else return match(ms, s, ep+1); */
            }
            case '*': { /* 0 or more repetitions */
                return max_expand(ms, s, p, ep);
            }
            case '+': { /* 1 or more repetitions */
                return m ? max_expand(ms, s + 1, p, ep) : NULL;
            }
            case '-': { /* 0 or more repetitions (minimum) */
                return min_expand(ms, s, p, ep);
            }
            default: {
                if (!m)
                    return NULL;
                s++;
                p = ep;
                goto init;
                /* else return match(ms, s+1, ep); */
            }
            }
        }
    }
}

static const char *lmemfind(const char *s1, size_t l1, const char *s2,
        size_t l2)
{
    if (l2 == 0)
        return s1; /* empty strings are everywhere */
    else if (l2 > l1)
        return NULL; /* avoids a negative `l1' */
    else {
        const char *init; /* to search for a `*s2' inside `s1' */

        l2--; /* 1st char will be checked by `memchr' */
        l1 = l1 - l2; /* `s2' cannot be found after that */
        while (l1 > 0 && (init = (const char *) memchr(s1, *s2, l1)) != NULL) {
            init++; /* 1st char is already checked */
            if (memcmp(init, s2 + 1, l2) == 0)
                return init - 1;
            else { /* correct `l1' and `s1' to try again */
                l1 -= (size_t)(init - s1);
                s1 = init;
            }
        }
        return NULL; /* not found */
    }
}

static int push_string_captures(struct MatchState *ms, const char *s, struct capture *capture, size_t len)
{
    size_t i;
    size_t nlevels = (size_t)((ms->level == 0 && s) ? 1 : ms->level);

    ASSERT(ms->level >= 0); /* verification of the casting above */

    if (nlevels > len) {
        debug_log(DEBUG_MEM, ("Capture limit exceeded\n"));
        nlevels = len;
    }

    for (i = 0; i < nlevels; i++) {
        capture[i].init = ms->capture[i].init;
        ASSERT(ms->capture[i].len >= 0);
        capture[i].len = (size_t)(ms->capture[i].len);
    }

    ASSERT(nlevels <= INT_MAX);
    return (int)(nlevels); /* number of strings pushed */
}

bool str_find(const char *s,
              size_t l1,
              const char *p,
              size_t l2,
              bool explicit,
              int32_t init_pos,
              size_t *num_of_cap,
              struct capture *capture,
              size_t len)
{
    ptrdiff_t init = posrelat(init_pos, l1) - 1;

    if (init < 0)
        init = 0;
    else if ((size_t) (init) > l1)
        init = (ptrdiff_t) l1;
    if ((explicit || /* explicit request? */
    strpbrk(p, SPECIALS) == NULL)) { /* or no special characters? */
        /* do a plain search */
        const char *s2 = lmemfind(s + init, l1 - (size_t)init, p, l2);

        if (s2)
            return 2;

    } else {
        struct MatchState ms;
        int anchor = (*p == '^') ? (p++, 1) : 0;
        const char *s1 = s + init;

        ms.src_init = s;
        ms.src_end = s + l1;
        do {
            const char *res;

            ms.level = 0;
            res = match(&ms, s1, p);
            if (res != NULL) {
                *num_of_cap = (size_t)(push_string_captures(&ms, NULL, capture, len));
                return TRUE;
            }
        } while (s1++ < ms.src_end && !anchor);
    }
    /* Not found */
    return FALSE;
}
