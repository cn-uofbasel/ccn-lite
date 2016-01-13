/*
 * @f ccn-lite-android.c
 * @b native code (library) for Android devices
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
 * 2015-04-29 created
 */

#include <dirent.h>
#include <fnmatch.h>
#include <jni.h>
#include <pthread.h>
#include <regex.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/in.h>
#include <linux/sockios.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android/looper.h>

#define CCNL_UNIX
#define CCNL_ANDROID

// #define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
#define USE_ECHO
#define USE_LINKLAYER                   // we co-use addr formatting for BTLE
//#define USE_FRAG
#define USE_LOGGING
#define USE_HMAC256
#define USE_HTTP_STATUS
#define USE_IPV4
#define USE_MGMT
// #define USE_NACK
// #define USE_NFN
// #define USE_NFN_NSTRANS
// #define USE_NFN_MONITOR
// #define USE_SCHEDULER
#define USE_STATS
#define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
// #define USE_SUITE_LOCALRPC
// #define USE_UNIXSOCKET
// #define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING
#define NEEDS_PACKET_CRAFTING

#include "ccnl-os-includes.h"

#include "ccnl-defs.h"
#include "ccnl-core.h"

#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

void append_to_log(char *line);

#ifdef USE_HMAC256
const char secret_key[] = "some secret secret secret secret";
unsigned char keyval[64];
unsigned char keyid[32];
#endif

#define ccnl_app_RX(x,y)                do{}while(0)
#define local_producer(...)             0

#include "ccnl-core.c"

#include "ccnl-ext-echo.c"
#include "ccnl-ext-hmac.c"
#include "ccnl-ext-http.c"
#include "ccnl-ext-localrpc.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-nfn.c"
#include "ccnl-ext-nfnmonitor.c"
#include "ccnl-ext-sched.c"
#include "ccnl-ext-frag.c"
#include "ccnl-ext-crypto.c"

// ----------------------------------------------------------------------

struct ccnl_relay_s theRelay;

// ----------------------------------------------------------------------

#ifdef USE_LINKLAYER
int
ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype)
{
    struct ifreq ifr;
    int s;

    DEBUGMSG(TRACE, "ccnl_open_ethdev %s 0x%04x\n", devname, ethtype);

    s = socket(AF_PACKET, SOCK_RAW, htons(ethtype));
    if (s < 0) {
        perror("eth socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, (char*) devname, IFNAMSIZ);
    if(ioctl(s, SIOCGIFHWADDR, (void *) &ifr) < 0 ) {
        perror("ethsock ioctl get hw addr");
        return -1;
    }

    sll->sll_family = AF_PACKET;
    memcpy(sll->sll_addr, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    if(ioctl(s, SIOCGIFINDEX, (void *) &ifr) < 0 ) {
        perror("ethsock ioctl get index");
        return -1;
    }
    sll->sll_ifindex = ifr.ifr_ifindex;
    sll->sll_protocol = htons(ethtype);
    if (bind(s, (struct sockaddr*) sll, sizeof(*sll)) < 0) {
        perror("ethsock bind");
        return -1;
    }

    return s;
}
#endif // USE_LINKLAYER

int
ccnl_open_udpdev(int port, struct sockaddr_in *si)
{
    int s;
    unsigned int len;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        return -1;
    }

    si->sin_addr.s_addr = INADDR_ANY;
    si->sin_port = htons(port);
    si->sin_family = PF_INET;
    if(bind(s, (struct sockaddr *)si, sizeof(*si)) < 0) {
        perror("udp sock bind");
        return -1;
    }
    len = sizeof(*si);
    getsockname(s, (struct sockaddr*) si, &len);

    return s;
}

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
    int rc;

    switch(dest->sa.sa_family) {
#ifdef USE_IPV4
    case AF_INET:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ip4, sizeof(struct sockaddr_in));
        DEBUGMSG(DEBUG, "udp sendto %s/%d returned %d\n",
                 inet_ntoa(dest->ip4.sin_addr), ntohs(dest->ip4.sin_port), rc);
        break;
#endif
#ifdef USE_LINKLAYER
    case AF_PACKET:
        rc = jni_bleSend(buf->data, buf->datalen);
        DEBUGMSG(DEBUG, "eth_sendto %s returned %d\n",
                 ll2ascii(dest->linklayer.sll_addr, dest->linklayer.sll_halen), rc);
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
}

void
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

#ifdef USE_UNIXSOCKET
    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
#endif

    close(s);
}

// ----------------------------------------------------------------------

static int inter_ccn_interval = 0; // in usec
static int inter_pkt_interval = 0; // in usec

#ifdef USE_SCHEDULER
struct ccnl_sched_s*
ccnl_relay_defaultFaceScheduler(struct ccnl_relay_s *ccnl,
                                void(*cb)(void*,void*))
{
    return ccnl_sched_pktrate_new(cb, ccnl, inter_ccn_interval);
}

struct ccnl_sched_s*
ccnl_relay_defaultInterfaceScheduler(struct ccnl_relay_s *ccnl,
                                     void(*cb)(void*,void*))
{
    return ccnl_sched_pktrate_new(cb, ccnl, inter_pkt_interval);
}
/*
#else
# define ccnl_relay_defaultFaceScheduler       NULL
# define ccnl_relay_defaultInterfaceScheduler  NULL
*/
#endif // USE_SCHEDULER


void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

char *echopath = "/local/echo";

void
add_udpPort(struct ccnl_relay_s *relay, int port)
{
    struct ccnl_if_s *i;

    i = &relay->ifs[relay->ifcount];
    i->sock = ccnl_open_udpdev(port, &i->addr.ip4);
    if (i->sock < 0) {
        DEBUGMSG(WARNING, "sorry, could not open udp device (port %d)\n",
                 port);
        return;
    }

//      i->frag = CCNL_DGRAM_FRAG_NONE;

#ifdef USE_SUITE_CCNB
    i->mtu = CCN_DEFAULT_MTU;
#endif
#ifdef USE_SUITE_NDNTLV
    i->mtu = NDN_DEFAULT_MTU;
#endif

    i->fwdalli = 1;

    if (relay->defaultInterfaceScheduler)
        i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
    relay->ifcount++;
    DEBUGMSG(INFO, "UDP interface (%s) configured\n",
             ccnl_addr2ascii(&i->addr));
}

// ----------------------------------------------------------------------

void
ccnl_relay_config(struct ccnl_relay_s *relay, int httpport, char *uxpath,
                  int max_cache_entries, char *crypto_face_path)
{
    struct ccnl_if_s *i;

    DEBUGMSG(INFO, "configuring relay\n");

    relay->max_cache_entries = max_cache_entries;
#ifdef USE_SCHEDULER
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
    relay->defaultInterfaceScheduler = ccnl_relay_defaultInterfaceScheduler;
#endif

    // use interface 0 for BT low energy
    relay->ifs[0].mtu = 20;
    if (relay->defaultInterfaceScheduler)
        relay->ifs[0].sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
    relay->ifcount++;

#ifdef USE_SUITE_NDNTLV
    add_udpPort(relay, NDN_UDP_PORT);
#endif
#ifdef USE_SUITE_CCNTLV
    add_udpPort(relay, CCN_UDP_PORT);
#endif

#ifdef USE_HTTP_STATUS
    if (httpport > 0) {
        relay->http = ccnl_http_new(relay, httpport);
    }
#endif // USE_HTTP_STATUS

#ifdef USE_NFN
    relay->km = ccnl_calloc(1, sizeof(struct ccnl_krivine_s));
    relay->km->configid = -1;
#endif

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

void
ccnl_populate_cache(struct ccnl_relay_s *ccnl, char *path)
{
    DIR *dir;
    struct dirent *de;

    dir = opendir(path);
    if (!dir) {
        DEBUGMSG(ERROR, "could not open directory %s\n", path);
        return;
    }

    DEBUGMSG(INFO, "populating cache from directory %s\n", path);

    while ((de = readdir(dir))) {
        char fname[1000];
        struct stat s;
        struct ccnl_buf_s *buf = 0; // , *nonce=0, *ppkd=0, *pkt = 0;
        struct ccnl_content_s *c = 0;
        unsigned char *data;
        int fd, datalen, typ, len, suite, skip;
        struct ccnl_pkt_s *pk;

        if (de->d_name[0] == '.')
            continue;

        strcpy(fname, path);
        strcat(fname, "/");
        strcat(fname, de->d_name);

        if (stat(fname, &s)) {
            perror("stat");
            continue;
        }
        if (S_ISDIR(s.st_mode))
            continue;

        DEBUGMSG(INFO, "loading file %s, %d bytes\n", de->d_name,
                 (int) s.st_size);

        fd = open(fname, O_RDONLY);
        if (!fd) {
            perror("open");
            continue;
        }

        buf = (struct ccnl_buf_s *) ccnl_malloc(sizeof(*buf) + s.st_size);
        if (buf)
            datalen = read(fd, buf->data, s.st_size);
        else
            datalen = -1;
        close(fd);

        if (!buf || datalen != s.st_size || datalen < 2) {
            DEBUGMSG(WARNING, "size mismatch for file %s, %d/%d bytes\n",
                     de->d_name, datalen, (int) s.st_size);
            continue;
        }
        buf->datalen = datalen;
        suite = ccnl_pkt2suite(buf->data, datalen, &skip);

        pk = NULL;
        switch (suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB: {
            unsigned char *start;

            data = start = buf->data + skip;
            datalen -= skip;

            if (data[0] != 0x04 || data[1] != 0x82)
                goto notacontent;
            data += 2;
            datalen -= 2;

            pk = ccnl_ccnb_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            int hdrlen;
            unsigned char *start;

            data = start = buf->data + skip;
            datalen -=  skip;

            hdrlen = ccnl_ccntlv_getHdrLen(data, datalen);
            data += hdrlen;
            datalen -= hdrlen;

            pk = ccnl_ccntlv_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_CISTLV
        case CCNL_SUITE_CISTLV: {
            int hdrlen;
            unsigned char *start;

            data = start = buf->data + skip;
            datalen -=  skip;

            hdrlen = ccnl_cistlv_getHdrLen(data, datalen);
            data += hdrlen;
            datalen -= hdrlen;

            pk = ccnl_cistlv_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV: {
            unsigned char *olddata;

            data = olddata = buf->data + skip;
            datalen -= skip;
            if (ccnl_iottlv_dehead(&data, &datalen, &typ, &len) ||
                                                       typ != IOT_TLV_Reply)
                goto notacontent;
            pk = ccnl_iottlv_bytes2pkt(typ, olddata, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV: {
            unsigned char *olddata;

            data = olddata = buf->data + skip;
            datalen -= skip;
            if (ccnl_ndntlv_dehead(&data, &datalen, &typ, &len) ||
                                                         typ != NDN_TLV_Data)
                goto notacontent;
            pk = ccnl_ndntlv_bytes2pkt(typ, olddata, &data, &datalen);
            break;
        }
#endif
        default:
            DEBUGMSG(WARNING, "unknown packet format (%s)\n", de->d_name);
            goto Done;
        }
        if (!pk) {
            DEBUGMSG(DEBUG, "  parsing error in %s\n", de->d_name);
            goto Done;
        }
        c = ccnl_content_new(ccnl, &pk);
        if (!c) {
            DEBUGMSG(WARNING, "could not create content (%s)\n", de->d_name);
            goto Done;
        }
        ccnl_content_add2cache(ccnl, c);
        c->flags |= CCNL_CONTENT_FLAGS_STATIC;
Done:
        free_packet(pk);
        ccnl_free(buf);
        continue;
notacontent:
        DEBUGMSG(WARNING, "not a content object (%s)\n", de->d_name);
        ccnl_free(buf);
    }

    closedir(dir);
}

// ----------------------------------------------------------------------

ALooper *theLooper;

pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t timer_thread;
volatile int timer_usec = -1;
int pipeT2R[2]; // timer thread to relay

int
ccnl_android_timer_io(int fd, int events, void *data)
{
    unsigned char c;
    int len;

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP))
        return 0;

    len = read(pipeT2R[0], &c, 1);
    DEBUGMSG(TRACE, "timer_io %d\n", len);

    timer_usec = ccnl_run_events();
    pthread_mutex_unlock(&timer_mutex);

    return 1; // continue receiving callbacks
}

int
ccnl_android_udp_io(int fd, int events, void *data)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) data;
    unsigned char buf[CCNL_MAX_PACKET_SIZE];
    int i, len;

    DEBUGMSG(TRACE, "-- udp_io\n");

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        return 0;
    }

    DEBUGMSG(TRACE, "android_udp_io %d 0x%04x, %d\n",
             fd, events, relay->ifcount);

    for (i = 0; i < relay->ifcount; i++) {
        if (relay->ifs[i].sock != fd)
            continue;
        if (events | ALOOPER_EVENT_INPUT) {
            sockunion src_addr;
            socklen_t addrlen = sizeof(sockunion);

            if ((len = recvfrom(relay->ifs[i].sock, buf, sizeof(buf),
                                MSG_DONTWAIT,
                                (struct sockaddr*) &src_addr, &addrlen)) > 0) {

                DEBUGMSG(TRACE, "android_udp_io - input event %d\n", len);
                if (src_addr.sa.sa_family == AF_INET) {
                    ccnl_core_RX(relay, i, buf, len,
                                 &src_addr.sa, sizeof(src_addr.ip4));
                }
            }
        }
    }

    return 1; // continue receiving callbacks
}

int
ccnl_android_http_io(int fd, int events, void *data)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) data;
    struct ccnl_http_s *http = relay->http;

    DEBUGMSG(TRACE, "-- http_io\n");

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        close(http->client);
        http->client = 0;
        // we should check if there are pending clients ...
        return 0;
    }
    if (events | ALOOPER_EVENT_INPUT) {
        int len = sizeof(http->in) - http->inlen - 1;
        len = recv(http->client, http->in + http->inlen, len, 0);
        if (len == 0) {
            ALooper_removeFd(theLooper, http->client);
            DEBUGMSG(WARNING, "web client went away\n");
            close(http->client);
            http->client = 0;
            // we should check if there are pending clients ...
            return 0;
        } else if (len > 0) {
            http->in[len] = 0;
            ccnl_http_status(relay, http);
        }
    }
    if (http->outlen > 0 && (events | ALOOPER_EVENT_OUTPUT)) {
        int len = send(http->client, http->out + http->outoffs,
                       http->outlen, 0);
        if (len > 0) {
            http->outlen -= len;
            http->outoffs += len;
            if (http->outlen <= 0) {
                ALooper_removeFd(theLooper, http->client);
                close(http->client);
                http->client = 0;
                // we should check if there are pending clients ...
                DEBUGMSG(TRACE, " http closed\n");
                return 0;
            } else {
                DEBUGMSG(TRACE, " http more to send %d\n", http->outlen);
            }
        }
    }

    return 1; // continue receiving callbacks
}

int
ccnl_android_http_accept(int fd, int events, void *data)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) data;
    struct ccnl_http_s *http = relay->http;
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);

    DEBUGMSG(TRACE, "-- http_accept\n");

    if (http->client) // only one client at the time
        return 1;

    if (!(events | ALOOPER_EVENT_INPUT))
        return 1;

    http->client = accept(http->server,
                          (struct sockaddr*) &peer, &len);
    if (http->client < 0)
        http->client = 0;
    else {
        DEBUGMSG(DEBUG, "accepted web server client fd=%d\n", http->client);
        http->inlen = http->outlen = http->inoffs = http->outoffs = 0;

        ALooper_addFd(theLooper, http->client,
                      ALOOPER_POLL_CALLBACK,
                      ALOOPER_EVENT_INPUT, // | ALOOPER_EVENT_OUTPUT,
                      ccnl_android_http_io,
                      &theRelay);
    }

    return 1; // continue receiving callbacks
}

void*
timer()
{
    char c = 't';

    usleep(1000000);
    pthread_mutex_lock(&timer_mutex);

    while (1) {
        timer_usec = -1;
        if (theLooper) {
            write(pipeT2R[1], &c, 1);
            pthread_mutex_lock(&timer_mutex);
        }
        usleep(timer_usec < 0 ? 500000 : timer_usec);
    }

    return NULL;
}

// ----------------------------------------------------------------------

char*
ccnl_android_init()
{
    static char done;
    static char hello[200];
    time_t now = time(NULL);
    int i, dummy = 0;
    struct ccnl_prefix_s *echoprefix;

    if (done)
        return hello;
    done = 1;

    time(&theRelay.startup_time);
    debug_level = INFO;

    ccnl_core_init();

    DEBUGMSG(INFO, "This is 'ccn-lite-android', starting at %s",
             ctime(&theRelay.startup_time) + 4);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);

    ccnl_relay_config(&theRelay, 8080, NULL, 0, NULL);

    theLooper = ALooper_forThread();

    // launch timer thread
    pipe2(pipeT2R, O_DIRECT);
    pthread_create(&timer_thread, NULL, timer, NULL);

    // timer handler
    ALooper_addFd(theLooper, pipeT2R[0], ALOOPER_POLL_CALLBACK,
                  ALOOPER_EVENT_INPUT, ccnl_android_timer_io, &theRelay);

    // UDP and HTTP ports
    for (i = 0; i < theRelay.ifcount; i++) {
        if (theRelay.ifs[i].addr.sa.sa_family == AF_INET)
            ALooper_addFd(theLooper, theRelay.ifs[i].sock,
                  ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
                  ccnl_android_udp_io, &theRelay);
    }
    ALooper_addFd(theLooper, theRelay.http->server, ALOOPER_POLL_CALLBACK,
                  ALOOPER_EVENT_INPUT, ccnl_android_http_accept, &theRelay);


    ccnl_populate_cache(&theRelay, "/mnt/sdcard/ccn-lite");

#ifdef USE_SUITE_CCNTLV
    strcpy(hello, echopath);
    echoprefix = ccnl_URItoPrefix(hello, CCNL_SUITE_CCNTLV, NULL, &dummy);
    ccnl_echo_add(&theRelay, echoprefix);
#endif
#ifdef USE_SUITE_IOTTLV
    strcpy(hello, echopath);
    echoprefix = ccnl_URItoPrefix(hello, CCNL_SUITE_IOTTLV, NULL, NULL);
    ccnl_echo_add(&theRelay, echoprefix);
#endif
#ifdef USE_SUITE_NDNTLV
    strcpy(hello, echopath);
    echoprefix = ccnl_URItoPrefix(hello, CCNL_SUITE_NDNTLV, NULL, NULL);
    ccnl_echo_add(&theRelay, echoprefix);
#endif

#ifdef USE_HMAC256
    ccnl_hmac256_keyval((unsigned char*)secret_key,
                        strlen(secret_key), keyval);
    ccnl_hmac256_keyid((unsigned char*)secret_key,
                        strlen(secret_key), keyid);
#endif

    sprintf(hello, "ccn-lite-android, %s",
            ctime(&theRelay.startup_time) + 4);

    return hello;
}


char*
ccnl_android_getTransport()
{
    static char addr[100];
    struct ifreq *ifr;
    struct ifconf ifc;
    int s, i;
    int numif;

    // find number of interfaces.
    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_ifcu.ifcu_req = NULL;
    ifc.ifc_len = 0;

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return NULL;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
        return NULL;
    if ((ifr = (struct ifreq*) ccnl_malloc(ifc.ifc_len)) == NULL)
        return NULL;
    ifc.ifc_ifcu.ifcu_req = ifr;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
        ccnl_free(ifr);
        return NULL;
    }
    numif = ifc.ifc_len / sizeof(struct ifreq);

    for (i = 0; i < numif; i++) {
        struct ifreq *r = &ifr[i];
        struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
        if (strcmp(r->ifr_name, "lo")) {
            sprintf(addr, "(%s)  UDP    ", ifr[i].ifr_name);
            for (i = 0; i < theRelay.ifcount; i++) {
                if (theRelay.ifs[i].addr.sa.sa_family != AF_INET)
                    continue;
                sin->sin_port = theRelay.ifs[i].addr.ip4.sin_port;
                sprintf(addr + strlen(addr), "%s    ",
                        ccnl_addr2ascii((sockunion*)sin));
            }
            ccnl_free(ifr);
            return addr;
        }
    }
    ccnl_free(ifr);
    return NULL;
}

// eof
