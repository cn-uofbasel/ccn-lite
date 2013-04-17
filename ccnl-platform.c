/*
 * @f ccnl-platform.c
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

long
timevaldelta(struct timeval *a, struct timeval *b) {
    return 1000000*(a->tv_sec - b->tv_sec) + a->tv_usec - b->tv_usec;
}

#ifndef CCNL_OMNET
#  define CCNL_NOW()			current_time()
#endif
// ----------------------------------------------------------------------

#ifdef CCNL_LINUXKERNEL

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

static int
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

// ----------------------------------------------------------------------
#else // !CCNL_LINUXKERNEL
// ----------------------------------------------------------------------

// (ms) I moved the following struct def here because it is used by all
// containers apart from the kernel (this way I don't need to redefine it
// for omnet.
//
struct ccnl_timer_s {
    struct ccnl_timer_s *next;
    struct timeval timeout;
    void (*fct)(char,int);
    void (*fct2)(void*,void*);
    char node;
    int intarg;
    void *aux1;
    void *aux2;
    int handler;
};


#if defined(CCNL_UNIX) || defined(CCNL_SIMULATION)

struct ccnl_timer_s *eventqueue;

void
ccnl_get_timeval(struct timeval *tv)
{
    gettimeofday(tv, NULL);
}

void*
ccnl_set_timer(int usec, void (*fct)(void *aux1, void *aux2),
		 void *aux1, void *aux2)
{
    struct ccnl_timer_s *t, **pp;
    static int handlercnt;

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
        t->handler = handlercnt++;
        *pp = t;
        return t;
    }
    }
    return NULL; // ?
}

void*
ccnl_set_absolute_timer(struct timeval abstime, void (*fct)(void *aux1, void *aux2),
         void *aux1, void *aux2)
{
    struct ccnl_timer_s *t, **pp;
    static int handlercnt;

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
        t->handler = handlercnt++;
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

    DEBUGMSG(99, "removing time handler %p\n", h);

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


double
current_time()
{
    struct timeval tv;
    static time_t start;
    static time_t start_usec;

    ccnl_get_timeval(&tv);

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
    static char ts[30], *cp;

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

// void ccnl_get_timeval(struct timeval *tv);


// eof
