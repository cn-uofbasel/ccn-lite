/*
 * @f ccnl-ext-logging.c
 * @b CCNL logging
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
 * 2017-06-16 created <christopher.scherb@unibas.ch>
 */

#ifndef CCNL_LINUXKERNEL
#include "ccnl-logging.h"
#include <string.h>
#include <stdio.h>
#else
#include "../include/ccnl-logging.h"
#endif

int debug_level;

char
ccnl_debugLevelToChar(int level)
{
#if !defined(CCNL_ARDUINO) && !defined(CCNL_RIOT)
    switch (level) {
        case FATAL:     return 'F';
        case ERROR:     return 'E';
        case WARNING:   return 'W';
        case INFO:      return 'I';
        case DEBUG:     return 'D';
        case VERBOSE:   return 'V';
        case TRACE:     return 'T';
        default:        return '?';
    }
#else // gcc-avr creates a lookup table (in the data section) which
      // we want to avoid. To force PROGMEM, we have to code :-(
    if (level == FATAL)
        return 'F';
    if (level == ERROR)
        return 'E';
    if (level == WARNING)
        return 'W';
    if (level == INFO)
        return 'I';
    if (level == DEBUG)
        return 'D';
    if (level == VERBOSE)
        return 'V';
    if (level == TRACE)
        return 'T';
    return '?';
#endif
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

#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC

char *
getBaseName(char *fn)
{
    char *cp = fn + strlen(fn);

    for (cp--; cp >= fn; cp--)
        if (*cp == '/')
            break;

    return cp+1;
}

void
debug_memdump(void)
{
    struct mhdr *h;

    CONSOLE("[M] %s: @@@ memory dump starts\n", timestamp());
    for (h = mem; h; h = h->next) {
#ifdef CCNL_ARDUINO
        sprintf_P(logstr, PSTR("addr %p %5d Bytes, "),
                  (int)((unsigned char *)h + sizeof(struct mhdr)),
                  h->size);
        Serial.print(logstr);
        // remove the "src/../" prefix:
        strcpy_P(logstr, h->fname);
        Serial.print(getBaseName(logstr));
        CONSOLE(":%d @%d.%03d\n", h->lineno,
                int(h->tstamp), int(1000*(h->tstamp - int(h->tstamp))));
#else
        CONSOLE("addr %p %lu Bytes, %s:%d @%s\n",
                (void *)(h + sizeof(struct mhdr)),
                h->size, getBaseName(h->fname), h->lineno, h->tstamp);
#endif
    }
    CONSOLE("[M] %s: @@@ memory dump ends\n", timestamp());
}
#endif //USE_DEBUG_MALLOC
#endif //USE_DEBUG
