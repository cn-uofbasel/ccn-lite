/**
 * @addtogroup CCNL-core
 * @{
 *
 * @file ccnl-malloc.h
 * @brief Malloc (re-)definition of CCN-lite
 *
 * Copyright (C) 2011-18, University of Basel
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
 */
#ifndef CCNL_MALLOC_H
#define CCNL_MALLOC_H

#ifndef CCNL_LINUXKERNEL
#include <stdlib.h>
#include <string.h>
#include "ccnl-os-time.h"
#endif //CCNL_LINUXKERNEL


#ifdef USE_DEBUG_MALLOC
struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno;
    size_t size;
#ifdef CCNL_ARDUINO
    double tstamp;
#else
    char *tstamp; // Linux kernel (no double), also used for CCNL_UNIX
#endif // CCNL_ARDUINO
} *mem;
#endif // USE_DEBUG_MALLOC


#ifdef USE_DEBUG_MALLOC

void *debug_realloc(void *p, size_t s, const char *fn, int lno);
void debug_free(void *p, const char *fn, int lno);

#ifdef CCNL_ARDUINO
void*
debug_malloc(size_t num, size_t size, const char *fn, int lno, double tstamp);
void*
debug_calloc(size_t num, size_t size, const char *fn, int lno, double tstamp);
void*
debug_strdup(const char *s, const char *fn, int lno, double tstamp);

#  define ccnl_malloc(s)        debug_malloc(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_calloc(n,s)      debug_calloc(n, s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_realloc(p,s)     debug_realloc(p, s, PSTR(__FILE__), __LINE__)
#  define ccnl_strdup(s)        debug_strdup(s, PSTR(__FILE__), __LINE__, CCNL_NOW())
#  define ccnl_free(p)          debug_free(p, PSTR(__FILE__), __LINE__)

#else 
void*
debug_malloc(size_t s, const char *fn, int lno, char *tstamp);
void* 
debug_calloc(size_t num, size_t size, const char *fn, int lno, char *tstamp);
void*
debug_strdup(const char *s, const char *fn, int lno, char *tstamp);

#  define ccnl_malloc(s)        debug_malloc(s, __FILE__, __LINE__,timestamp())
#  define ccnl_calloc(n,s)      debug_calloc(n, s, __FILE__, __LINE__,timestamp())
#  define ccnl_realloc(p,s)     debug_realloc(p, s, __FILE__, __LINE__)
#  define ccnl_strdup(s)        debug_strdup(s, __FILE__, __LINE__,timestamp())
#  define ccnl_free(p)          debug_free(p, __FILE__, __LINE__)

#endif // CCNL_ARDUINO

#else // !USE_DEBUG_MALLOC


# ifndef CCNL_LINUXKERNEL
#  define ccnl_malloc(s)        malloc(s)
    #ifdef __linux__ 
    char* strdup(const char* str);// {
    //    return strcpy( ccnl_malloc( strlen(str)+1), str );
    //}
    #endif
#  define ccnl_calloc(n,s)      calloc(n,s)
#  define ccnl_realloc(p,s)     realloc(p,s)
#  define ccnl_strdup(s)        strdup(s)
#  define ccnl_free(p)          free(p)
# endif

#endif// USE_DEBUG_MALLOC

#ifdef CCNL_LINUXKERNEL


/**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param[in] size Size of the memory block, in bytes
 *
 * @return Upon failure, the function returns NULL
 * @return Upon success, a pointer to the memory block allocated by the function
 */
/*
static inline void*
ccnl_malloc(size_t s);

static inline void*
ccnl_calloc(size_t num, size_t size);

static inline void
ccnl_free(void *ptr);*/
#endif

#endif 
/** @} */
