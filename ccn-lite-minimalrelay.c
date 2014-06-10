/*
 * @f ccn-lite-minimalrelay.c
 * @b user space CCN relay, minimalist version
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
 * 2012-08-02 created
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#if !(defined(_BSD_SOURCE) && defined(SVID_SOURCE))
int inet_aton(const char *cp, struct in_addr *inp);
#endif

// ----------------------------------------------------------------------

#define USE_SUITE_CCNB
#define USE_SUITE_NDNTLV

#define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)
#define DEBUGSTMT(LVL, ...)		do {} while(0)
#define ccnl_prefix_to_path(p) 		"null"


#define ccnl_malloc(s)			malloc(s)
#define ccnl_calloc(n,s) 		calloc(n,s)
#define ccnl_realloc(p,s)		realloc(p,s)
#define ccnl_free(p)			free(p)

#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)   ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);

#define free_prefix(p)	do{ if(p) \
			free_4ptr_list(p->path,p->comp,p->complen,p); } while(0)
#define free_content(c) do{ free_prefix(c->name); \
			free_2ptr_list(c->pkt, c); } while(0)

#define ccnl_addr2ascii(sup)		inet_ntoa((sup)->ip4.sin_addr)

#define ccnl_frag_new(a,b)			NULL
#define ccnl_frag_destroy(e)			do {} while(0)

// #define ccnl_face_CTS_done		NULL
#define ccnl_sched_destroy(s)		do {} while(0)

#define ccnl_mgmt(r,b,p,f)		-1

#define ccnl_print_stats(x,y)		do{}while(0)
#define ccnl_app_RX(x,y)		do{}while(0)

#define ccnl_ll_TX(r,i,a,b)		sendto(i->sock,b->data,b->datalen,r?0:0,(struct sockaddr*)&(a)->ip4,sizeof(struct sockaddr_in))
#define ccnl_close_socket(s)		close(s)


#include "pkt-ccnb.h"
#include "pkt-ndntlv.h"
#include "ccnl.h"
#include "ccnl-core.h"

#define compute_ccnx_digest(b) NULL

// ----------------------------------------------------------------------

int debug_level;

void
ccnl_get_timeval(struct timeval *tv)
{
    gettimeofday(tv, NULL);
}

long
timevaldelta(struct timeval *a, struct timeval *b) {
    return 1000000*(a->tv_sec - b->tv_sec) + a->tv_usec - b->tv_usec;
}

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

double
CCNL_NOW()
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

struct timeval*
ccnl_run_events()
{
    static struct timeval now;
    long usec;

    gettimeofday(&now, 0);
    while (eventqueue) {
	struct ccnl_timer_s *t = eventqueue;
	usec = timevaldelta(&(t->timeout), &now);
	if (usec >= 0) {
	    now.tv_sec = usec / 1000000;
	    now.tv_usec = usec % 1000000;
	    return &now;
	}
	else if (t->fct2)
	    (t->fct2)(t->aux1, t->aux2);
	eventqueue = t->next;
	ccnl_free(t);
    }
    return NULL;
}

static struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(*pr));

    if (!pr)
        return NULL;
    pr->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    pr->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    pr->path = (unsigned char*) ccnl_malloc(strlen(path)+1);
    if (!pr->comp || !pr->complen || !pr->path) {
        ccnl_free(pr->comp);
        ccnl_free(pr->complen);
        ccnl_free(pr->path);
        ccnl_free(pr);
        return NULL;
    }

    strcpy((char*) pr->path, path);
    cp = (char*) pr->path;
    for (path = strtok(cp, "/");
		 path && pr->compcnt < CCNL_MAX_NAME_COMP;
		 path = strtok(NULL, "/")) {
        pr->comp[pr->compcnt] = (unsigned char*) path;
        pr->complen[pr->compcnt] = strlen(path);
        pr->compcnt++;
    }
    return pr;
}

// ----------------------------------------------------------------------

#include "ccnl-core.c"

// ----------------------------------------------------------------------

struct ccnl_relay_s theRelay;

int
ccnl_open_udpdev(int port)
{
    int s;
    struct sockaddr_in si;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	perror("udp socket");
	return -1;
    }

    si.sin_addr.s_addr = INADDR_ANY;
    si.sin_port = htons(port);
    si.sin_family = PF_INET;
    if (bind(s, (struct sockaddr *)&si, sizeof(si)) < 0) {
        perror("udp sock bind");
	return -1;
    }

    return s;
}

void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

void
ccnl_io_loop(struct ccnl_relay_s *ccnl)
{
    int i, maxfd = -1, rc;
    fd_set readfs, writefs;
    
    if (ccnl->ifcount == 0) {
	fprintf(stderr, "no socket to work with, not good, quitting\n");
	exit(EXIT_FAILURE);
    }
    for (i = 0; i < ccnl->ifcount; i++)
	if (ccnl->ifs[i].sock > maxfd)
	    maxfd = ccnl->ifs[i].sock;
    maxfd++;

    FD_ZERO(&readfs);
    FD_ZERO(&writefs);
    while(!ccnl->halt_flag) {
	struct timeval *timeout;

	for (i = 0; i < ccnl->ifcount; i++) {
	    FD_SET(ccnl->ifs[i].sock, &readfs);
	    if (ccnl->ifs[i].qlen > 0)
		FD_SET(ccnl->ifs[i].sock, &writefs);
	    else
		FD_CLR(ccnl->ifs[i].sock, &writefs);
	}

	timeout = ccnl_run_events();
	rc = select(maxfd, &readfs, &writefs, NULL, timeout);
	if (rc < 0) {
	    perror("select(): ");
	    exit(EXIT_FAILURE);
	}

	for (i = 0; i < ccnl->ifcount; i++) {
	    if (FD_ISSET(ccnl->ifs[i].sock, &readfs)) {
		sockunion src_addr;
		socklen_t addrlen = sizeof(sockunion);
		unsigned char buf[CCNL_MAX_PACKET_SIZE];
		int len;
		if ((len = recvfrom(ccnl->ifs[i].sock, buf, sizeof(buf), 0,
				(struct sockaddr*) &src_addr, &addrlen)) > 0)
		    ccnl_core_RX(ccnl, i, buf, len, &src_addr.sa, sizeof(src_addr.ip4));
	    }
	    if (FD_ISSET(ccnl->ifs[i].sock, &writefs))
		ccnl_interface_CTS(&theRelay, &theRelay.ifs[0]);
	}
    }
}

// ----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int opt;
    int udpport = CCN_UDP_PORT;
    char *prefix, *defaultgw;
    struct ccnl_if_s *i;
    struct ccnl_forward_s *fwd;
    sockunion sun;

    srandom(time(NULL));

    while ((opt = getopt(argc, argv, "hs:u:v:")) != -1) {
        switch (opt) {
        case 's':
	    opt = atoi(optarg);
	    if (opt >= 0 && opt <= 2)
		theRelay.suite = opt;
            break;
        case 'u':
            udpport = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
	    break;
        case 'h':
        default:
            fprintf(stderr,
		    "usage:\n%s [-h] [-s SUITE] [-u udpport] [-v debuglevel] PREFIX DGWIP/DGWPORT\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if ((optind+1) >= argc) {
	fprintf(stderr, "Expected two arguments (e.g. /a/b/c 1.2.3.4/9695) after options\n");
	exit(EXIT_FAILURE);
    }
    prefix = argv[optind];
    defaultgw = argv[optind+1];
#ifdef USE_SUITE_CCNB
    if (theRelay.suite == CCNL_SUITE_CCNB && !udpport)
	udpport = CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_NDNTLV
    if (theRelay.suite == CCNL_SUITE_NDNTLV && !udpport)
	udpport = NDN_UDP_PORT;
#endif

    i = &theRelay.ifs[0];
    i->mtu = CCN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->sock = ccnl_open_udpdev(udpport);
    if (i->sock < 0)
	exit(-1);
    theRelay.ifcount++;

    inet_aton(strtok(defaultgw,"/"), &sun.ip4.sin_addr);
    sun.ip4.sin_port = atoi(strtok(NULL, ""));
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    fwd->prefix = ccnl_path_to_prefix(prefix);
    fwd->face = ccnl_get_face_or_create(&theRelay, 0, &sun.sa, sizeof(sun.ip4));
    fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;
    theRelay.fib = fwd;

    ccnl_set_timer(1000000, ccnl_ageing, &theRelay, 0);
    ccnl_io_loop(&theRelay);

    return 0;
}


// eof
