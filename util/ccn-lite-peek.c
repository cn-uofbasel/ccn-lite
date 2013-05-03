/*
 * @f util/ccn-lite-peek.c
 * @b request content: send an interest, wait for reply, output to stdout
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../ccnx.h"
#include "../ccnl.h"

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
mkHeader(unsigned char *buf, unsigned int num, unsigned int tt)
{
    unsigned char tmp[100];
    int len = 0, i;

    *tmp = 0x80 | ((num & 0x0f) << 3) | tt;
    len = 1;
    num = num >> 4;

    while (num > 0) {
	tmp[len++] = num & 0x7f;
	num = num >> 7;
    }
    for (i = len-1; i >= 0; i--)
	*buf++ = tmp[i];
    return len;
}

int
mkInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = strlen(*namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    len += mkHeader(out+len, CCN_DTAG_MAXSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, 1, CCN_TT_UDATA);
    out[len++] = '1';
    out[len++] = 0; // end-of-maxsuffcomp

    if (nonce) {
	len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(unsigned int));
	len += sizeof(unsigned int);
	out[len++] = 0; // end-of-nonce
    }

    out[len++] = 0; // end-of-interest

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
    int sock;
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

    FD_ZERO(&readfs);
    FD_SET(sock, &readfs);
    timeout.tv_sec = wait;
    timeout.tv_usec = 1000000.0 * (wait - timeout.tv_sec);
    if (select(sock+1, &readfs, NULL, NULL, &timeout) < 0) {
	perror("select()");
	return -1;
    }
    return 0;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[8*1024];
    int i = 0, len, opt, sock = 0;
    char *prefix[32], *cp, *udp = "127.0.0.1/9695", *ux = NULL;
    float wait = 3.0;

    while ((opt = getopt(argc, argv, "hu:w:x:")) != -1) {
        switch (opt) {
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
	    "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/9695)\n"
	    "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
	    "  -w timeout       in sec (float)\n",
	    argv[0]);
	    exit(1);
        }
    }

    if (!argv[optind]) 
	goto Usage;
    cp = strtok(argv[optind], "/");
    while (i < 31 && cp) {
	prefix[i++] = cp;
	cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;
    len = mkInterest(prefix, NULL, out);

    if (ux) { // use UNIX socket
	sock = ux_open();
	len = ux_sendto(sock, ux, out, len);
    } else { // UDP
	sock = udp_open();
	len = udp_sendto(sock, udp, out, len);
    }
    if (len < 0) {
	perror("sendto");
	myexit(1);
    }

    /*
    for(; !block_on_read(sock, wait);) {
    */
	len = recv(sock, out, sizeof(out), 0);
	if (len > 0)
	    write(1, out, len);
	//    }

    close(sock);
    myexit(0);
    return 0; // avoid a compiler warning
}

// eof
