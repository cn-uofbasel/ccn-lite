/*
 * @f ccnl-platform.c
 * @b support functions for providing a uniform environment on each platform
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 * 2011-12 created
 * 2013-03-18 updated (ms): removed omnet related code, and moved struct
 *   ccnl_timer_s to the '#ifndef CCNL_KERNEL' section
 */

// ----------------------------------------------------------------------

long
timevaldelta(struct timeval *a, struct timeval *b) {
    return 1000000*(a->tv_sec - b->tv_sec) + a->tv_usec - b->tv_usec;
}


#ifdef CCNL_KERNEL

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

struct ccnl_timerlist_s {
    struct timer_list tl;
    void (*fct)(void *ptr, void *aux);
    void *ptr, *aux;
};
static struct ccnl_timerlist_s *spare_timer;

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

#else // !CCNL_KERNEL

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

// ----------------------------------------------------------------------
// (ms) The following code is only used in omnet so I take care of it
// in the container ccn-lite-omnet.c
/*
#ifdef CCNL_OMNET

void
ccnl_get_timeval(struct timeval *tv)
{
    double now = CCNL_NOW();
    tv->tv_sec = now;
    tv->tv_usec = 1000000 * (now - tv->tv_sec);
}

int
ccnl_set_timer(int usec, void (*fct)(char node, int aux),
		 char node, int aux)
{
    struct ccnl_timer_s *t, **pp;
    static int handlercnt;

    t = (struct ccnl_timer_s *) ccnl_calloc(1, sizeof(*t));
    if (!t)
	return -1;
    t->fct = fct;
    gettimeofday(&t->timeout, NULL);
    usec += t->timeout.tv_usec;
    t->timeout.tv_sec += usec / 1000000;
    t->timeout.tv_usec = usec % 1000000;
    t->node = node;
    t->aux = aux;

    for (pp = &events; ; pp = &((*pp)->next)) {
	if (!*pp || (*pp)->timeout.tv_sec > t->timeout.tv_sec ||
	    ((*pp)->timeout.tv_sec == t->timeout.tv_sec &&
	     (*pp)->timeout.tv_usec > t->timeout.tv_usec)) {
	    t->next = *pp;
	    t->handler = handlercnt++;
	    *pp = t;
	    return t->handler;
	}
    }
}

void
ccnl_rem_timer(int h)
{
    struct ccnl_timer_s **pp;

    DEBUGMSG(99, "removing time handler %d\n", h);

    for (pp = &events; *pp && (*pp)->handler != h; pp = &((*pp)->next));
    if (*pp) {
	struct ccnl_timer_s *e = *pp;
	*pp = e->next;
	ccnl_free(e);
    }
}

#endif // CCNL_OMNET
*/

// ----------------------------------------------------------------------

#if defined(CCNL_UNIX) || defined(CCNL_SIMULATION)

void
ccnl_get_timeval(struct timeval *tv)
{
    gettimeofday(tv, NULL);
}

struct ccnl_timer_s;    // (ms) moved actual definition further down

struct ccnl_timer_s *eventqueue;

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


// ----------------------------------------------------------------------




#ifdef XXX

void
simu_do_ageing(char node, int dummy)
{
    struct ccnl_s *relay = char2relay(node);
    int usec;

    DEBUGMSG(99, "do_aging for node %c\n", node);

    usec = ccnl_do_ageing(relay, 0);
    if (usec > 0)
      wowmom_set_timer(usec, simu_do_ageing, node, dummy);
}



void
ccnl_wowmom_add_fwd(char node, const char *name, char *dstaddr)
{
    struct ccnl_forward_s *fwd;
    struct ccnl_s *relay;
    sockunion sun;

    DEBUGMSG(99, "ccnl_wowmom_add_fwd\n");

    relay = char2relay(node);
    if (!relay) return;

    sun.eth.sll_family = AF_PACKET;
    memcpy(sun.eth.sll_addr, dstaddr, ETH_ALEN);
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    fwd->prefix = ccnl_path_to_prefix(name);
    fwd->face = ccnl_get_face_or_create(relay, 0, &sun.sa, sizeof(sun.eth),
				       CCNL_DGRAM_ENCAPS_ETH2011);
    fwd->next = relay->fib;
    relay->fib = fwd;
}

void
wowmom_fini_relay(struct ccnl_s *ccnl)
{
    struct ccnl_face_s *f, *f2;
    struct ccnl_forward_s *fw, *fw2;
    struct ccnl_interest_s *i, *i2;
    int k;

    DEBUGMSG(99, "wowmom_fini_relay %p()\n", (void *) ccnl);

    wowmom_client_endlog(relay2char(ccnl), 0);

    for (fw = ccnl->fib; fw;) {
	ccnl_prefix_free(fw->prefix);
	fw2 = fw->next;
	ccnl_free(fw);
	fw = fw2;
    }

    for (f = ccnl->faces; f;) {
	struct ccnl_buf_s *b, *b2;
	for (b = f->outq; b; ) {
	    b2 = b->next;
	    ccnl_free(b);
	    b = b2;
	}
	f2 = f->next;
	ccnl_free(f);
	f = f2;
    }

    for (i = ccnl->pit; i;) {
	struct ccnl_pendint_s *pi, *pi2;
	for (pi = i->pend; pi; ) {
	    pi2 = pi->next;
	    ccnl_free(pi);
	    pi = pi2;
	}
	ccnl_prefix_free(i->prefix);
	ccnl_free(i->nonce);
	ccnl_free(i->ppkd);
	ccnl_free(i->data);
	i2 = i->next;
	ccnl_free(i);
	i = i2;
    }

    while (ccnl->contents)
	ccnl_content_remove(ccnl, ccnl->contents);

    for (k = 0; k < ccnl->ifcount; k++) {
	struct ccnl_if_s *ifs = ccnl->ifs + k;
	struct ccnl_buf_s *b, *b2;

	for (b = ifs->sendbuf; b;) {
	    b2 = b->next;
	    ccnl_free(b);
	    b = b2;
	}
//	ccnl_free(ifs->sendface);
	ccnl_free(ifs->sched);
    }

#if defined(WOWMOM) && !defined(CCNL_SIMU_NET_H)
    if (ccnl->simu) {
	ccnl_free(ccnl->simu);
	ccnl->simu = NULL;
    }
    if (ccnl->stats) {
	fclose(ccnl->stats->ofp);
	ccnl_free(ccnl->stats);
	ccnl->stats = NULL;
    }
#endif
}

void 
wowmom_node_log_start(struct ccnl_s *relay, char node)
{
    // write the node-log here
    char name[100];
    struct ccnl_stats_s *st = relay->stats;

    if (!st)
	return;

    sprintf(name, "node-%c-sent.log", node);
    st->ofp_s = fopen(name, "w");
    if (st->ofp_s) {
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# sent_log Node %c\n", node);
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# time\t sent i\t\t sent c\t\t sent i+c\n");
	fflush(st->ofp_s);
    }
    sprintf(name, "node-%c-rcvd.log", node);
    st->ofp_r = fopen(name, "w");
    if (st->ofp_r) {
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# rcvd_log Node %c\n", node);
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# time\t recv i\t\t recv c\t\t recv i+c\n");
	fflush(st->ofp_r);
    }
    sprintf(name, "node-%c-qlen.log", node);
    st->ofp_q = fopen(name, "w");
    if (st->ofp_q) {
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# qlen_log Node %c (relay->ifs[0].outcnt)\n", node);
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# time\t qlen\n");
	fflush(st->ofp_q);
    }
}

#endif

// eof
