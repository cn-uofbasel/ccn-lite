/*
 * @f util/ccn-lite-mkI.c
 * @b CLI mkInterest, write to Stdout
 *
 * Copyright (C) 2013-15, Christian Tschudin, University of Basel
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
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV

#define NEEDS_PACKET_CRAFTING

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
    int packettype = CCNL_SUITE_NDNTLV;
    struct ccnl_prefix_s *prefix;
    time_t curtime;
    uint32_t nonce;
    int isLambda = 0;
    unsigned int chunknum = UINT_MAX;

    time(&curtime);
    // Get current time in double to avoid dealing with time_t
    nonce = (uint32_t) difftime(curtime, 0);

    while ((opt = getopt(argc, argv, "ha:c:d:e:i:ln:o:p:s:v:x:")) != -1) {
        switch (opt) {
        case 'a':
            minSuffix = optarg;
            break;
        case 'c':
            scope = optarg;
            break;
        case 'd':
            digest = optarg;
            dlen = unescape_component(digest);
            if (dlen != 32) {
                DEBUGMSG(ERROR, "digest has wrong length (%d instead of 32)\n",
                        dlen);
                exit(-1);
            }
            break;
        case 'e':
            nonce = atoi(optarg);
            break;
        case 'l':
            isLambda = 1 - isLambda;
            break;
        case 'n':
            chunknum = atoi(optarg);
            break;
        case 'o':
            fname = optarg;
            break;
        case 'p':
            publisher = optarg;
            plen = unescape_component(publisher);
            if (plen != 32) {
                DEBUGMSG(ERROR,
                 "publisher key digest has wrong length (%d instead of 32)\n",
                 plen);
                exit(-1);
            }
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
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
            "  -a LEN     miN additional components\n"
            "  -c SCOPE\n"

            "  -d DIGEST  content digest (sets -x to 0)\n"
            "  -e NONCE   random 4 bytes\n"
            "  -l         URI is a Lambda expression\n"
            "  -n CHUNKNUM positive integer for chunk interest\n"
            "  -o FNAME   output file (instead of stdout)\n"
            "  -p DIGEST  publisher fingerprint\n"
            "  -s SUITE   (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
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
    prefix = ccnl_URItoPrefix(argv[optind],
                              packettype,
                              argv[optind+1],
                              chunknum == UINT_MAX ? NULL : &chunknum);
    if (!prefix) {
        DEBUGMSG(ERROR, "no URI found, aborting\n");
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
    case CCNL_SUITE_CISTLV:
        len = cistlv_mkInterest(prefix,
                                (int*)&nonce,
                                out, CCNL_MAX_PACKET_SIZE);
	break;
    case CCNL_SUITE_IOTTLV:
        len = iottlv_mkRequest(prefix, NULL, out, CCNL_MAX_PACKET_SIZE);
        break;
    case CCNL_SUITE_NDNTLV:
        len = ndntlv_mkInterest(prefix,
                                (int*)&nonce,
                                out,
                                CCNL_MAX_PACKET_SIZE);
        break;
    default:
        DEBUGMSG(ERROR, "Not Implemented (yet)\n");
        return -1;
    }

    if (len <= 0) {
        DEBUGMSG(ERROR, "internal error: empty packet\n");
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
