/*
 * @f util/ccn-lite-mkC.c
 * @b CLI mkContent, write to Stdout
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


//#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.h"
#include "ccnl-crypto.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-ext-hmac.h"

#ifndef CCN_LITE_MKC_OUT_SIZE
#define CCN_LITE_MKC_OUT_SIZE (65 * 1024)
#endif

#ifndef CCN_LITE_MKC_BODY_SIZE
#define CCN_LITE_MKC_BODY_SIZE (64 * 1024)
#endif

// ----------------------------------------------------------------------

char *private_key_path;
char *witness;

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char body[CCN_LITE_MKC_BODY_SIZE];
    unsigned char out[CCN_LITE_MKC_OUT_SIZE];
    unsigned char *publisher = NULL;
    char *infname = 0, *outfname = 0;
    unsigned int chunknum = UINT_MAX, lastchunknum = UINT_MAX;
    int f, opt;
    size_t  plen, len, offs = 0;
    int contentpos;
    struct ccnl_prefix_s *name;
    int suite = CCNL_SUITE_DEFAULT;
    struct key_s *keys = NULL;
    ccnl_data_opts_u data_opts;

    (void)contentpos;
    (void)keys;
    (void)lastchunknum;
    (void)chunknum;
    (void)data_opts;

    while ((opt = getopt(argc, argv, "hg:i:k:l:n:o:p:s:v:w:")) != -1) {
        switch (opt) {
        case 'i':
            infname = optarg;
            break;
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'l':
            lastchunknum = (int)strtol(optarg, (char**)NULL, 10);
            break;
        case 'n':
            chunknum = (int)strtol(optarg, (char**)NULL, 10);
            break;
        case 'o':
            outfname = optarg;
            break;
        case 'p':
            publisher = (unsigned char*) optarg;
            plen = unescape_component((char*) publisher);
            if (plen != 32) {
                DEBUGMSG(ERROR, "publisher key digest has wrong length (%zu instead of 32)\n", plen);
                exit(-1);
            }
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite)) {
                DEBUGMSG(ERROR, "Unsupported suite %d\n", suite);
                goto Usage;
            }
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = (int)strtol(optarg, (char**)NULL, 10);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;

        case 'w':
            witness = optarg;
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, "usage: %s [options] URI\n"
        "  -i FNAME    input file (instead of stdin)\n"
        "  -k FNAME    HMAC256 key (base64 encoded)\n"
        "  -l LASTCHUNKNUM number of last chunk\n"
        "  -n CHUNKNUM chunknum\n"
        "  -o FNAME    output file (instead of stdout)\n"
        "  -p DIGEST   publisher fingerprint\n"
        "  -s SUITE    (ccnb, ccnx2015, ndn2013)\n"
#ifdef USE_LOGGING
        "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
        "  -w STRING   witness\n"
        "Examples:\n"
        "%% mkC /ndn/edu/wustl/ping             (classic lookup)\n"
        "%% mkC /rpc/site \"call 1 /test/data\"   (lambda RPC, directed)\n",
        argv[0]);
        exit(1);
        }
    }

    if (!argv[optind]) {
        goto Usage;
    }

    if (infname) {
        f = open(infname, O_RDONLY);
        if (f < 0) {
            perror("file open:");
        }
    } else {
        f = 0;
    }
    ssize_t _len = read(f, body, sizeof(body));
    if (_len < 0) {
        DEBUGMSG(ERROR, "read: %d\n", errno);
        exit(1);
    }
    len = (size_t) _len;
    close(f);
    memset(out, 0, sizeof(out));

    name = ccnl_URItoPrefix(argv[optind], suite, 
                            chunknum == UINT32_MAX ? NULL : &chunknum);

    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        if (ccnl_ccnb_fillContent(name, body, len, NULL, out, out + sizeof(out), &len)) {
            DEBUGMSG(ERROR, "Error: Failed creating content object.");
            exit(1);
        }
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:

        offs = CCNL_MAX_PACKET_SIZE;
        if (keys) {
            uint8_t keyval[64];
            uint8_t keyid[32];
            // use the first key found in the key file
            if (keys->keylen < 0) {
                DEBUGMSG(ERROR, "Error: Invalid key length: %d", keys->keylen);
                exit(1);
            }
            ccnl_hmac256_keyval(keys->key, (size_t) keys->keylen, keyval);
            ccnl_hmac256_keyid(keys->key, (size_t) keys->keylen, keyid);
            if (ccnl_ccntlv_prependSignedContentWithHdr(name, body, len,
                                                        lastchunknum == UINT32_MAX ? NULL : &lastchunknum,
                                                        NULL, keyval, keyid, &offs, out, &len)) {
                DEBUGMSG(ERROR, "Error: Failed prepending signed content.");
                exit(1);
            }
        } else {
            if (ccnl_ccntlv_prependContentWithHdr(name, body, len,
                                                  lastchunknum == UINT32_MAX ? NULL : &lastchunknum,
                                                  NULL /* Int *contentpos */, &offs, out, &len)) {
                DEBUGMSG(ERROR, "Error: Failed prepending content.");
                exit(1);
            }
        }
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        offs = CCNL_MAX_PACKET_SIZE;
        if (keys) {
            uint8_t keyval[64];
            uint8_t keyid[32];
            // use the first key found in the key file
            if (keys->keylen < 0) {
                DEBUGMSG(ERROR, "Error: Invalid key length: %d", keys->keylen);
                exit(1);
            }
            ccnl_hmac256_keyval(keys->key, (size_t) keys->keylen, keyval);
            ccnl_hmac256_keyid(keys->key, (size_t) keys->keylen, keyid);
            if (ccnl_ndntlv_prependSignedContent(name, body, len,
                  lastchunknum == UINT32_MAX ? NULL : &lastchunknum,
                  NULL, keyval, keyid, &offs, out, &len)) {
                DEBUGMSG(ERROR, "Error: Failed prepending signed content.");
                exit(1);
            }
        } else {
            data_opts.ndntlv.finalblockid = lastchunknum;
            if (ccnl_ndntlv_prependContent(name, body, len,
                  NULL, lastchunknum == UINT32_MAX ? NULL : &(data_opts.ndntlv),
                                             &offs, out, &len)) {
                DEBUGMSG(ERROR, "Error: Failed prepending content.");
                exit(1);
            }
            //TODO: new implementation of prependContent returns -1 or 0, not length
            printf("pkt len: %zu\n", len);
        }
        break;
#endif
    default:
        break;
    }

    if (outfname) {
        f = creat(outfname, 0666);
        if (f < 0) {
            perror("file open:");
        }
    } else {
        f = 1;
    }
    write(f, out + offs, len);
    close(f);

    return 0;
}

// eof
