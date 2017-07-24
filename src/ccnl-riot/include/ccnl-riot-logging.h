/*
 * @file ccnl-riot-logging.h
 *
 * @copyright Copyright (C) 2011-18, University of Basel
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CCNL_RIOT_LOGGING
#define CCNL_RIOT_LOGGING

/**
 * Define the log level of the CCN-Lite stack
 */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_DEBUG
#endif

#include "log.h"

 /**
 * Closing an interface or socket from CCN-Lite
 */
#define ccnl_close_socket(s)            close(s)

/**
 * @name Log levels used by CCN-Lite debugging
 *
 * @{
 */
#define FATAL   LOG_ERROR
#define ERROR   LOG_ERROR
#define WARNING LOG_WARNING
#define INFO    LOG_INFO
#define DEBUG   LOG_DEBUG
#define TRACE   LOG_DEBUG
#define VERBOSE LOG_ALL
/**
 * @}
 */

/**
 * @name CCN-Lite's debugging macros
 *
 * @{
 */
#define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        LOG(LVL, __VA_ARGS__);   \
    } while (0)

#define DEBUGMSG_CORE(...) DEBUGMSG(__VA_ARGS__)
#define DEBUGMSG_CFWD(...) DEBUGMSG(__VA_ARGS__)
#define DEBUGMSG_CUTL(...) DEBUGMSG(__VA_ARGS__)
#define DEBUGMSG_PIOT(...) DEBUGMSG(__VA_ARGS__)

#define DEBUGSTMT(LVL, ...) do { \
        if ((LVL)>debug_level) break; \
        __VA_ARGS__; \
     } while (0)

#define TRACEIN(...)                    do {} while(0)
#define TRACEOUT(...)                   do {} while(0)
/**
 * @}
 */

 #endif
