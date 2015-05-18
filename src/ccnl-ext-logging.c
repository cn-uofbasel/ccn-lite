/*
 * @f ccnl-ext-logging.c
 * @b CCNL logging 
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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

#ifdef USE_LOGGING

extern int debug_level;

#define FATAL   0  // FATAL
#define ERROR   1  // ERROR
#define WARNING 2  // WARNING 
#define INFO    3  // INFO 
#define DEBUG   4  // DEBUG 
#define VERBOSE 5  // VERBOSE
#define TRACE 	6  // TRACE 

char
ccnl_debugLevelToChar(int level)
{
#ifndef CCNL_ARDUINO
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

#ifdef CCNL_ARDUINO

#define _TRACE(F,P) do { \
     if (debug_level >= TRACE) {                     \
          Serial.print("[");            \
          Serial.print(P); \
          Serial.print("] ");           \
          Serial.print(timestamp());    \
          Serial.print(": ");            \
          strcpy_P(logstr, PSTR(__FILE__) + 8);     \
          Serial.print(logstr);            \
          Serial.print(":");            \
          Serial.print(__LINE__);            \
          Serial.println("\r");           \
     }} while(0)

#else

#define _TRACE(F,P) if (debug_level >= TRACE)                            \
                      fprintf(stderr, "[%c] %s: %s() in %s:%d\n",        \
                              (P), timestamp(), (F), __FILE__, __LINE__)

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

static char android_logstr[1024];
void jni_append_to_log(char *line);

#  define DEBUGMSG(LVL, ...) do { int len;          \
        if ((LVL)>debug_level) break;               \
        len = sprintf(android_logstr, "[%c] %s: ",  \
            ccnl_debugLevelToChar(LVL),             \
            timestamp());                           \
        len += sprintf(android_logstr+len, __VA_ARGS__);   \
        if (android_logstr[len - 1] == '\n')       \
            android_logstr[len - 1] = '\0';         \
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

X

#  define DEBUGMSG(LVL, ...) do {                   \
        if ((LVL)>debug_level) break;               \
        fprintf(stderr, "[%c] %s: ",                \
            ccnl_debugLevelToChar(LVL),             \
            timestamp());                           \
        fprintf(stderr, __VA_ARGS__);               \
    } while (0)

#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define TRACEIN(F)    do { _TRACE(__func__, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE(__func__, '<'); } while (0)
#else
#  define TRACEIN(F)    do { _TRACE("" F, '>'); } while (0)
#  define TRACEOUT(F)   do { _TRACE("" F, '<'); } while (0)
#endif

// ----------------------------------------------------------------------

#else // !USE_LOGGING
#  define DEBUGSTMT(...)                   do {} while(0)
#  define DEBUGMSG(...)                    do {} while(0)
#  define DEBUGMSG_ON(...)                 do {} while(0)
#  define DEBUGMSG_OFF(...)                do {} while(0)
#  define TRACEIN(...)                     do {} while(0)
#  define TRACEOUT(...)                    do {} while(0)

#endif // USE_LOGGING

#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC

void
debug_memdump()
{
    struct mhdr *h;

    CONSOLE("%s @@@ memory dump starts\n", timestamp());
    for (h = mem; h; h = h->next) {
#ifdef CCNL_ARDUINO
        char *cp = logstr;
        sprintf_P(logstr, PSTR("%s @@@ mem %p %5d Bytes, allocated by "),
                  timestamp(),
                  (int)((unsigned char *)h + sizeof(struct mhdr)),
                  h->size);
        cp += strlen(logstr);
        strcpy_P(cp, h->fname + 7); // remove the "src/../" prefix
        Serial.print(logstr);
        CONSOLE(":%d @%d.%03d\n", h->lineno,
                int(h->tstamp), int(1000*(h->tstamp - int(h->tstamp))));
#else
        CONSOLE("%s @@@ mem %p %5d Bytes, allocated by %s:%d @%s\n",
                timestamp(),
                (unsigned char *)h + sizeof(struct mhdr),
                h->size, h->fname, h->lineno, h->tstamp);
#endif
    }
    CONSOLE("%s @@@ memory dump ends\n", timestamp());
}
#endif //USE_DEBUG_MALLOC
#endif //USE_DEBUG


// only in the Ardiono case we wish to control debugging on a module basis
#ifndef CCNL_ARDUINO
# define DEBUGMSG_CORE(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CFWD(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CUTL(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PCNX(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PIOT(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PNDN(...) DEBUGMSG(__VA_ARGS__)
#endif

//eof
