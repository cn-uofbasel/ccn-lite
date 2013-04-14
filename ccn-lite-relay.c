/*
 * @f ccn-lite-relay.c
 * @b user space CCN relay
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
 * 2011-11-22 created
 */

#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>


#define CCNL_UNIX

#define USE_DEBUG
#define USE_DEBUG_MALLOC
#define USE_ENCAPS
#define USE_ETHERNET
#define USE_HTTP_STATUS
#define USE_MGMT
#define USE_SCHEDULER
#define USE_UNIXSOCKET

#include "ccnl-includes.h"

#include "ccnx.h"
#include "ccnl.h"
#include "ccnl-core.h"

#include "ccnl-ext-debug.c"
#include "ccnl-ext.h"
#include "ccnl-platform.c"

#define ccnl_app_RX(x,y)		do{}while(0)
#define ccnl_print_stats(x,y)		do{}while(0)

#include "ccnl-core.c"

#include "ccnl-ext-http.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-sched.c"
#include "ccnl-pdu.c"
#include "ccnl-ext-encaps.c"

// ----------------------------------------------------------------------

struct ccnl_relay_s theRelay;

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

#ifdef USE_ETHERNET
int
ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll)
{
    struct ifreq ifr;
    int s;

    sll->sll_family = AF_PACKET;
    sll->sll_protocol = htons(CCNL_ETH_TYPE);
    s = socket(AF_PACKET, SOCK_RAW, sll->sll_protocol);
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
    memcpy(sll->sll_addr, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    if(ioctl(s, SIOCGIFINDEX, (void *) &ifr) < 0 ) {
        perror("ethsock ioctl get index");
	return -1;
    }
    sll->sll_ifindex = ifr.ifr_ifindex;
    sll->sll_protocol = htons(CCNL_ETH_TYPE);
    if (bind(s, (struct sockaddr*) sll, sizeof(*sll)) < 0) {
        perror("ethsock bind");
	return -1;
    }

    return s;
}
#endif // USE_ETHERNET


#ifdef USE_UNIXSOCKET
int
ccnl_open_unixpath(char *path, struct sockaddr_un *ux)
{
    int sock;

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

    return sock;

}
#endif // USE_UNIXSOCKET


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

#ifdef USE_ETHERNET
int
ccnl_eth_sendto(int sock, unsigned char *dst, unsigned char *src,
	       unsigned char *data, int datalen)
{
    short type = htons(CCNL_ETH_TYPE);
    unsigned char buf[2000];
    int hdrlen;

    strcpy((char*)buf, eth2ascii(dst));
    DEBUGMSG(99, "ccnl_eth_sendto %d bytes (src=%s, dst=%s)\n",
	     datalen, eth2ascii(src), buf);

    hdrlen = 14;
    if ((datalen+hdrlen) > sizeof(buf))
            datalen = sizeof(buf) - hdrlen;
    memcpy(buf, dst, 6);
    memcpy(buf+6, src, 6);
    memcpy(buf+12, &type, sizeof(type));
    memcpy(buf+hdrlen, data, datalen);

    return sendto(sock, buf, hdrlen + datalen, 0, 0, 0);
}
#endif // USE_ETHERNET


void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
	   sockunion *dest, struct ccnl_buf_s *buf)
{
    int rc;

    switch(dest->sa.sa_family) {
    case AF_INET:
	rc = sendto(ifc->sock,
		    buf->data, buf->datalen, 0,
		    (struct sockaddr*) &dest->ip4, sizeof(struct sockaddr_in));
	DEBUGMSG(99, "udp sendto %s returned %d\n",
		 inet_ntoa(dest->ip4.sin_addr), rc);
	break;
#ifdef USE_ETHERNET
    case AF_PACKET:
 	rc = ccnl_eth_sendto(ifc->sock,
			     dest->eth.sll_addr,
			     ifc->addr.eth.sll_addr,
			     buf->data, buf->datalen);
	DEBUGMSG(99, "eth_sendto %s returned %d\n",
		 eth2ascii(dest->eth.sll_addr), rc);
	break;
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
	rc = sendto(ifc->sock,
		    buf->data, buf->datalen, 0,
		    (struct sockaddr*) &dest->ux, sizeof(struct sockaddr_un));
	DEBUGMSG(99, "unix sendto %s returned %d\n",
		 dest->ux.sun_path, rc);
	break;
#endif
    default:
	break;
    }
    rc = rc; // just to silence a compiler warning (if USE_DEBUG is not set)
}

void
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
					su.sun_family == AF_UNIX) {
	unlink(su.sun_path);
    }
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
#else
# define ccnl_relay_defaultFaceScheduler       NULL
# define ccnl_relay_defaultInterfaceScheduler  NULL
#endif // USE_SCHEDULER


void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

void
ccnl_relay_config(struct ccnl_relay_s *relay, char *ethdev, int udpport,
		  int tcpport, char *uxpath, int max_cache_entries)
{
    struct ccnl_if_s *i;

    DEBUGMSG(99, "ccnl_relay_config\n");

    relay->max_cache_entries = max_cache_entries;
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
    relay->defaultInterfaceScheduler = ccnl_relay_defaultInterfaceScheduler;

#ifdef USE_ETHERNET
    // add (real) eth0 interface with index 0:
    if (ethdev) {
	i = &relay->ifs[relay->ifcount];
	i->sock = ccnl_open_ethdev(ethdev, &i->addr.eth);
	i->mtu = 1500;
	i->reflect = 1;
	i->fwdalli = 1;
	if (i->sock >= 0) {
	    relay->ifcount++;
	    DEBUGMSG(99, "new ETH interface (%s %s) configured\n",
		     ethdev, ccnl_addr2ascii(&i->addr));
	    i->sched = relay->defaultInterfaceScheduler(relay,
							ccnl_interface_CTS);
	} else
	    fprintf(stderr, "sorry, could not open eth device\n");
    }
#endif // USE_ETHERNET

    if (udpport > 0) {
	i = &relay->ifs[relay->ifcount];
	i->sock = ccnl_open_udpdev(udpport, &i->addr.ip4);
//	i->encaps = CCNL_DGRAM_ENCAPS_NONE;
	i->mtu = CCN_DEFAULT_MTU;
	i->fwdalli = 1;
	if (i->sock >= 0) {
	    relay->ifcount++;
	    DEBUGMSG(99, "new UDP interface (ip4 %s) configured\n",
		     ccnl_addr2ascii(&i->addr));
	    i->sched = relay->defaultInterfaceScheduler(relay,
							ccnl_interface_CTS);
	} else
	    fprintf(stderr, "sorry, could not open udp device\n");
    }

#ifdef USE_HTTP_STATUS
    if (tcpport) {
	relay->http = ccnl_http_new(relay, tcpport);
    }
#endif // USE_HTTP_STATUS

#ifdef USE_UNIXSOCKET
    if (uxpath) {
	i = &relay->ifs[relay->ifcount];
	i->sock = ccnl_open_unixpath(uxpath, &i->addr.ux);
	i->mtu = 4096;
	if (i->sock >= 0) {
	    relay->ifcount++;
	    DEBUGMSG(99, "new UNIX interface (%s) configured\n",
		     ccnl_addr2ascii(&i->addr));
	    i->sched = relay->defaultInterfaceScheduler(relay,
							ccnl_interface_CTS);
	} else
	    fprintf(stderr, "sorry, could not open unix datagram device\n");
    }
#endif // USE_UNIXSOCKET

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------


int
ccnl_io_loop(struct ccnl_relay_s *ccnl)
{
    int i, maxfd = -1, rc;
    fd_set readfs, writefs;
    unsigned char buf[CCNL_MAX_PACKET_SIZE];
    int len;
    
    if (ccnl->ifcount == 0) {
	fprintf(stderr, "no socket to work with, not good, quitting\n");
	exit(EXIT_FAILURE);
    }
    for (i = 0; i < ccnl->ifcount; i++)
	if (ccnl->ifs[i].sock > maxfd)
	    maxfd = ccnl->ifs[i].sock;
    maxfd++;

    DEBUGMSG(1, "starting main event and IO loop\n");
    while(!ccnl->halt_flag) {
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

    return 0;
}


void
ccnl_populate_cache(struct ccnl_relay_s *ccnl, char *path)
{
    DIR *dir;
    struct dirent *de;

    DEBUGMSG(99, "ccnl_populate_cache %s\n", path);

    dir = opendir(path);
    if (!dir)
	return;
    while ((de = readdir(dir))) {
	if (!fnmatch("*.ccnb", de->d_name, FNM_NOESCAPE)) {
	    char fname[1000];
	    struct stat s;
	    strcpy(fname, path);
	    strcat(fname, "/");
	    strcat(fname, de->d_name);
	    if (stat(fname, &s)) {
		perror("stat");
	    } else {
		struct ccnl_buf_s *buf = 0;
		int fd;
		DEBUGMSG(6, "loading file %s, %d bytes\n",
			 de->d_name, (int) s.st_size);

		fd = open(fname, O_RDONLY);
		if (!fd) {
		    perror("open");
		    continue;
		}
		buf = (struct ccnl_buf_s *) ccnl_malloc(sizeof(*buf) +
							s.st_size);
		buf->datalen = s.st_size;
		read(fd, buf->data, s.st_size);
		close(fd);
		if (ccnl_is_content(buf)) {
		        struct ccnl_prefix_s *prefix = 0;
			struct ccnl_content_s *c = 0;
			struct ccnl_buf_s *nonce=0, *ppkd=0;
			unsigned char *content;
			int contlen;
			if (ccnl_extract_prefix_nonce_ppkd(buf, &prefix,
				&nonce, &ppkd, &content, &contlen) || !prefix) {
			    DEBUGMSG(6, "  parsing error or no prefix\n");
			    goto Done;
			}
			c = ccnl_content_new(ccnl, &buf, &prefix, &ppkd,
					     content, contlen);
			if (!c)
			    goto Done;
			ccnl_content_add2cache(ccnl, c);
			c->flags |= CCNL_CONTENT_FLAGS_STATIC;
Done:
			free_prefix(prefix);
			ccnl_free(buf);
			ccnl_free(nonce);
			ccnl_free(ppkd);
		} else {
		    DEBUGMSG(6, "  not a content object\n");
		    ccnl_free(buf);
		}
	    }
	}
    }
}

// ----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int opt;
    int max_cache_entries = -1;
    int udpport = CCN_UDP_PORT;
    int tcpport = CCN_UDP_PORT;
    char *datadir = NULL;
    char *ethdev = NULL;
#ifdef USE_UNIXSOCKET
    char *uxpath = CCNL_DEFAULT_UNIXSOCKNAME;
#else
    char *uxpath = NULL;
#endif

    srandom(time(NULL));

    while ((opt = getopt(argc, argv, "hc:d:e:g:i:s:u:v:x:")) != -1) {
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
        case 's':
            tcpport = atoi(optarg);
            break;
        case 'u':
            udpport = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
            break;
        case 'x':
            uxpath = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr,
		    "usage: %s [-h] [-c MAX_CONTENT_ENTRIES]"
		    " [-d databasedir]"
		    " [-e ethdev]"
		    " [-g MIN_INTER_PACKET_INTERVAL]"
		    " [-i MIN_INTER_CCNMSG_INTERVAL]"
		    " [-s tcpport]"
		    " [-u udpport]"
		    " [-v DEBUG_LEVEL]"
#ifdef USE_UNIXSOCKET
		    " [-x unixpath]"
#endif
		    "\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    ccnl_relay_config(&theRelay, ethdev, udpport, tcpport,
		      uxpath, max_cache_entries);
    if (datadir)
	ccnl_populate_cache(&theRelay, datadir);

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
