/*
 * @f util/ccn-lite-peek.c
 * @b request content: send an interest, wait for reply, output to stdout
 *
 * Copyright (C) 2013-14, Christian Tschudin, University of Basel
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
 * 2013-04-06  created
 * 2014-06-18  added NDNTLV support
 */

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#define USE_SUITE_CCNB
#define USE_SUITE_NDNTLV

#include "../ccnl.h"

#include "ccnl-common.c"

#ifdef USE_SUITE_CCNB
# include "../pkt-ccnb.h"
#endif

#ifdef USE_SUITE_NDNTLV
# include "../pkt-ndntlv.h"
#endif

#include "../ccnl-util.c"

#ifdef USE_SUITE_CCNB
# include "../pkt-ccnb-dec.c"
# include "../pkt-ccnb-enc.c"
#endif

#ifdef USE_SUITE_NDNTLV
# include "../pkt-ndntlv-dec.c"
# include "../pkt-ndntlv-enc.c"
#endif


// ----------------------------------------------------------------------

char *unix_path;

void
myexit(int rc)
{
    if (unix_path)
	unlink(unix_path);
    exit(rc);
}

// ----------------------------------------------------------------------

int
peek_mkCcnbInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;
    unsigned char *cp;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
	len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	cp = (unsigned char*) strdup(*namecomp);
	k = unescape_component(cp);
	//	k = strlen(*namecomp);
	len += ccnl_ccnb_mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, cp, k);
	len += k;
	out[len++] = 0; // end-of-component
	free(cp);
	namecomp++;
    }
    out[len++] = 0; // end-of-name
    if (nonce) {
	len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += ccnl_ccnb_mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(unsigned int));
	len += sizeof(unsigned int);
    }
    out[len++] = 0; // end-of-interest

    return len;
}


int
peek_mkNdntlvInterest(char **namecomp, unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ndntlv_mkInterest(namecomp, -1, &offset, out);
    memmove(out, out + offset, len);

    return len;
}



// ----------------------------------------------------------------------

int
udp_open()
{
    int s;
    struct sockaddr_in si;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	perror("udp socket");
	exit(1);
    }
    si.sin_addr.s_addr = INADDR_ANY;
    si.sin_port = htons(0);
    si.sin_family = PF_INET;
    if (bind(s, (struct sockaddr *)&si, sizeof(si)) < 0) {
        perror("udp sock bind");
	exit(1);
    }

    return s;
}

int
udp_sendto(int sock, char *dest, unsigned char *data, int len)
{
    struct sockaddr_in dst;
    char buf[256];
    strcpy(buf, dest);

    dst.sin_family = PF_INET;
    dst.sin_addr.s_addr = inet_addr(strtok(buf, "/"));
    dst.sin_port = htons(atoi(strtok(NULL, "/")));

    return sendto(sock, data, len, 0, (struct sockaddr*) &dst, sizeof(dst));
}

// ----------------------------------------------------------------------

int
ux_open()
{
static char mysockname[200];
 int sock, bufsize;
    struct sockaddr_un name;

    sprintf(mysockname, "/tmp/.ccn-lite-peek-%d.sock", getpid());
    unlink(mysockname);

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
	perror("opening datagram socket");
	exit(1);
    }
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, mysockname);
    if (bind(sock, (struct sockaddr *) &name,
	     sizeof(struct sockaddr_un))) {
	perror("binding path name to datagram socket");
	exit(1);
    }

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    unix_path = mysockname;
    return sock;
}

int ux_sendto(int sock, char *topath, unsigned char *data, int len)
{
    struct sockaddr_un name;

    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, topath);

    return sendto(sock, data, len, 0, (struct sockaddr*) &name,
		  sizeof(struct sockaddr_un));
}

// ----------------------------------------------------------------------

int
block_on_read(int sock, float wait)
{
    fd_set readfs;
    struct timeval timeout;
    int rc;

    FD_ZERO(&readfs);
    FD_SET(sock, &readfs);
    timeout.tv_sec = wait;
    timeout.tv_usec = 1000000.0 * (wait - timeout.tv_sec);
    rc = select(sock+1, &readfs, NULL, NULL, &timeout);
    if (rc < 0)
	perror("select()");
    return rc;
}

void
request_content(int sock, int (*sendproc)(int,char*,unsigned char*,int),
		char *dest, unsigned char *out, int len, float wait, int suite)
{
    unsigned char buf[64*1024], *cp;
    int len2 = sendproc(sock, dest, out, len), typ, dummy;

    if (len2 < 0) {
	perror("sendto");
	myexit(1);
    }
    
    for (;;) {
	if (block_on_read(sock, wait) <= 0)
	    break;
	len2 = recv(sock, buf, sizeof(buf), 0);
	fprintf(stderr, "received %d bytes\n", len2);
	cp = buf;
	len = len2;
	switch(ccnl_pkt2suite(buf, len)) {
#ifdef USE_SUITE_CCNB
	case CCNL_SUITE_CCNB:
	    if (len2 < 0 || ccnl_ccnb_dehead(&cp, &len, &dummy, &typ))
		return;
	    if (typ != CCN_TT_DTAG || dummy != CCN_DTAG_CONTENTOBJ)
		continue;
	    break;
#endif

#ifdef USE_SUITE_NDNTLV
	case CCNL_SUITE_NDNTLV:
	    if (len2 < 0 || ccnl_ndntlv_dehead(&cp, &len, &typ, &dummy))
		return;
	    if (typ != NDN_TLV_Data)
		continue;
	    break;
#endif
	default:
	    return;
	}
	write(1, buf, len2);
	myexit(0);
    }
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[8*1024];
    int i = 0, len, opt, sock = 0, suite = 2;
    char *prefix[CCNL_MAX_NAME_COMP], *cp, *dest;
//    char *udp = "127.0.0.1/9695", *ux = NULL;
    char *udp = "127.0.0.1/6363", *ux = NULL;
    float wait = 3.0;
    int (*sendproc)(int,char*,unsigned char*,int);

    while ((opt = getopt(argc, argv, "hs:u:w:x:")) != -1) {
        switch (opt) {
        case 's':
	    suite = atoi(optarg);
	    break;
        case 'u':
	    udp = optarg;
	    break;
        case 'w':
	    wait = atof(optarg);
	    break;
        case 'x':
	    ux = optarg;
	    break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s "
	    "[-u host/port] [-x ux_path_name] [-w timeout] URI\n"
	    "  -s SUITE         0=ccnb, 1=ccntlv, 2=ndntlv (default)\n"
	    "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/6363)\n"
	    "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
	    "  -w timeout       in sec (float)\n"
	    "Example URI: /ndn/edu/wustl/ping\n",
	    argv[0]);
	    exit(1);
        }
    }

    if (!argv[optind]) 
	goto Usage;

    srandom(time(NULL)); // random() is  used in mkNdntlvInterest()

    cp = strtok(argv[optind], "/");
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
	prefix[i++] = cp;
	cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;
    if (suite == 0)
	len = ccnl_ccnb_mkInterest(prefix, NULL, out);
    else if (suite == 1) {
	printf("CCNx-TLV not supported at this time, aborting\n");
	exit(-1);
    } else
	len = peek_mkNdntlvInterest(prefix, out, sizeof(out));

    i = creat("pkt.bin", S_IRWXU);
    write(i, out, len);
    close(i);

    if (ux) { // use UNIX socket
	dest = ux;
	sock = ux_open();
	sendproc = ux_sendto;
    } else { // UDP
	dest = udp;
	sock = udp_open();
	sendproc = udp_sendto;
    }

    for (i = 0; i < 3; i++) {
	request_content(sock, sendproc, dest, out, len, wait, suite);
    }
    close(sock);

    myexit(-1);
    return 0; // avoid a compiler warning
}

// eof
