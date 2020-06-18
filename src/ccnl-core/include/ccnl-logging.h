/*
 * @f ccnl-ext-logging.h
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

#ifndef CCNL_LOGGING_H
#define CCNL_LOGGING_H

#ifdef DEBUG
#undef DEBUG
#endif

#ifndef CCNL_RIOT
#define FATAL   0  // FATAL
#define ERROR   1  // ERROR
#define WARNING 2  // WARNING
#define INFO    3  // INFO
#define DEBUG   4  // DEBUG
#define VERBOSE 5  // VERBOSE
#define TRACE 	6  // TRACE
#else
#include "ccnl-riot-logging.h"
#endif

#ifdef USE_LOGGING
#ifndef CCNL_LINUXKERNEL
#include "ccnl-malloc.h"
#include <stdio.h>
#endif
//#include "ccnl-os-time.h"

extern int debug_level;

char
ccnl_debugLevelToChar(int level);

// ----------------------------------------------------------------------
// _TRACE macro

#ifdef CCNL_ARDUINO

#define _TRACE(F,P) do {                    \
    if (debug_level >= TRACE) { char *cp;   \
          Serial.print("[");                \
          Serial.print(P); \
          Serial.print("] ");               \
          Serial.print(timestamp());        \
          Serial.print(": ");               \
          strcpy_P(logstr, PSTR(__FILE__)); \
          cp = logstr + strlen(logstr);     \
          while (cp >= logstr && *cp != '/') cp--; \
          Serial.print(cp+1);               \
          Serial.print(":");                \
          Serial.print(__LINE__);           \
          Serial.println("\r");             \
     }} while(0)

#else

#ifdef CCNL_LINUXKERNEL

#define _TRACE(F,P) do {                                    \
    if (debug_level >= TRACE) {                             \
        printk("%s: ", THIS_MODULE->name);                  \
        printk("%s() in %s:%d\n", (F), __FILE__, __LINE__); \
    }} while (0)

#else

#define _TRACE(F,P) do {                                    \
    if (debug_level >= TRACE) {                             \
        fprintf(stderr, "[%c] %s: %s() in %s:%d\n",         \
                (P), timestamp(), (F), __FILE__, __LINE__); \
    }} while (0)

#endif // CCNL_LINUXKERNEL

int
ccnl_debug_str2level(char *s);

#endif // CCNL_ARDUINO

#define DEBUGSTMT(LVL, ...) do { \
        if ((LVL)>debug_level) break; \
        __VA_ARGS__; \
} while (0)

// ----------------------------------------------------------------------
// DEBUGMSG macro

#ifdef CCNL_LINUXKERNEL

#  define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        printk("%s: ", THIS_MODULE->name);      \
        printk(__VA_ARGS__);            \
    } while (0)
#  define fprintf(fd, ...)      printk(__VA_ARGS__)

#elif defined(CCNL_ANDROID)

#  define DEBUGMSG(LVL, ...) do { int len;          \
        if ((LVL)>debug_level) break;               \
        len = snprintf(android_logstr, sizeof(android_logstr), "[%c] %s: ",  \
            ccnl_debugLevelToChar(LVL),             \
            timestamp());                           \
        len += snprintf(android_logstr+len, sizeof(android_logstr)-len, __VA_ARGS__);   \
        jni_append_to_log(android_logstr);          \
    } while (0)

#elif defined(CCNL_ARDUINO)

#  define DEBUGMSG_OFF(...) do{}while(0)
#  define DEBUGMSG_ON(L,FMT, ...) do {     \
        if ((L) <= debug_level) {       \
          Serial.print("[");            \
          Serial.print(ccnl_debugLevelToChar(debug_level)); \
          Serial.print("] ");           \
          sprintf_P(logstr, PSTR(FMT), ##__VA_ARGS__); \
          Serial.print(timestamp());    \
          Serial.print(" ");            \
          Serial.print(logstr);         \
          Serial.print("\r");           \
        }                               \
   } while(0)

#else
#ifndef CCNL_RIOT
#  define DEBUGMSG(LVL, ...) do {                   \
        if ((LVL)>debug_level) break;               \
        fprintf(stderr, "[%c] %s: ",                \
            ccnl_debugLevelToChar(LVL),             \
            timestamp());                           \
        fprintf(stderr, __VA_ARGS__);               \
    } while (0)
#endif
#endif
#ifndef CCNL_RIOT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define TRACEIN(F)    do { _TRACE(__func__, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE(__func__, '<'); } while (0)
#else
#  define TRACEIN(F)    do { _TRACE("" F, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE("" F, '<'); } while (0)
#endif
#endif

// ----------------------------------------------------------------------

#else // !USE_LOGGING
#ifndef CCNL_RIOT
#  define DEBUGSTMT(...)                   do {} while(0)
#  define DEBUGMSG(...)                    do {} while(0)
#  define DEBUGMSG_ON(...)                 do {} while(0)
#  define DEBUGMSG_OFF(...)                do {} while(0)
#  define TRACEIN(...)                     do {} while(0)
#  define TRACEOUT(...)                    do {} while(0)
#endif
#endif // USE_LOGGING

#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC

char *
getBaseName(char *fn);

void
debug_memdump(void);

#endif //USE_DEBUG_MALLOC
#endif //USE_DEBUG


// only in the Arduino case we wish to control debugging on a module basis
#ifndef CCNL_ARDUINO
// core source files
# define DEBUGMSG_CORE(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CFWD(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CUTL(...) DEBUGMSG(__VA_ARGS__)
// extensions
# define DEBUGMSG_EFRA(...) DEBUGMSG(__VA_ARGS__)
// packet formats
# define DEBUGMSG_PCNX(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PIOT(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PNDN(...) DEBUGMSG(__VA_ARGS__)
#endif

#ifdef CCNL_ARDUINO
#  define CONSOLE(FMT, ...)   do { \
     sprintf_P(logstr, PSTR(FMT), ##__VA_ARGS__); \
     Serial.print(logstr); \
     Serial.print("\r"); \
   } while(0)
#  define CONSTSTR(s)         ccnl_arduino_getPROGMEMstr(PSTR(s))

#elif defined(CCNL_ANDROID)

static char android_logstr[1024];
void jni_append_to_log(char *line);

#  define CONSOLE(...)        do { snprintf(android_logstr, sizeof(android_logstr), __VA_ARGS__); \
                                   jni_append_to_log(android_logstr); \
                              } while(0)
#  define CONSTSTR(s)         s

#elif !defined(CCNL_LINUXKERNEL)

#  define CONSOLE(...)        fprintf(stderr, __VA_ARGS__)
#  define CONSTSTR(s)         s

#else // CCNL_LINUXKERNEL

#  define CONSOLE(FMT, ...)   printk(FMT, ##__VA_ARGS__)
#  define CONSTSTR(s)         s

#endif

#endif // CCNL_LOGGING_H
