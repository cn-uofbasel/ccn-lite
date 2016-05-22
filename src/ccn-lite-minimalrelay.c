/*
 * @f ccn-lite-minimalrelay.c
 * @b user space NDN relay, minimalist version
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
 * 2014-11-05 small code cleanups, now using NDNTLV as default encoding
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef _DEFAULT_SOURCE
int inet_aton(const char *cp, struct in_addr *inp);
#endif

// ----------------------------------------------------------------------

#undef USE_NFN

#define USE_DUP_CHECK
#define USE_IPV4
#define USE_SUITE_NDNTLV
#define NEEDS_PREFIX_MATCHING

// ----------------------------------------------------------------------
// "replacement lib"

#define FATAL   0 // FATAL
#define ERROR   1 // ERROR
#define WARNING 2 // WARNING
#define INFO    3 // INFO
#define DEBUG   4 // DEBUG
#define TRACE   5 // TRACE
#define VERBOSE 6 // VERBOSE

#define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)
# define DEBUGMSG_CORE(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CFWD(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_CUTL(...) DEBUGMSG(__VA_ARGS__)
# define DEBUGMSG_PIOT(...) DEBUGMSG(__VA_ARGS__)

#define DEBUGSTMT(LVL, ...) do { \
        if ((LVL)>debug_level) break; \
        __VA_ARGS__; \
     } while (0)

#define TRACEIN(...)                    do {} while(0)
#define TRACEOUT(...)                   do {} while(0)

#define CONSTSTR(s)                     s

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)

#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)   ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);
#define free_5ptr_list(a,b,c,d,e) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d), ccnl_free(e);

#define free_prefix(p)  do{ if(p) \
                free_5ptr_list(p->bytes,p->comp,p->complen,p->chunknum,p); } while(0)
#define free_content(c) do{ /* free_prefix(c->name); */ free_packet(c->pkt); \
                        ccnl_free(c); } while(0)

#define ccnl_frag_new(a,b)                      NULL
#define ccnl_frag_destroy(e)                    do {} while(0)

#define ccnl_sched_destroy(s)           do {} while(0)

#define ccnl_mgmt(r,b,p,f)              -1

#define ccnl_nfn_monitor(a,b,c,d,e)     do{}while(0)

#define ccnl_app_RX(x,y)                do{}while(0)

#define ccnl_close_socket(s)            close(s)

#define compute_ccnx_digest(b) NULL
#define local_producer(...)             0
#define cache_strategy_remove(...)      0

//----------------------------------------------------------------------

#include "ccnl-defs.h"
#include "ccnl-core.h"

void free_packet(struct ccnl_pkt_s *pkt);

struct ccnl_interest_s* ccnl_interest_remove(struct ccnl_relay_s *ccnl,
                     struct ccnl_interest_s *i);
int ccnl_pkt2suite(unsigned char *data, int len, int *skip);

char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr,
                    int ccntlv_skip, int escape_components, int call_slash);
#define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)

char* ccnl_addr2ascii(sockunion *su);
void ccnl_core_addToCleanup(struct ccnl_buf_s *buf);
const char* ccnl_suite2str(int suite);
bool ccnl_isSuite(int suite);

//----------------------------------------------------------------------
static inline void ccnl_ll_TX(struct ccnl_relay_s *r, struct ccnl_if_s *i,
                sockunion *a, struct ccnl_buf_s *b)
{
    sendto(i->sock,b->data,b->datalen,r?0:0,(struct sockaddr*)&(a)->ip4,sizeof(struct sockaddr_in));
}

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

// ----------------------------------------------------------------------
// timer support and event server
// copied from ccnl-os-time.c
// (because we do not want to have includes beyond the core CCN logic)

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

// ----------------------------------------------------------------------

int debug_level;
struct ccnl_relay_s theRelay;

#include "ccnl-core.c"

// ----------------------------------------------------------------------
// UDP socket, main event loop

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

void ccnl_minimalrelay_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_minimalrelay_ageing, relay, 0);
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
    int udpport = 0;
    char *prefix, *defaultgw;
    struct ccnl_if_s *i;
    struct ccnl_forward_s *fwd;
    sockunion sun;

    srandom(time(NULL));

    int suite = CCNL_SUITE_NDNTLV;

    while ((opt = getopt(argc, argv, "hs:u:v:")) != -1) {
        switch (opt) {
        case 's':
            opt = ccnl_str2suite(optarg);
            if (opt >= 0 && opt < CCNL_SUITE_LAST)
                suite = opt;
            else
                fprintf(stderr, "Suite parameter <%s> ignored.\n", optarg);
            break;
        case 'u':
            udpport = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
            break;
        case 'h':
        default:
usage:
            fprintf(stderr,
                    "usage:    %s [options] PREFIX DGWIP/DGWUDPPORT\n"
                    "options:  [-h] [-s SUITE] [-u udpport] [-v debuglevel]\n"
                    "example:  %s /ndn 128.252.153.194/6363\n",
                    argv[0], argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if ((optind+1) >= argc)
        goto usage;
    prefix = argv[optind];
    defaultgw = argv[optind+1];

    ccnl_core_init();

//    if (theRelay.suite == CCNL_SUITE_NDNTLV && !udpport)
        udpport = NDN_UDP_PORT;

    i = &theRelay.ifs[0];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->sock = ccnl_open_udpdev(udpport);
    if (i->sock < 0)
        exit(-1);
    theRelay.ifcount++;
    fprintf(stderr, "NDN minimalrelay started, listening on UDP port %d\n",
            udpport);

    inet_aton(strtok(defaultgw,"/"), &sun.ip4.sin_addr);
    sun.ip4.sin_port = atoi(strtok(NULL, ""));
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
    fwd->prefix = ccnl_URItoPrefix(prefix, suite, NULL, NULL);
    fwd->suite = suite;
    fwd->face = ccnl_get_face_or_create(&theRelay, 0, &sun.sa, sizeof(sun.ip4));
    fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;
    theRelay.fib = fwd;

    ccnl_set_timer(1000000, ccnl_minimalrelay_ageing, &theRelay, 0);
    ccnl_io_loop(&theRelay);

    return 0;
}

// eof
