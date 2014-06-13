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


#define USE_SUITE_CCNB

#include "../ccnl.h"

#include "ccnl-common.c"
#include "../pkt-ccnb.h"
#include "../pkt-ccnb-dec.c"
#include "../pkt-ccnb-enc.c"

#include "../pkt-ndntlv.h"
#include "../pkt-ndntlv-dec.c"
#include "../pkt-ndntlv-enc.c"


// ----------------------------------------------------------------------


#include "ccnl-socket.c"

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    int i = 0, len, opt, sock = 0;
    char *prefix[CCNL_MAX_NAME_COMP], *cp, *dest;
    char *udp = "127.0.0.1/9695", *ux = NULL;
    char *packettype = "CCNB";
    float wait = 3.0;
    int (*sendproc)(int,char*,unsigned char*,int);

    while ((opt = getopt(argc, argv, "hu:w:x:f:")) != -1) {
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
        case 'f':
        packettype = optarg;
        break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s "
	    "[-u host/port] [-x ux_path_name] [-w timeout] URI\n"
	    "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/9695)\n"
	    "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
        "  -f packet type [CCNB | NDNTLV | CCNTLV]"
	    "  -w timeout       in sec (float)\n",
	    argv[0]);
	    exit(1);
        }
    }

    if (!argv[optind]) 
	goto Usage;
    cp = strtok(argv[optind], "|");
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefix[i++] = cp;
        cp = strtok(NULL, "|");
    }
    prefix[i] = NULL;
    if(!strncmp(packettype, "CCNB", 4)){
        len = mkInterest(prefix, NULL, out);
    }
    else if(!strncmp(packettype, "NDNTLV", 6)){
        int tmplen = CCNL_MAX_PACKET_SIZE;
        len = ccnl_ndntlv_mkInterest(prefix, -1, &tmplen, out);
        memmove(out, out + tmplen, CCNL_MAX_PACKET_SIZE - tmplen);
        len = CCNL_MAX_PACKET_SIZE - tmplen;
    }

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
        request_content(sock, sendproc, dest, out, len, wait);
    //	fprintf(stderr, "retry\n");
    }
    close(sock);

    myexit(-1);
    return 0; // avoid a compiler warning
}

// eof
