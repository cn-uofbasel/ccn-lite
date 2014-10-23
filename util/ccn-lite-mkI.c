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

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV

#include "ccnl-common.c"

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
ccnb_mkInterest(struct ccnl_prefix_s *name,
                char *minSuffix, char *maxSuffix,
                unsigned char *digest, int dlen,
                unsigned char *publisher, int plen,
                char *scope,
                uint32_t *nonce,
                unsigned char *out)
{
  int len = 0, i, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest

    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    for (i = 0; i < name->compcnt; i++) {
        len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
        k = name->complen[i];
        len += mkHeader(out+len, k, CCN_TT_BLOB);
        memcpy(out+len, name->comp[i], k);
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

int
ccntlv_mkInterest(struct ccnl_prefix_s *name,
                  char *scope, unsigned char *out, int outlen)
{
     int len, offset, oldoffset;

     offset = oldoffset = outlen;
     len = ccnl_ccntlv_fillInterest(name, &offset, out);
     ccnl_ccntlv_prependFixedHdr(0, 1,
				 len, 255, &offset, out);
     len = oldoffset - offset;
     if (len > 0)
         memmove(out, out + offset, len);

     return len;
}

int
ndntlv_mkInterest(struct ccnl_prefix_s *name, char *scope, int *nonce,
                  unsigned char *out, int outlen)
{
    int len, offset;
    int s = scope ? atoi(scope) : -1;

    offset = outlen;
    len = ccnl_ndntlv_fillInterest(name, s, nonce, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    char *minSuffix = 0, *maxSuffix = 0, *scope = 0;
    char *digest = 0, *publisher = 0;
    char *fname = 0;
    int f, len=0, opt;
    int dlen = 0, plen = 0;
    int packettype = 2;
    struct ccnl_prefix_s *prefix;
    uint32_t nonce;
    int isLambda = 0;

    time((time_t*) &nonce);

    while ((opt = getopt(argc, argv, "hc:d:ln:o:p:s:x:")) != -1) {
        switch (opt) {
        case 'c':
            scope = optarg;
            break;
        case 'd':
            digest = optarg;
            dlen = unescape_component(digest);
            if (dlen != 32) {
                fprintf(stderr, "digest has wrong length (%d instead of 32)\n",
                        dlen);
                exit(-1);
            }
            break;
        case 'l':
            isLambda = 1 - isLambda;
            break;
        case 'n':
            minSuffix = optarg;
            break;
        case 'o':
            fname = optarg;
            break;
        case 'p':
            publisher = optarg;
            plen = unescape_component(publisher);
            if (plen != 32) {
                fprintf(stderr,
                 "publisher key digest has wrong length (%d instead of 32)\n",
                 plen);
                exit(-1);
            }
            break;
        case 's':
            packettype = atoi(optarg);
            break;
        case 'x':
            maxSuffix = optarg;
            break;
        case 'h':
        default:
Usage:
            fprintf(stderr, "usage: %s [options] URI\n"
            "  -c SCOPE\n"
            "  -d DIGEST  content digest (sets -x to 0)\n"
            "  -l         URI is a Lambda expression\n"
            "  -n LEN     miN additional components\n"
            "  -o FNAME   output file (instead of stdout)\n"
            "  -p DIGEST  publisher fingerprint\n"
            "  -s SUITE   0=ccnb, 1=ccntlv, 2=ndntlv (default)\n"
            "  -x LEN     maX additional components\n",
            argv[0]);
            exit(1);
        }
    }

    if (!argv[optind]) 
        goto Usage;

    /*
    if (isLambda)
        i = ccnl_lambdaStrToComponents(prefix, argv[optind]);
    else
    */
    prefix = ccnl_URItoPrefix(argv[optind], packettype, NULL);
    if (!prefix) {
        fprintf(stderr, "no URI found, aborting\n");
        return -1;
    }

    switch (packettype) {
   	case CCNL_SUITE_CCNB:
    	len = ccnb_mkInterest(prefix, minSuffix, maxSuffix,
			      (unsigned char*) digest, dlen,
			      (unsigned char*) publisher, plen,
			      scope, &nonce, out);
	break;
    case CCNL_SUITE_CCNTLV:
        len = ccntlv_mkInterest(prefix, scope, out,
				CCNL_MAX_PACKET_SIZE);
	break;
    case CCNL_SUITE_NDNTLV:
        len = ndntlv_mkInterest(prefix, scope, (int*)&nonce, out,
                                CCNL_MAX_PACKET_SIZE);
        break;
    default:
        fprintf(stderr, "Not Implemented (yet)\n");
        return -1;
    }

    if (len <= 0) {
        fprintf(stderr, "internal error: empty packet\n");
        return -1;
    }

    if (fname) {
        f = creat(fname, 0666);
        if (f < 0) {
            perror("file open:");
            return -1;
        }
    } else
        f = 1;

    write(f, out, len);
    close(f);

    return 0;
}

// eof
