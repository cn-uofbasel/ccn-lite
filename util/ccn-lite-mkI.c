/*
 * @f util/ccn-lite-mkI.c
 * @b CLI mkInterest, write to Stdout
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

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../ccnx.h"
#include "../ccnl.h"

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
mkInterest(char **namecomp,
	   char *minSuffix, char *maxSuffix,
	   unsigned char *digest, int dlen,
	   unsigned char *publisher, int plen,
	   char *scope,
	   uint32_t *nonce,
	   unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest

    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = unescape_component((unsigned char*) *namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    if (digest) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	len += mkHeader(out+len, dlen, CCN_TT_BLOB);
	memcpy(out+len, digest, dlen);
	len += dlen;
	out[len++] = 0; // end-of-component
	if (!maxSuffix)
	    maxSuffix = "0";
    }
    out[len++] = 0; // end-of-name

    if (minSuffix) {
	k = strlen(minSuffix);
	len += mkHeader(out+len, CCN_DTAG_MINSUFFCOMP, CCN_TT_DTAG);
	len += mkHeader(out+len, k, CCN_TT_UDATA);
	memcpy(out + len, minSuffix, k);
	len += k;
	out[len++] = 0; // end-of-minsuffcomp
    }

    if (maxSuffix) {
	k = strlen(maxSuffix);
	len += mkHeader(out+len, CCN_DTAG_MAXSUFFCOMP, CCN_TT_DTAG);
	len += mkHeader(out+len, k, CCN_TT_UDATA);
	memcpy(out + len, maxSuffix, k);
	len += k;
	out[len++] = 0; // end-of-maxsuffcomp
    }

    if (publisher) {
	len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
	len += mkHeader(out+len, plen, CCN_TT_BLOB);
	memcpy(out+len, publisher, plen);
	len += plen;
	out[len++] = 0; // end-of-component
    }

    if (scope) {
	k = strlen(scope);
	len += mkHeader(out+len, CCN_DTAG_SCOPE, CCN_TT_DTAG);
	len += mkHeader(out+len, k, CCN_TT_UDATA);
	memcpy(out + len, (unsigned char*)scope, k);
	len += k;
	out[len++] = 0; // end-of-maxsuffcomp
    }

    if (nonce) {
	len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(*nonce), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(*nonce));
	len += sizeof(*nonce);
	out[len++] = 0; // end-of-nonce
    }

    out[len++] = 0; // end-of-interest

    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[8*1024];
    char *minSuffix = 0, *maxSuffix = 0, *scope;
    unsigned char *digest = 0, *publisher = 0;
    char *fname = 0;
    int i = 0, f, len, opt;
    int dlen = 0, plen = 0;
    char *prefix[CCNL_MAX_NAME_COMP], *cp;
    uint32_t nonce;

    time((time_t*) &nonce);

    while ((opt = getopt(argc, argv, "hd:n:o:p:s:x:")) != -1) {
        switch (opt) {
        case 'd':
	    digest = (unsigned char*)optarg;
	    dlen = unescape_component(digest);
	    if (dlen != 32) {
		fprintf(stderr, "digest has wrong length (%d instead of 32)\n",
			dlen);
		exit(-1);
	    }
	    break;
        case 'n':
	    minSuffix = optarg;
	    break;
        case 'o':
	    fname = optarg;
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
        case 's':
	    scope = optarg;
	    break;
        case 'x':
	    maxSuffix = optarg;
	    break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s [options] URI\n"
	    "  -d DIGEST  content digest (sets -x to 0)\n"
	    "  -n LEN     miN additional components\n"
	    "  -o FNAME   output file (instead of stdout)\n"
	    "  -p DIGEST  publisher fingerprint\n"
	    "  -x LEN     maX additional components\n",
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
    len = mkInterest(prefix,
		     minSuffix, maxSuffix,
		     digest, dlen,
		     publisher, plen,
		     scope, &nonce,
		     out);

    if (fname) {
	f = creat(fname, 0666);
      if (f < 0)
	perror("file open:");
    } else
      f = 1;

    write(f, out, len);
    close(f);

    return 0;
}

// eof
