/*
 * @f util/ccn-lite-mkC.c
 * @b CLI mkContent, write to Stdout
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
 * 2013-07-06  created
 */

#define USE_SUITE_CCNB
#define USE_SIGNATURES

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h> // htonl()
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../ccnl.h"

#include "../pkt-ccnb.h"
#include "../pkt-ccnb-enc.c"

#include "../pkt-ndntlv.h"
#include "../pkt-ndntlv-enc.c"

#include "ccnl-crypto.c"



// ----------------------------------------------------------------------

char *private_key_path; 
char *witness;

// ----------------------------------------------------------------------

int
mkContent(char **namecomp,
	  unsigned char *publisher, int plen,
	  unsigned char *body, int blen,
	  unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

    // add signature
#ifdef USE_SIGNATURES
    if(private_key_path)
        len += add_signature(out+len, private_key_path, body, blen);  
#endif
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = unescape_component((unsigned char*) *namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    if (publisher) {
	struct timeval t;
	unsigned char tstamp[6];
	uint32_t *sec;
	uint16_t *secfrac;

	gettimeofday(&t, NULL);
	sec = (uint32_t*)(tstamp + 0); // big endian
	*sec = htonl(t.tv_sec);
	secfrac = (uint16_t*)(tstamp + 4);
	*secfrac = htons(4048L * t.tv_usec / 1000000);
	len += mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
	memcpy(out+len, tstamp, sizeof(tstamp));
	len += sizeof(tstamp);
	out[len++] = 0; // end-of-timestamp

	len += mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
	len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
	len += mkHeader(out+len, plen, CCN_TT_BLOB);
	memcpy(out+len, publisher, plen);
	len += plen;
	out[len++] = 0; // end-of-publisher
	out[len++] = 0; // end-of-signedinfo
    }

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char body[64*1024];
    unsigned char out[65*1024];
    unsigned char *publisher = 0;
    char *infname = 0, *outfname = 0;
    int i = 0, f, len, opt, plen;
    char *prefix[CCNL_MAX_NAME_COMP], *cp;
    char *packettype = "CCNB";
    private_key_path = 0;
    witness = 0;

    while ((opt = getopt(argc, argv, "hi:o:p:k:w:f:")) != -1) {
        switch (opt) {
        case 'i':
            infname = optarg;
            break;
        case 'o':
            outfname = optarg;
            break;
        case 'f':
            packettype = optarg;
            break;
        case 'k':
            private_key_path = optarg;
            break;
        case 'w':
            witness = optarg;
            break;
        case 'p':
            publisher = (unsigned char*)optarg;
            plen = unescape_component(publisher);
            if (plen != 32) {
            fprintf(stderr,
             "publisher key digest has wrong length (%d instead of 32)\n",
             plen);
            exit(-1);
            }
            break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s [options] URI\n"
	    "  -i FNAME   input file (instead of stdin)\n"
	    "  -o FNAME   output file (instead of stdout)\n"
	    "  -p DIGEST  publisher fingerprint\n"
        "  -k FNAME   publisher private key\n"
        "  -f packet type [CCNB | NDNTLV | CCNTLV]"
        "  -w STRING  witness\n"       ,
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

    if (infname) {
	f = open(infname, O_RDONLY);
      if (f < 0)
	perror("file open:");
    } else
      f = 0;
    len = read(f, body, sizeof(body));
    close(f);

    if(!strncmp(packettype, "CCNB", 4)){
        len = mkContent(prefix, publisher, plen, body, len, out);
    }
    else if(!strncmp(packettype, "NDNTLV", 6)){
        int len2 = CCNL_MAX_PACKET_SIZE;
        len = ccnl_ndntlv_mkContent(prefix, body, len, &len2, out);
        memmove(out, out+len2, CCNL_MAX_PACKET_SIZE - len2);
        len = CCNL_MAX_PACKET_SIZE - len2;
    }

    if (outfname) {
	f = creat(outfname, 0666);
      if (f < 0)
	perror("file open:");
    } else
      f = 1;
    write(f, out, len);
    close(f);

    return 0;
}

// eof
