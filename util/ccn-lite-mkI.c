/*
 * @f util/ccn-lite-mkI.c
 * @b CLI mkInterest, write to Stdout
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
 * 2013-07-06  created
 */

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV

#include "ccnl-common.c"

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
        case 'x':
            maxSuffix = optarg;
            break;
        case 's':
            packettype = ccnl_str2suite(optarg);
            if (packettype >= 0 && packettype < CCNL_SUITE_LAST)
                break;
        case 'h':
        default:
Usage:
            fprintf(stderr, "usage: %s [options] URI [NFNexpr]\n"
            "  -c SCOPE\n"
            "  -d DIGEST  content digest (sets -x to 0)\n"
            "  -l         URI is a Lambda expression\n"
            "  -n LEN     miN additional components\n"
            "  -o FNAME   output file (instead of stdout)\n"
            "  -p DIGEST  publisher fingerprint\n"
            "  -s SUITE   (ccnb, ccnx2014, ndn2013)\n"
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
    prefix = ccnl_URItoPrefix(argv[optind], packettype, argv[optind+1], NULL);
    if (!prefix) {
        fprintf(stderr, "no URI found, aborting\n");
        return -1;
    }

    switch (packettype) {
    case CCNL_SUITE_CCNB:
    	len = ccnl_ccnb_mkInterest(prefix, minSuffix, maxSuffix,
                                   (unsigned char*) digest, dlen,
                                   (unsigned char*) publisher, plen,
                                   scope, &nonce, out);
	break;
    case CCNL_SUITE_CCNTLV:
        len = ccntlv_mkInterest(prefix, 
                                (int*)&nonce, 
                                out, CCNL_MAX_PACKET_SIZE);
	break;
    case CCNL_SUITE_NDNTLV:
        len = ndntlv_mkInterest(prefix, 
                                (int*)&nonce,
                                out,
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
