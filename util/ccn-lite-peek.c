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


// ----------------------------------------------------------------------


#include "ccnl-socket.c"

// ----------------------------------------------------------------------

int
hex2int(char c)
{
    if (c >= '0' && c <= '9')
	return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(unsigned char *comp) // inplace, returns len after shrinking
{
    unsigned char *in = comp, *out = comp;
    int len;

    for (len = 0; *in; len++) {
	if (in[0] != '%' || !in[1] || !in[2]) {
	    *out++ = *in++;
	    continue;
	}
	*out++ = hex2int(in[1])*16 + hex2int(in[2]);
	in += 3;
    }
    return len;
}

int
peek_mkInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;
    unsigned char *cp;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	cp = (unsigned char*) strdup(*namecomp);
	k = unescape_component(cp);
	//	k = strlen(*namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, cp, k);
	len += k;
	out[len++] = 0; // end-of-component
	free(cp);
	namecomp++;
    }
    out[len++] = 0; // end-of-name
    if (nonce) {
	len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(unsigned int));
	len += sizeof(unsigned int);
    }
    out[len++] = 0; // end-of-interest

    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    int i = 0, len, opt, sock = 0;
    char *prefix[CCNL_MAX_NAME_COMP], *cp, *dest;
    char *udp = "127.0.0.1/9695", *ux = NULL;
    float wait = 3.0;
    int (*sendproc)(int,char*,unsigned char*,int);

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
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
	prefix[i++] = cp;
	cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;
    len = peek_mkInterest(prefix, NULL, out);

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
