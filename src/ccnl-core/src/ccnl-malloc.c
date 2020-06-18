/*
 * @f ccnl-malloc.c
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2017-06-16 created
 */
#include "ccnl-malloc.h"
#include "ccnl-logging.h"
#include "ccnl-overflow.h"


#ifdef USE_DEBUG_MALLOC

#ifdef CCNL_ARDUINO
void* debug_malloc(size_t s, const char *fn, int lno, double tstamp)
#else
void* debug_malloc(size_t s, const char *fn, int lno, char *tstamp)
#endif
{
    size_t size;
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
    /** check if the operation can be performed without causing an integer overflow */
    if (!INT_ADD_OVERFLOW(s, sizeof(struct mhdr), &size)) {
#else
    size = s + sizeof(struct mhdr);
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
        struct mhdr *h = (struct mhdr *) malloc(size); 
        /** memory allocation failed */
        if (!h) {
            return NULL;
        }

        h->next = mem;
        mem = h;
        h->fname = (char *) fn;
        h->lineno = lno;
        h->size = s;

#ifdef CCNL_ARDUINO
        h->tstamp = tstamp;
#else
        size_t new_timestamp_size;
        /** determine size of the timestamp */
        size_t timestamp_size = strlen(tstamp);
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
        /** check if +1 can safely be added */
        if (!INT_ADD_OVERFLOW(timestamp_size, 1, &new_timestamp_size)) {
#else
        new_timestamp_size = timestamp_size + 1;
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
            char *timestamp = malloc(new_timestamp_size); 

            if (timestamp) {
                h->tstamp = strncpy(timestamp, tstamp, new_timestamp_size);
            /** allocating the timestamp failed */
            } else { 
                /** free previously allocated memory */
                free(h);
                return NULL;
            }
        /** potential integer overflow detected, apparently tstamp was 'garbage'  */
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
        } else {
            /** free previously allocated memory */
            free(h);
            return NULL;
        }
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE

#endif  // CCNL_ARDUINO
        return ((unsigned char *)h) + sizeof(struct mhdr);
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
    }

    return NULL;
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
}

#ifdef CCNL_ARDUINO
void* debug_calloc(size_t n, size_t s, const char *fn, int lno, double tstamp)
#else
void* debug_calloc(size_t n, size_t s, const char *fn, int lno, char *tstamp)
#endif
{
    size_t size;
    void *p = NULL;
#ifndef BUILTIN_INT_MULT_OVERFLOW_DETECTION_UNAVAILABLE
    /** can the operation be performed without causing an integer overflow */
    if (!INT_MULT_OVERFLOW(n, s, &size)) {
#else
    size = n * s;
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
         p = debug_malloc(size, fn, lno, tstamp);

         if (p) {
            memset(p, 0, n*s);
         }
#ifndef BUILTIN_INT_MULT_OVERFLOW_DETECTION_UNAVAILABLE
    }
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE

    return p;
}

int
debug_unlink(struct mhdr *hdr)
{
    struct mhdr **pp = &mem;

    for (pp = &mem; pp; pp = &((*pp)->next)) {
        if (*pp == hdr) {
            *pp = hdr->next;
            return 0;
        }
    if (!(*pp)->next)
            break;
    }
    return 1;
}

void*
debug_realloc(void *p, size_t s, const char *fn, int lno)
{
    size_t size;
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
    /** 
     * check if the add operation in the realloc/malloc call below would cause an 
     * integer overflow 
     */
    if (INT_ADD_OVERFLOW(s, sizeof(struct mhdr), &size)) {
        return NULL;
    }
#else 
    size = s + sizeof(struct mhdr);
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
    if (p) {
        if (debug_unlink(h)) {
            CONSOLE("%s @@@ memerror - realloc(%s:%d) at "
                    "%s:%d does not find memory block\n",
                    timestamp(), h->fname, h->lineno, fn, lno);
            return NULL;
        }

        h = (struct mhdr *) realloc(h, size);

        if (!h) {
            return NULL;
        }
    } else {
        h = (struct mhdr *) malloc(size);

        if (!h) {
            return NULL;
        }
    }

    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
    h->next = mem;
    mem = h;
    return ((unsigned char *)h) + sizeof(struct mhdr);
}

#ifdef CCNL_ARDUINO
void* debug_strdup(const char *s, const char *fn, int lno, double tstamp)
#else
void* debug_strdup(const char *s, const char *fn, int lno, char *tstamp)
#endif
{
    char *cp = NULL;

    if (s) {
        size_t size;
        size_t str_size = strlen(s);
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
        if (!INT_ADD_OVERFLOW(str_size, 1, &size)) {
#else
        size = str_size + 1;

#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
            cp = (char*) debug_malloc(size, fn, lno, tstamp);
             
            if (cp) {
                strncpy(cp, s, size);
            }
        }
#ifndef BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE
    }
#endif // BUILTIN_INT_ADD_OVERFLOW_DETECTION_UNAVAILABLE

    return cp;
}

void
debug_free(void *p, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (!p) {
//      CONSOLE("%s: @@@ memerror - free() of NULL ptr at %s:%d\n",
//         timestamp(), fn, lno);
        return;
    }
    if (debug_unlink(h)) {
        CONSOLE(
           "%s @@@ memerror - free() at %s:%d does not find memory block %p\n",
                timestamp(), fn, lno, p);
        return;
    }
#ifndef CCNL_ARDUINO
    if (h->tstamp && *h->tstamp)
         free(h->tstamp);
#endif
    //free(h);
    // instead of free: do a
       memset(h+1, 0x8f, h->size);
    // to discover continued use of a freed memory zone
}

#endif // USE_DEBUG_MALLOC
