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
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV
#define USE_SIGNATURES

#include "ccnl-common.c"
#include "ccnl-crypto.c"


// ----------------------------------------------------------------------

char *private_key_path; 
char *witness;

// ----------------------------------------------------------------------

#ifdef XXX
int
ccnl_ccnb_mkContent(char **namecomp,
      unsigned char *publisher, int plen,
      unsigned char *body, int blen,
      char *private_key_path,
      char *witness,
      unsigned char *out)
{
    int len = 0, k;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

    // add signature
#ifdef USE_SIGNATURES
    if(private_key_path)
        len += add_signature(out+len, private_key_path, body, blen);  
#endif
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    k = strlen(*namecomp);
    len += ccnl_ccnb_mkHeader(out+len, k, CCN_TT_BLOB);
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
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
    memcpy(out+len, tstamp, sizeof(tstamp));
    len += sizeof(tstamp);
    out[len++] = 0; // end-of-timestamp

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, plen, CCN_TT_BLOB);
    memcpy(out+len, publisher, plen);
    len += plen;
    out[len++] = 0; // end-of-publisher
    out[len++] = 0; // end-of-signedinfo
    }

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}

#endif

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{

    // char *private_key_path; 
    // char *witness;
    unsigned char body[64*1024];
    unsigned char out[65*1024];
    char *publisher = 0;
    char *infname = 0, *outfname = 0;
    unsigned int chunknum = UINT_MAX, lastchunknum = UINT_MAX;
    int f, len, opt, plen, offs = 0;
    struct ccnl_prefix_s *name;
    int suite = CCNL_SUITE_DEFAULT;
    private_key_path = 0;
    witness = 0;

    while ((opt = getopt(argc, argv, "hi:k:l:n:o:p:s:v:w:")) != -1) {
        switch (opt) {
        case 'i':
            infname = optarg;
            break;
        case 'k':
            private_key_path = optarg;
            break;
        case 'o':
            outfname = optarg;
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
        case 'l':
            lastchunknum = atoi(optarg);
            break;
        case 'n':
            chunknum = atoi(optarg);
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;

        case 'w':
            witness = optarg;
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (suite < 0 || suite >= CCNL_SUITE_LAST) {
                DEBUGMSG(ERROR, "Unsupported suite %d\n", suite);
                goto Usage;
            }
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, "usage: %s [options] URI [NFNexpr]\n"
        "  -i FNAME    input file (instead of stdin)\n"
        "  -k FNAME    publisher private key (CCNB)\n"
        "  -l LASTCHUNKNUM number of last chunk\n"       
        "  -n CHUNKNUM chunknum\n"
        "  -o FNAME    output file (instead of stdout)\n"
        "  -p DIGEST   publisher fingerprint\n"
        "  -s SUITE    (ccnb, ccnx2014, ndn2013)\n"
#ifdef USE_LOGGING
        "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, trace, verbose)\n"
#endif
        "  -w STRING   witness\n"
        "Examples:\n"
        "%% mkC /ndn/edu/wustl/ping             (classic lookup)\n"
        "%% mkC /th/ere  \"lambda expr\"          (lambda expr, in-net)\n"
        "%% mkC \"\" \"add 1 1\"                    (lambda expr, local)\n"
        "%% mkC /rpc/site \"call 1 /test/data\"   (lambda RPC, directed)\n",
        argv[0]);
        exit(1);
        }
    }

    if (!argv[optind]) 
        goto Usage;

    if (infname) {
        f = open(infname, O_RDONLY);
        if (f < 0)
            perror("file open:");
    } else
        f = 0;
    len = read(f, body, sizeof(body));
    close(f);

    name = ccnl_URItoPrefix(argv[optind], suite, argv[optind+1],
                            chunknum == UINT_MAX ? NULL : &chunknum);

    switch (suite) {
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, body, len, NULL, out);
        break;
    case CCNL_SUITE_CCNTLV:
        offs = CCNL_MAX_PACKET_SIZE;
        len = ccnl_ccntlv_prependContentWithHdr(name, body, len, 
            lastchunknum == UINT_MAX ? NULL : &lastchunknum, 
            &offs, 
            NULL, // Int *contentpos
            out);
        break;
    case CCNL_SUITE_NDNTLV:
        offs = CCNL_MAX_PACKET_SIZE;
        len = ccnl_ndntlv_prependContent(name, body, len, &offs,
                                         NULL, 
                                         lastchunknum == UINT_MAX ? NULL : &lastchunknum, 
                                         out);
        break;
    default:
        break;
    }

    if (outfname) {
        f = creat(outfname, 0666);
        if (f < 0)
            perror("file open:");
    } else
        f = 1;
    write(f, out + offs, len);
    close(f);

    return 0;
}

// eof
