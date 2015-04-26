/*
 * @f ccn-lite-arduino.c
 * @b a small server/relay for the Ardiuno platform
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-04-25 created
 */

#define CCNL_ARDUINO

//#define USE_CCNxDIGEST
//#define USE_DEBUG                      // must select this for USE_MGMT
//#define USE_DEBUG_MALLOC
//#define USE_FRAG
//#define USE_ETHERNET
//#define USE_HTTP_STATUS
// #define USE_LOGGING
//#define USE_MGMT
//#define USE_NACK
//#define USE_NFN
//#define USE_NFN_NSTRANS
//#define USE_NFN_MONITOR
//#define USE_SCHEDULER
//#define USE_SUITE_CCNB                 // must select this for USE_MGMT
//#define USE_SUITE_CCNTLV
#define USE_SUITE_IOTTLV
//#define USE_SUITE_NDNTLV
//#define USE_SUITE_LOCALRPC
//#define USE_UNIXSOCKET
//#define USE_SIGNATURES

// #include "ccnl-os-includes.h"

struct sockaddr {
  unsigned char sa_family;
};

# define ntohs(i)  (((i<<8) & 0xff00) | ((i>>8) & 0x0ff))
# define htons(i)  ntohs(i)

// 32 bit values in an Arduino context?
# define ntohl(i)  -1
# define htonl(i)  -1


#ifdef XXX
#ifdef EXTRACT_DEBUGMSG_STR
# define CONCATE(X,Y) X##Y
# define CONCAT(X,Y) CONCATE(X,Y)
# undef DEBUGMSG
//# define DEBUGMSG(L,F) PROGMEM_STR(CONCAT(var,__COUNTER__), F)
# define DEBUGMSG(L,F,...) PROGMEM_STR(CONCAT(var,__COUNTER__), F)
#else
# undef DEBUGMSG
# define DEBUGMSG(L,F,...) DEBUGMSG_PROGMEM(L,__COUNTER__,__VA_ARGS__)
#endif
#endif

static char mem[512];

#undef DEBUGMSG
#define DEBUGMSG(L,FMT, ...) do { \
  if ((L) <= debug_level) {                      \
    sprintf_P(mem, PSTR(FMT), ##__VA_ARGS__);      \
    Serial.print(timestamp()); \
    Serial.print(" "); \
    Serial.print(mem); \
    Serial.print("\r"); \
  } \
} while(0)

#include "ccnl-defs.h"
#include "ccnl-core.h"

#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
//#include "ccnl-ext-logging.c"

#define FATAL   94  // FATAL
#define ERROR   95  // ERROR
#define WARNING 96  // WARNING 
#define INFO    97  // INFO 
#define DEBUG   98  // DEBUG 
#define VERBOSE 99  // VERBOSE
#define TRACE 	100 // TRACE 

const char Xmsg[] PROGMEM = "hello world <%s>";
const char* const Xmsgs[] PROGMEM = {Xmsg};

//#undef DEBUGMSG (
//#define DEBUGMSG(Lvl,Mid

#define ccnl_app_RX(x,y)                do{}while(0)
#define ccnl_print_stats(x,y)           do{}while(0)

char* ccnl_addr2ascii(sockunion *su);

#include "ccnl-core.c"

/*
#include "ccnl-ext-http.c"
#include "ccnl-ext-localrpc.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-nfn.c"
#include "ccnl-ext-nfnmonitor.c"
#include "ccnl-ext-sched.c"
#include "ccnl-ext-frag.c"
#include "ccnl-ext-crypto.c"
*/

// ----------------------------------------------------------------------

//static struct ccnl_relay_s theRelay;
static const char suite = CCNL_SUITE_IOTTLV;

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

long
timevaldelta(struct timeval *a, struct timeval *b) {
    return 1000000*(a->tv_sec - b->tv_sec) + a->tv_usec - b->tv_usec;
}

void*
ccnl_set_timer(long usec, void (*fct)(void *aux1, void *aux2),
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
        if (t->fct)
            (t->fct)(t->node, t->intarg);
        else if (t->fct2)
            (t->fct2)(t->aux1, t->aux2);
        eventqueue = t->next;
        ccnl_free(t);
    }

    return NULL;
}

// ----------------------------------------------------------------------

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
/*
    int rc;

    if (dest) {
    switch(dest->sa.sa_family) {
    case AF_INET:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ip4, sizeof(struct sockaddr_in));
        DEBUGMSG(DEBUG, "udp sendto %s/%d returned %d\n",
                 inet_ntoa(dest->ip4.sin_addr), ntohs(dest->ip4.sin_port), rc);
        break;
#ifdef USE_ETHERNET
    case AF_PACKET:
        rc = ccnl_eth_sendto(ifc->sock,
                             dest->eth.sll_addr,
                             ifc->addr.eth.sll_addr,
                             buf->data, buf->datalen);
        DEBUGMSG(DEBUG, "eth_sendto %s returned %d\n",
                 eth2ascii(dest->eth.sll_addr), rc);
        break;
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ux, sizeof(struct sockaddr_un));
        DEBUGMSG(DEBUG, "unix sendto %s returned %d\n",
                 dest->ux.sun_path, rc);
        break;
#endif
    default:
        DEBUGMSG(WARNING, "unknown transport\n");
        break;
    }
    rc = 0; // just to silence a compiler warning (if USE_DEBUG is not set)
*/
    DEBUGMSG(WARNING, "unknown transport\n");
}

void
ccnl_close_socket(int s)
{
/*
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
    close(s);
*/
}

// ----------------------------------------------------------------------


void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

unsigned long
ccnl_arduino_run_events(struct ccnl_relay_s *relay)
{
    struct timeval *timeout;
    timeout = ccnl_run_events();
    if (timeout)
        return 1000 * timeout->tv_sec + timeout->tv_usec / 1000;
    else
        return 10;

/*
    int i, len, maxfd = -1, rc;
    fd_set readfs, writefs;
    unsigned char buf[CCNL_MAX_PACKET_SIZE];
    
    if (ccnl->ifcount == 0) {
        DEBUGMSG(ERROR, "no socket to work with, not good, quitting\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < ccnl->ifcount; i++)
        if (ccnl->ifs[i].sock > maxfd)
            maxfd = ccnl->ifs[i].sock;
    maxfd++;

    DEBUGMSG(INFO, "starting main event and IO loop\n");
    while (!ccnl->halt_flag) {
        struct timeval *timeout;

        FD_ZERO(&readfs);
        FD_ZERO(&writefs);

#ifdef USE_HTTP_STATUS
        ccnl_http_anteselect(ccnl, ccnl->http, &readfs, &writefs, &maxfd);
#endif
        for (i = 0; i < ccnl->ifcount; i++) {
            FD_SET(ccnl->ifs[i].sock, &readfs);
            if (ccnl->ifs[i].qlen > 0)
                FD_SET(ccnl->ifs[i].sock, &writefs);
        }

        timeout = ccnl_run_events();
        rc = select(maxfd, &readfs, &writefs, NULL, timeout);

        if (rc < 0) {
            perror("select(): ");
            exit(EXIT_FAILURE);
        }

#ifdef USE_HTTP_STATUS
        ccnl_http_postselect(ccnl, ccnl->http, &readfs, &writefs);
#endif
        for (i = 0; i < ccnl->ifcount; i++) {
            if (FD_ISSET(ccnl->ifs[i].sock, &readfs)) {
                sockunion src_addr;
                socklen_t addrlen = sizeof(sockunion);
                if ((len = recvfrom(ccnl->ifs[i].sock, buf, sizeof(buf), 0,
                                (struct sockaddr*) &src_addr, &addrlen)) > 0) {
                    
                    if (src_addr.sa.sa_family == AF_INET) {
                        ccnl_core_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ip4));
                    }
#ifdef USE_ETHERNET
                    else if (src_addr.sa.sa_family == AF_PACKET) {
                        if (len > 14)
                            ccnl_core_RX(ccnl, i, buf+14, len-14,
                                         &src_addr.sa, sizeof(src_addr.eth));
                    }
#endif
#ifdef USE_UNIXSOCKET
                    else if (src_addr.sa.sa_family == AF_UNIX) {
                        ccnl_core_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ux));
                    }
#endif
                }
            }

            if (FD_ISSET(ccnl->ifs[i].sock, &writefs)) {
              ccnl_interface_CTS(ccnl, ccnl->ifs + i);
            }
        }
    }
*/

    return 0;
}


// ----------------------------------------------------------------------

int
ccnl_arduino_init(struct ccnl_relay_s *relay)
{
    int opt, max_cache_entries = -1, udpport = -1, httpport = -1;
    char *datadir = NULL, *ethdev = NULL;

    DEBUGMSG(INFO, "This is ccn-lite-relay\n");
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string());
    DEBUGMSG(INFO, "  using suite %s\n", ccnl_suite2str(suite));

    DEBUGMSG(INFO, "configuring the relay\n");

    // init interfaces here

    ccnl_core_init();

    relay->max_cache_entries = 0;
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);

    DEBUGMSG(INFO, "config done, starting the loop\n");

    return 0;
}

// eof
