/*
 * @f ccn-lite-relay.c
 * @b user space CCN relay
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
 * 2011-11-22 created
 */

#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CCNL_UNIX

#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
#define USE_DUP_CHECK
#define USE_ECHO
#define USE_LINKLAYER
//#define USE_FRAG
#define USE_HMAC256
#define USE_HTTP_STATUS
#define USE_IPV4
#define USE_IPV6
#define USE_MGMT
// #define USE_NACK
// #define USE_NFN
#define USE_NFN_NSTRANS
// #define USE_NFN_MONITOR
// #define USE_SCHEDULER
#define USE_STATS
#define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
#define USE_SUITE_LOCALRPC
#define USE_UNIXSOCKET
// #define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING

#include "ccnl-os-includes.h"

#include "ccnl-defs.h"
#include "ccnl-core.h"

#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define local_producer(...)             0
#define cache_strategy_remove(...)      0

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
char suite = CCNL_SUITE_DEFAULT;

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

#ifdef USE_UNIXSOCKET
int
ccnl_open_unixpath(char *path, struct sockaddr_un *ux)
{
  int sock, bufsize;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        return -1;
    }

    unlink(path);
    ux->sun_family = AF_UNIX;
    strcpy(ux->sun_path, path);

    if (bind(sock, (struct sockaddr *) ux, sizeof(struct sockaddr_un))) {
        perror("binding name to datagram socket");
        close(sock);
        return -1;
    }

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;

}
#endif // USE_UNIXSOCKET

#ifdef USE_IPV4
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
#endif

#ifdef USE_IPV6
int
ccnl_open_udp6dev(int port, struct sockaddr_in6 *sin)
{
    int s;
    unsigned int len;

    s = socket(PF_INET6, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        return -1;
    }

    sin->sin6_addr = in6addr_any;
    sin->sin6_port = htons(port);
    sin->sin6_family = PF_INET6;
    if(bind(s, (struct sockaddr *)sin, sizeof(*sin)) < 0) {
        perror("udp sock bind");
        return -1;
    }
    len = sizeof(*sin);
    getsockname(s, (struct sockaddr*) sin, &len);

    return s;
}
#endif

#ifdef USE_LINKLAYER
int
ccnl_eth_sendto(int sock, unsigned char *dst, unsigned char *src,
                unsigned char *data, int datalen)
{
    short type = htons(CCNL_ETH_TYPE);
    unsigned char buf[2000];
    int hdrlen;

#ifdef USE_DEBUG
    strcpy((char*)buf, ll2ascii(dst, 6));
    DEBUGMSG(TRACE, "ccnl_eth_sendto %d bytes (src=%s, dst=%s)\n",
             datalen, ll2ascii(src, 6), buf);
#endif

    hdrlen = 14;
    if ((datalen+hdrlen) > sizeof(buf))
            datalen = sizeof(buf) - hdrlen;
    memcpy(buf, dst, 6);
    memcpy(buf+6, src, 6);
    memcpy(buf+12, &type, sizeof(type));
    memcpy(buf+hdrlen, data, datalen);

    return sendto(sock, buf, hdrlen + datalen, 0, 0, 0);
}
#endif // USE_LINKLAYER

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
        /*
        {
            int fd = open("t.bin", O_WRONLY | O_CREAT | O_TRUNC);
            write(fd, buf->data, buf->datalen);
            close(fd);
        }
        */

        break;
#endif
#ifdef USE_IPV6
    case AF_INET6:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ip6, sizeof(struct sockaddr_in6));
	{
	    char abuf[INET6_ADDRSTRLEN];
	    DEBUGMSG(DEBUG, "udp sendto %s/%d returned %d\n",
		     inet_ntop(AF_INET6, &dest->ip6.sin6_addr, abuf, sizeof(abuf)),
		     ntohs(dest->ip6.sin6_port), rc);
	}

        break;
#endif
#ifdef USE_LINKLAYER
    case AF_PACKET:
        rc = ccnl_eth_sendto(ifc->sock,
                             dest->linklayer.sll_addr,
                             ifc->addr.linklayer.sll_addr,
                             buf->data, buf->datalen);
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
    (void) rc; // just to silence a compiler warning (if USE_DEBUG is not set)
}

int
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
    return close(s);
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


static int lasthour = -1;

void ccnl_ageing(void *relay, void *aux)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    if (lasthour != tm->tm_hour) {
        DEBUGMSG(INFO, "local time is %s", ctime(&t));
        lasthour = tm->tm_hour;
    }

    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

#if defined(USE_IPV4) || defined(USE_IPV6)
void
ccnl_relay_udp(struct ccnl_relay_s *relay, int port, int af)
{
    struct ccnl_if_s *i;

    if (port < 0)
        return;
    i = &relay->ifs[relay->ifcount];
    switch (af) {
#ifdef USE_IPV4
    case AF_INET:
	i->sock = ccnl_open_udpdev(port, &i->addr.ip4);
	break;
#endif
#ifdef USE_IPV6
    case AF_INET6:
	i->sock = ccnl_open_udp6dev(port, &i->addr.ip6);
	break;
#endif
    default:
	return;
    }
    if (i->sock <= 0) {
        DEBUGMSG(WARNING, "sorry, could not open udp device (port %d)\n",
                 port);
        return;
    }

//      i->frag = CCNL_DGRAM_FRAG_NONE;

#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        i->mtu = CCN_DEFAULT_MTU;
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        i->mtu = CCN_DEFAULT_MTU;
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        i->mtu = NDN_DEFAULT_MTU;
#endif
    i->fwdalli = 1;
    relay->ifcount++;
    DEBUGMSG(INFO, "UDP interface (%s) configured\n",
             ccnl_addr2ascii(&i->addr));
    if (relay->defaultInterfaceScheduler)
        i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
}
#endif

void
ccnl_relay_config(struct ccnl_relay_s *relay, char *ethdev,
                  int udpport1, int udpport2,
		  int udp6port1, int udp6port2, int httpport,
                  char *uxpath, int suite, int max_cache_entries,
                  char *crypto_face_path)
{
#if defined(USE_LINKLAYER) || defined(USE_UNIXSOCKET)
    struct ccnl_if_s *i;
#endif

    DEBUGMSG(INFO, "configuring relay\n");

    relay->max_cache_entries = max_cache_entries;
    relay->max_pit_entries = CCNL_DEFAULT_MAX_PIT_ENTRIES;
#ifdef USE_SCHEDULER
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
    relay->defaultInterfaceScheduler = ccnl_relay_defaultInterfaceScheduler;
#endif

#ifdef USE_LINKLAYER
    // add (real) eth0 interface with index 0:
    if (ethdev) {
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_ethdev(ethdev, &i->addr.linklayer, CCNL_ETH_TYPE);
        i->mtu = 1500;
        i->reflect = 1;
        i->fwdalli = 1;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "ETH interface (%s %s) configured\n",
                     ethdev, ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
        } else
            DEBUGMSG(WARNING, "sorry, could not open eth device\n");
    }
#endif // USE_LINKLAYER

#ifdef USE_IPV4
    ccnl_relay_udp(relay, udpport1, AF_INET);
    ccnl_relay_udp(relay, udpport2, AF_INET);
#endif

#ifdef USE_IPV6
    ccnl_relay_udp(relay, udp6port1, AF_INET6);
    ccnl_relay_udp(relay, udp6port2, AF_INET6);
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

#ifdef USE_UNIXSOCKET
    if (uxpath) {
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_unixpath(uxpath, &i->addr.ux);
        i->mtu = 4096;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "UNIX interface (%s) configured\n",
                     ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
        } else
            DEBUGMSG(WARNING, "sorry, could not open unix datagram device\n");
    }
#ifdef USE_SIGNATURES
    if(crypto_face_path) {
        char h[1024];
        //sending interface + face
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_unixpath(crypto_face_path, &i->addr.ux);
        i->mtu = 4096;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "new UNIX interface (%s) configured\n",
                     ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
            ccnl_crypto_create_ccnl_crypto_face(relay, crypto_face_path);
            relay->crypto_path = crypto_face_path;
        } else
            DEBUGMSG(WARNING, "sorry, could not open unix datagram device\n");

        //receiving interface
        memset(h,0,sizeof(h));
        sprintf(h,"%s-2",crypto_face_path);
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_unixpath(h, &i->addr.ux);
        i->mtu = 4096;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "new UNIX interface (%s) configured\n",
                     ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
            //create_ccnl_crypto_face(relay, crypto_face_path);
        } else
            DEBUGMSG(WARNING, "sorry, could not open unix datagram device\n");
    }
#endif //USE_SIGNATURES
#endif // USE_UNIXSOCKET

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

int
ccnl_io_loop(struct ccnl_relay_s *ccnl)
{
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
        int usec;

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

        usec = ccnl_run_events();
        if (usec >= 0) {
            struct timeval deadline;
            deadline.tv_sec = usec / 1000000;
            deadline.tv_usec = usec % 1000000;
            rc = select(maxfd, &readfs, &writefs, NULL, &deadline);
        } else
            rc = select(maxfd, &readfs, &writefs, NULL, NULL);

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
                    if (0) {}
#ifdef USE_IPV4
                    else if (src_addr.sa.sa_family == AF_INET) {
                        ccnl_core_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ip4));
                    }
#endif
#ifdef USE_IPV6
                    else if (src_addr.sa.sa_family == AF_INET6) {
                        ccnl_core_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ip6));
                    }
#endif
#ifdef USE_LINKLAYER
                    else if (src_addr.sa.sa_family == AF_PACKET) {
                        if (len > 14)
                            ccnl_core_RX(ccnl, i, buf+14, len-14,
                                         &src_addr.sa, sizeof(src_addr.linklayer));
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

    return 0;
}


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
        int fd, datalen, suite, skip;
        unsigned char *data;
        (void) data; // silence compiler warning (if any USE_SUITE_* is not set)
#if defined(USE_SUITE_IOTTLV) || defined(USE_SUITE_NDNTLV)
        unsigned int typ;
        int len;
#endif
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
            if (ccnl_ndntlv_dehead(&data, &datalen, (int*) &typ, &len) ||
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
#if defined(USE_SUITE_CCNB) || defined(USE_SUITE_IOTTLV) || defined(USE_SUITE_NDNTLV)
notacontent:
        DEBUGMSG(WARNING, "not a content object (%s)\n", de->d_name);
        ccnl_free(buf);
#endif
    }

    closedir(dir);
}

// ----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int opt, max_cache_entries = -1, httpport = -1;
    int udpport1 = -1, udpport2 = -1;
    int udp6port1 = -1, udp6port2 = -1;
    char *datadir = NULL, *ethdev = NULL, *crypto_sock_path = NULL;
#ifdef USE_UNIXSOCKET
    char *uxpath = CCNL_DEFAULT_UNIXSOCKNAME;
#else
    char *uxpath = NULL;
#endif
#ifdef USE_ECHO
    char *echopfx = NULL;
#endif

    time(&theRelay.startup_time);
    srandom(time(NULL));

    while ((opt = getopt(argc, argv, "hc:d:e:g:i:o:p:s:t:u:6:v:x:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'd':
            datadir = optarg;
            break;
        case 'e':
            ethdev = optarg;
            break;
        case 'g':
            inter_pkt_interval = atoi(optarg);
            break;
        case 'i':
            inter_ccn_interval = atoi(optarg);
            break;
#ifdef USE_ECHO
        case 'o':
            echopfx = optarg;
            break;
#endif
        case 'p':
            crypto_sock_path = optarg;
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite))
                goto usage;
            break;
        case 't':
            httpport = atoi(optarg);
            break;
        case 'u':
            if (udpport1 == -1)
                udpport1 = atoi(optarg);
            else
                udpport2 = atoi(optarg);
            break;
        case '6':
            if (udp6port1 == -1)
                udp6port1 = atoi(optarg);
            else
                udp6port2 = atoi(optarg);
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'x':
            uxpath = optarg;
            break;
        case 'h':
        default:
usage:
            fprintf(stderr,
                    "usage: %s [options]\n"
                    "  -c MAX_CONTENT_ENTRIES\n"
                    "  -d databasedir\n"
                    "  -e ethdev\n"
                    "  -g MIN_INTER_PACKET_INTERVAL\n"
                    "  -h\n"
                    "  -i MIN_INTER_CCNMSG_INTERVAL\n"
#ifdef USE_ECHO
                    "  -o echo_prefix\n"
#endif
                    "  -p crypto_face_ux_socket\n"
                    "  -s SUITE (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
                    "  -t tcpport (for HTML status page)\n"
                    "  -u udpport (can be specified twice)\n"
                    "  -6 udp6port (can be specified twice)\n"

#ifdef USE_LOGGING
                    "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
#ifdef USE_UNIXSOCKET
                    "  -x unixpath\n"
#endif
                    , argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    opt = ccnl_suite2defaultPort(suite);
    if (udpport1 < 0)
        udpport1 = opt;
    if (httpport < 0)
        httpport = opt;

    ccnl_core_init();

    DEBUGMSG(INFO, "This is ccn-lite-relay, starting at %s",
             ctime(&theRelay.startup_time) + 4);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);
//    DEBUGMSG(INFO, "using suite %s\n", ccnl_suite2str(suite));

    ccnl_relay_config(&theRelay, ethdev, udpport1, udpport2,
		      udp6port1, udp6port2, httpport,
                      uxpath, suite, max_cache_entries, crypto_sock_path);
    if (datadir)
        ccnl_populate_cache(&theRelay, datadir);

#ifdef USE_ECHO
    if (echopfx) {
        struct ccnl_prefix_s *pfx;
        char *dup = ccnl_strdup(echopfx);

        pfx = ccnl_URItoPrefix(dup, suite, NULL, NULL);
        if (pfx)
            ccnl_echo_add(&theRelay, pfx);
        ccnl_free(dup);
    }
#endif

    ccnl_io_loop(&theRelay);

    while (eventqueue)
        ccnl_rem_timer(eventqueue);

    ccnl_core_cleanup(&theRelay);
#ifdef USE_HTTP_STATUS
    theRelay.http = ccnl_http_cleanup(theRelay.http);
#endif
#ifdef USE_DEBUG_MALLOC
    debug_memdump();
#endif

    return 0;
}

// eof
