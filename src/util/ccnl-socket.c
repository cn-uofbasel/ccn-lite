/*
 * @f util/ccnl-socket.c
 * @b request content: send an interest open socket etc
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
 * 2014-03-17  created <christopher.scherb@unibas.ch>
 */


#include <ctype.h>
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

char *unix_path;

void
myexit(int rc)
{
    if (unix_path)
        unlink(unix_path);
    exit(rc);
}

int
udp_open()
{
  int s, bufsize;
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

    bufsize = CCNL_MAX_SOCK_SPACE;
    setsockopt(s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return s;
}

int
udp_sendto(int sock, char *dest, unsigned char *data, int len)
{
    struct sockaddr_in dst;
    char addr[256];
    unsigned int port;

    sscanf(dest, "%255[^/]/%u", addr, &port);
    ccnl_setIpSocketAddr(&dst, addr, port);

    return sendto(sock, data, len, 0, (struct sockaddr*) &dst, sizeof(dst));
}

// ----------------------------------------------------------------------

#ifdef USE_UNIXSOCKET

int
ux_open()
{
    static char mysockname[200];
    int sock, bufsize;
    struct sockaddr_un name;

    snprintf(mysockname, CCNL_ARRAY_SIZE(mysockname), "/tmp/.ccn-lite-%d.sock", getpid());
    unlink(mysockname);

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }

    ccnl_setUnixSocketPath(&name, mysockname);
    if (bind(sock, (struct sockaddr *) &name,
             sizeof(struct sockaddr_un))) {
        perror("binding path name to datagram socket");
        exit(1);
    }

    bufsize = CCNL_MAX_SOCK_SPACE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    unix_path = mysockname;
    return sock;
}

int ux_sendto(int sock, char *topath, unsigned char *data, int len)
{
    struct sockaddr_un name;

    ccnl_setUnixSocketPath(&name, topath);

    return sendto(sock, data, len, 0, (struct sockaddr*) &name,
                  sizeof(struct sockaddr_un));
}

#endif //USE_UNIXSOCKET

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
                char *dest, unsigned char *out, int len, float wait)
{
    unsigned char buf[64*1024];
    int len2 = sendproc(sock, dest, out, len), rc;

    if (len2 < 0) {
        perror("sendto");
        myexit(1);
    }

    rc = block_on_read(sock, wait);
    if (rc == 1) {
        len2 = recv(sock, buf, sizeof(buf), 0);
        if (len2 > 0) {

            write(1, buf, len2);
            myexit(0);
        }
    }
}
