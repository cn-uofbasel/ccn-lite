/*
 * @f util/ccn-lite-flic.c
 * @b utility for FLIC (FLIC = File-LIke-Collection)
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-11-10  created
 */

#define USE_SUITE_CCNTLV
// #define USE_SUITE_CISTLV
// #define USE_SUITE_IOTTLV
// #define USE_SUITE_NDNTLV

#define USE_CCNxMANIFEST
#define USE_HMAC256

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"

// ----------------------------------------------------------------------

char *progname;

void
usage(int exitval)
{
    fprintf(stderr, "usage 1: %s [options] URI                  (fetch)\n"
            "usage 2: %s -p FILENAMEPREFIX [options] FILE [URI] (produce)\n"
            "  -k FNAME    HMAC256 key (base64 encoded)\n"
//            "  -s SUITE   (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)\n"
            "  -s SUITE   (ccnx2015)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            , progname, progname);
    exit(exitval);
}


// ----------------------------------------------------------------------

void
flic_fetch()
{
    fprintf(stderr, "fetch not implemented yet\n");
    exit(1);
}

// ----------------------------------------------------------------------

int
flic_produce(int pktype, char *outprefix, struct key_s *keys,
             char *fname, char *uri)
{
    unsigned char body[64*1024], out[64*1024];
    int i, f, len, offs, msgLen;
    struct ccnl_prefix_s *name = NULL;
    SHA256_CTX_t ctx;
    unsigned char md[SHA256_DIGEST_LENGTH];

    fprintf(stderr, "produce ...\n");

    if (fname) {
        f = open(fname, O_RDONLY);
        if (f < 0) {
            perror("file open:");
            return -1;
        }
    } else
        f = 0;
    len = read(f, body, sizeof(body));
    close(f);
    memset(out, 0, sizeof(out));

    if (uri) {
        name = ccnl_URItoPrefix(uri, CCNL_SUITE_CCNTLV, NULL, NULL);
        if (!name)
            return -1;
    }

    offs = sizeof(out);

    if (keys) {
            unsigned char keyval[64];
            unsigned char keyid[32];
            // use the first key found in the key file
            ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
            ccnl_hmac256_keyid(keys->key, keys->keylen, keyid);
            len = ccnl_ccntlv_prependSignedManifestWithHdr(name, body, len,
                                      keyval, keyid, &offs, out, &msgLen);
    } else
        len = ccnl_ccntlv_prependManifestWithHdr(name, body, len, &offs, out,
                                                                 &msgLen);

    if (len <= 0) {
        DEBUGMSG(ERROR, "internal error: empty packet\n");
        return -1;
    }

    if (outprefix) {
        f = creat(outprefix, 0666);
        if (f < 0) {
            perror("file open:");
            return -1;
        }
    } else
        f = 1;

    write(f, out + offs, len);
    close(f);

    ccnl_SHA256_Init(&ctx);
    ccnl_SHA256_Update(&ctx, out + sizeof(out) - msgLen, msgLen);
    ccnl_SHA256_Final(md, &ctx);
    fprintf(stderr, "  ObjectHash%d is 0x", SHA256_DIGEST_LENGTH);
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
        fprintf(stderr, "%02x", md[i]);
    fprintf(stderr, "\n");

    return 0;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    char *outprefix = 0;
    int opt, packettype = CCNL_SUITE_CCNTLV;
    struct key_s *keys = NULL;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hk:p:s:v:")) != -1) {
        switch (opt) {
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'p':
            outprefix = optarg;
            break;
        case 's':
            packettype = ccnl_str2suite(optarg);
            if (packettype >= 0 && packettype < CCNL_SUITE_LAST)
                break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'h':
        default:
Usage:
            usage(1);
        }
    }

    if (outprefix) {
        char *uri = NULL;
        if (optind >= argc)
            goto Usage;
        if ((optind + 1) < argc)
            uri = argv[optind+1];
        flic_produce(packettype, outprefix, keys, argv[optind], uri);
    } else {
        if (optind > argc)
            goto Usage;
        flic_fetch();
    }

    return 0;
}

// eof
