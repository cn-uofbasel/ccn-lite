/*
 * @f ccnl-ext-logging.c
 * @b CCNL logging 
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
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
 * 2014-12-15 created <christopher.scherb@unibas.ch>
 */


#ifndef CCNL_EXT_LOGGING_C
#define CCNL_EXT_LOGGING_C

#ifdef USE_LOGGING

#include "ccnl-os-time.c"

extern int debug_level;

#define FATAL   94 // FATAL
#define ERROR   95 // ERROR
#define WARNING 96 // WARNING 
#define INFO    97 // INFO 
#define DEBUG   98 // DEBUG 
#define TRACE   99 // TRACE 
#define VERBOSE 100 // VERBOSE 

#define _TRACE(F,P) if (debug_level >= TRACE)                            \
                      fprintf(stderr, "[%c] %s: %s() in %s:%d\n",        \
                              (P), timestamp(), (F), __FILE__, __LINE__)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define TRACEIN(F)    do { _TRACE(__func__, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE(__func__, '<'); } while (0)
#else
#  define TRACEIN(F)    do { _TRACE("" F, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE("" F, '<'); } while (0)
#endif

char
ccnl_debugLevelToChar(int level)
{
    switch (level) {
        case FATAL:     return 'F';
        case ERROR:     return 'E';
        case WARNING:   return 'W';
        case INFO:      return 'I';
        case DEBUG:     return 'D';
        case TRACE:     return 'T';
        case VERBOSE:   return 'V';
        default:        return '?';
    }
}

int
ccnl_debug_str2level(char *s)
{
    if (!strcmp(s, "fatal"))   return FATAL;
    if (!strcmp(s, "error"))   return ERROR;
    if (!strcmp(s, "warning")) return WARNING;
    if (!strcmp(s, "info"))    return INFO;
    if (!strcmp(s, "debug"))   return DEBUG;
    if (!strcmp(s, "trace"))   return TRACE;
    if (!strcmp(s, "verbose")) return VERBOSE;
    return 1;
}

#define DEBUGSTMT(LVL, ...) do { \
        if ((LVL)>debug_level) break; \
        __VA_ARGS__; \
} while (0)

#ifndef CCNL_LINUXKERNEL
#  define DEBUGMSG(LVL, ...) do {                   \
        if ((LVL)>debug_level) break;               \
        fprintf(stderr, "[%c] %s: ",                \
            ccnl_debugLevelToChar(LVL),             \
            timestamp());                           \
        fprintf(stderr, __VA_ARGS__);               \
    } while (0)
/*
        if (debug_level >= TRACE)                   \
            fprintf(stderr, "[>] %s: %s() in %s:%d\n",         \
                    timestamp(), __func__, __FILE__, __LINE__); \

 */

#else // CCNL_LINUXKERNEL
#  define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        printk("%s: ", THIS_MODULE->name);      \
        printk(__VA_ARGS__);            \
    } while (0)
#  define fprintf(fd, ...)      printk(__VA_ARGS__)
#endif // CCNL_LINUXKERNEL

#else // !USE_LOGGING
#  define DEBUGSTMT(LVL, ...)                   do {} while(0)
#  define DEBUGMSG(LVL, ...)                    do {} while(0)
#  define TRACEIN(...)                          do {} while(0)
#  define TRACEOUT(...)                         do {} while(0)

#endif // USE_LOGGING

#ifdef USE_DEBUG_MALLOC
void
debug_memdump()
{
    struct mhdr *h;

    //    fprintf(stderr, "%s: @@@ memory dump starts\n", timestamp());
    DEBUGMSG(DEBUG, "%s: @@@ memory dump starts\n", timestamp());
    for (h = mem; h; h = h->next) {
      //        fprintf(stderr, "%s: @@@ mem %p %5d Bytes, allocated by %s:%d @%s\n",
        DEBUGMSG(DEBUG, "%s: @@@ mem %p %5d Bytes, allocated by %s:%d @%s\n",
                timestamp(),
                (unsigned char *)h + sizeof(struct mhdr),
                h->size, h->fname, h->lineno, h->tstamp);
    }
//    fprintf(stderr, "%s: @@@ memory dump ends\n", timestamp());
    DEBUGMSG(DEBUG, "%s: @@@ memory dump ends\n", timestamp());
}
#endif //USE_DEBUG_MALLOC

#endif //CCNL_EXT_LOGGING_C
//eof
