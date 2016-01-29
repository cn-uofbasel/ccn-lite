/*
 * @f ccnl-os-time.c
 * @b routines for uniform time handling
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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
 * 2012-11 created
 * 2013-03-18 updated (ms): removed omnet related code, and moved struct
 *   ccnl_timer_s to the '#ifndef CCNL_LINUXKERNEL' section
 */

//#include "ccnl-headers.h"
// ----------------------------------------------------------------------
#ifdef CCNL_ARDUINO


// typedef int time_t;
#define Hz 1000

double CCNL_NOW(void) { return (double) millis() / Hz; }

struct timeval {
    uint32_t tv_sec;
    uint32_t tv_usec;
};

void
gettimeofday(struct timeval *tv, void *dummy)
{
    uint32_t t = millis();
    tv->tv_sec = t / Hz;
    tv->tv_usec = (t % Hz) * (1000000 / Hz);
}

char*
timestamp(void)
{
    static char ts[12];
    uint32_t m = millis();

    sprintf_P(ts, PSTR("%lu.%03d"), m / 1000, (int)(m % 1000));

    return ts;
}

#else // !CCNL_ARDUINO

#define CCNL_NOW()                    current_time()

#ifndef CCNL_LINUXKERNEL
double
current_time(void)
{
    struct timeval tv;
    static time_t start;
    static time_t start_usec;

    gettimeofday(&tv, NULL);

    if (!start) {
        start = tv.tv_sec;
        start_usec = tv.tv_usec;
    }

    return (double)(tv.tv_sec) - start +
                ((double)(tv.tv_usec) - start_usec) / 1000000;
}

char*
timestamp(void)
{
    static char ts[16], *cp;

    sprintf(ts, "%.4g", CCNL_NOW());
    cp = strchr(ts, '.');
    if (!cp)
        strcat(ts, ".0000");
    else if (strlen(cp) > 5)
        cp[5] = '\0';
    else while (strlen(cp) < 5)
        strcat(cp, "0");

    return ts;
}

#endif

#endif // !CCNL_ARDUINO


// ----------------------------------------------------------------------
#ifdef CCNL_UNIX

#ifndef CCNL_OMNET
#  define CCNL_NOW()                    current_time()
#endif

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

struct ccnl_timer_s *eventqueue;

void
ccnl_get_timeval(struct timeval *tv)
{
    gettimeofday(tv, NULL);
}

long
timevaldelta(struct timeval *a, struct timeval *b) {
    return 1000000*(a->tv_sec - b->tv_sec) + a->tv_usec - b->tv_usec;
}

void*
ccnl_set_timer(uint64_t usec, void (*fct)(void *aux1, void *aux2),
                 void *aux1, void *aux2)
{
    struct ccnl_timer_s *t, **pp;
    //    static int handlercnt;

    t = (struct ccnl_timer_s *) ccnl_calloc(1, sizeof(*t));
    if (!t)
        return 0;
    t->fct2 = fct;
    gettimeofday(&t->timeout, NULL);
    usec += t->timeout.tv_usec;
    t->timeout.tv_sec += usec / 1000000;
    t->timeout.tv_usec = usec % 1000000;
    t->aux1 = aux1;
    t->aux2 = aux2;

    for (pp = &eventqueue; ; pp = &((*pp)->next)) {
    if (!*pp || (*pp)->timeout.tv_sec > t->timeout.tv_sec ||
        ((*pp)->timeout.tv_sec == t->timeout.tv_sec &&
         (*pp)->timeout.tv_usec > t->timeout.tv_usec)) {
        t->next = *pp;
        //        t->handler = handlercnt++;
        *pp = t;
        return t;
    }
    }
    return NULL; // ?
}

void
ccnl_rem_timer(void *h)
{
    struct ccnl_timer_s **pp;

    for (pp = &eventqueue; *pp; pp = &((*pp)->next)) {
        if ((void*)*pp == h) {
            struct ccnl_timer_s *e = *pp;
            *pp = e->next;
            ccnl_free(e);
            break;
        }
    }
}

#endif

// ----------------------------------------------------------------------

#ifdef CCNL_LINUXKERNEL

// we use the kernel's timerlist service, i.e. add_timer()

struct ccnl_timerlist_s {
    struct timer_list tl;
    void (*fct)(void *ptr, void *aux);
    void *ptr, *aux;
};

static struct ccnl_timerlist_s *spare_timer;

inline void
ccnl_get_timeval(struct timeval *tv)
{
    jiffies_to_timeval(jiffies, tv);
}

int
current_time(void)
{
    struct timeval tv;

    ccnl_get_timeval(&tv);
    return tv.tv_sec;
}

static void
ccnl_timer_callback(unsigned long data)
{
    struct ccnl_timerlist_s *t = (struct ccnl_timerlist_s*) data;
    void (*fct)(void*,void*) = t->fct;
    void *ptr = t->ptr, *aux = t->aux;

    if (!spare_timer)
        spare_timer = t;
    else
        kfree(t);
    (fct)(ptr, aux);
}

static void*
ccnl_set_timer(int usec, void(*fct)(void*,void*), void *ptr, void *aux)
{
    struct ccnl_timerlist_s *t;

    if (spare_timer) {
        t = spare_timer;
        spare_timer = NULL;
    } else {
        t = kmalloc(sizeof(struct ccnl_timerlist_s), GFP_ATOMIC);
        if (!t)
            return NULL;
    }
    init_timer(&t->tl);
    t->tl.function = ccnl_timer_callback;
    t->tl.data = (unsigned long) t;
    t->fct = fct;
    t->ptr = ptr;
    t->aux = aux;
    t->tl.expires = jiffies + usecs_to_jiffies(usec);
    add_timer(&t->tl);
    return t;
}

static void
ccnl_rem_timer(void *p)
{
    struct ccnl_timerlist_s *t = (struct ccnl_timerlist_s*) p;

    if (!p)
        return;
    del_timer(&t->tl);
    if (!spare_timer)
        spare_timer = t;
    else
        kfree(t);
}

#else // we have to work through the pending timer requests ourself

// "looper": serves all pending events and returns the number of microseconds
// when the next event should be triggered.
int
ccnl_run_events(void)
{
    static struct timeval now;
    long usec;

    gettimeofday(&now, 0);
    while (eventqueue) {
        struct ccnl_timer_s *t = eventqueue;

        usec = timevaldelta(&(t->timeout), &now);
        if (usec >= 0)
            return usec;

        if (t->fct)
            (t->fct)(t->node, t->intarg);
        else if (t->fct2)
            (t->fct2)(t->aux1, t->aux2);
        eventqueue = t->next;
        ccnl_free(t);
    }

    return -1;
}

#endif // CCNL_LINUXKERNEL

// ----------------------------------------------------------------------

#ifdef USE_SCHEDULER

void*
ccnl_set_absolute_timer(struct timeval abstime, void (*fct)(void *aux1, void *aux2),
         void *aux1, void *aux2)
{
    struct ccnl_timer_s *t, **pp;
    //    static int handlercnt;

    t = (struct ccnl_timer_s *) ccnl_calloc(1, sizeof(*t));
    if (!t)
    return 0;
    t->fct2 = fct;
    t->timeout = abstime;
    t->aux1 = aux1;
    t->aux2 = aux2;

    for (pp = &eventqueue; ; pp = &((*pp)->next)) {
    if (!*pp || (*pp)->timeout.tv_sec > t->timeout.tv_sec ||
        ((*pp)->timeout.tv_sec == t->timeout.tv_sec &&
         (*pp)->timeout.tv_usec > t->timeout.tv_usec)) {
        t->next = *pp;
        //        t->handler = handlercnt++;
        *pp = t;
        return t;
    }
    }
    return NULL; // ?
}

#endif

// eof
