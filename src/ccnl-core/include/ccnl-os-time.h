/*
 * @f ccnl-os-time.h
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

 #ifndef CCNL_OS_TIME_H
 #define CCNL_OS_TIME_H

#ifndef CCNL_LINUXKERNEL
#include <stdint.h>
#else
#include <linux/types.h>
#endif

 #ifdef CCNL_ARDUINO

 // typedef int time_t;
#define Hz 1000

double CCNL_NOW(void);

 struct timeval {
    uint32_t tv_sec;
    uint32_t tv_usec;
};

void
gettimeofday(struct timeval *tv, void *dummy);

char*
timestamp(void);

#else // !CCNL_ARDUINO
#ifndef CCNL_LINUXKERNEL
 #include <sys/time.h>
#endif

#ifndef CCNL_LINUXKERNEL
double
current_time(void);

char*
timestamp(void);

#endif

#endif // !CCNL_ARDUINO

// ----------------------------------------------------------------------
#ifdef CCNL_UNIX

#ifndef CCNL_OMNET
#  define CCNL_NOW()                    current_time()
#endif //CCNL_OMNET

#endif // CCNL_UNIX

#if defined(CCNL_UNIX) || defined (CCNL_RIOT) || defined (CCNL_ARDUINO)

// ----------------------------------------------------------------------

struct ccnl_timer_s {
    struct ccnl_timer_s *next;
    struct timeval timeout;
    void (*fct)(char,int);
    void (*fct2)(void*,void*);
    char node;
    int intarg;
    void *aux1;
    void *aux2;
  //    int handler;
};

void
ccnl_get_timeval(struct timeval *tv);

long
timevaldelta(struct timeval *a, struct timeval *b);

void*
ccnl_set_timer(uint64_t usec, void (*fct)(void *aux1, void *aux2),
                 void *aux1, void *aux2);

void
ccnl_rem_timer(void *h);

#endif

#ifdef CCNL_LINUXKERNEL
struct ccnl_timerlist_s {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
        struct legacy_timer_emu {
            struct timer_list t;
            void (*function)(unsigned long);
            unsigned long data;
    }tl;
#else
struct timer_list tl;
#endif
    void (*fct)(void *ptr, void *aux);
    void *ptr, *aux;
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);
	lt->function(lt->data);
}
#endif

static struct ccnl_timerlist_s *spare_timer;

inline void
ccnl_get_timeval(struct timeval *tv);

int
current_time2(void);

long
timevaldelta(struct timeval *a, struct timeval *b);

#  define CCNL_NOW()                    current_time2()

static void
ccnl_timer_callback(unsigned long data);

static void*
ccnl_set_timer(int usec, void(*fct)(void*,void*), void *ptr, void *aux);

static void
ccnl_rem_timer(void *p);

#else

int
ccnl_run_events(void);

#endif // CCNL_LINUXKERNEL

#ifdef USE_SCHEDULER

void*
ccnl_set_absolute_timer(struct timeval abstime, void (*fct)(void *aux1, void *aux2),
         void *aux1, void *aux2)

#endif

#endif // CCNL_OS_TIME_H
