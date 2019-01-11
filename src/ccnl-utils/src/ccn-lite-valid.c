/*
 * @f util/ccn-lite-valid.c
 * @b demo app (UNIX filter) for validating a packet's signature
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
 * 2015-06-08  created
 */

#define assert(...) do {} while(0)
#include "ccnl-common.h"
#include "ccnl-ext-hmac.h"

struct ccnl_pkt_s*
ccnl_parse(uint8_t *data, size_t datalen)
{
    unsigned char *base = data;
    int suite = -1;
    int32_t enc;
    size_t skip = 0;
    struct ccnl_pkt_s *pkt = 0;

    DEBUGMSG(DEBUG, "start parsing %zu bytes\n", datalen);

    // work through explicit code switching
    while (!ccnl_switch_dehead(&data, &datalen, &enc))
        suite = ccnl_enc2suite(enc);
    if (suite == -1)
        suite = ccnl_pkt2suite(data, datalen, &skip);

    if (!ccnl_isSuite(suite)) {
        DEBUGMSG(WARNING, "?unknown packet format? %zu bytes starting with 0x%02x at offset %zd\n",
                     datalen, *data, data - base);
        return NULL;
    }

    DEBUGMSG(DEBUG, "  suite=%s\n", ccnl_suite2str(suite));

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        size_t hdrlen;

        if (ccnl_ccntlv_getHdrLen(data, datalen, &hdrlen)) {
            return NULL;
        }
        data += hdrlen;
        datalen -= hdrlen;

        pkt = ccnl_ccntlv_bytes2pkt(base, &data, &datalen);
        if (!pkt) {
            DEBUGMSG(FATAL, "ccnx2015: parse error\n");
            return NULL;
        }
        if (pkt->type != CCNX_TLV_TL_Interest && pkt->type != CCNX_TLV_TL_Object) {
          DEBUGMSG(INFO, "ccnx2015: neither Interest nor Data (%lu)\n",
                   pkt->type);
            return pkt;
        }
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV: {
        uint64_t typ;
        size_t len2;

        if (ccnl_ndntlv_dehead(&data, &datalen, &typ, &len2)) {
            DEBUGMSG(FATAL, "ndn2013: parse error\n");
            return NULL;
        }
        pkt = ccnl_ndntlv_bytes2pkt(typ, base, &data, &datalen);
        if (!pkt) {
            DEBUGMSG(FATAL, "ndn2013: parse error\n");
            return NULL;
        }
        if (pkt->type != CCNX_PT_Interest && pkt->type != CCNX_PT_Data) {
            DEBUGMSG(INFO, "ccnx2015: neither Interest nor Data\n");
            return pkt;
        }
        break;
    }
#endif
    default:
        DEBUGMSG(INFO, "packet without HMAC\n");
        pkt = ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
        pkt->buf = ccnl_buf_new(NULL, datalen);
        return pkt;
    }
    return pkt;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char incoming[64*1024];
    size_t len = 0, signLen = 32;
    ssize_t rc;
    int opt, cnt, exitBehavior = 0;
    struct ccnl_pkt_s *pkt;
    unsigned char keyval[64], signature[32];
    char *keyfile = NULL;
    struct key_s *keys = NULL;

    base64_build_decoding_table();

    while ((opt = getopt(argc, argv, "e:hk:v:")) != -1) {
        switch (opt) {
        case 'e':
            exitBehavior = (int)strtol(optarg, (char**)NULL, 10);
            break;
        case 'k':
            keyfile = optarg;
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0])) {
                debug_level = (int) strtol(optarg, (char **) NULL, 10);
            } else {
                debug_level = ccnl_debug_str2level(optarg);
            }
#endif
            break;
        case 'h':
        default:
Usage:
            fprintf(stderr, "usage: %s [options] <INCOMING >OUTGOING\n"
            "  -k FILE  HMAC256 keys (base64 encoded)\n"
            "  -e N     exit: 0=drop-missing-or-inval, 1=warn-missing-or-inval, 2=warn-inval\n"
            "  -h       this help text\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
,
            argv[0]);
            exit(1);
        }
    }
    if(!keyfile){
        DEBUGMSG(FATAL, "Keyfile must be specified");
    }
    keys = load_keys_from_file(keyfile);

    if (!keys) {
        fprintf(stderr, "no key, aborting\n");
        goto Usage;
    }

    while ((rc = read(0, incoming + len, sizeof(incoming) - len)) > 0) {
        if (rc < 0) {
            fprintf(stderr, "read failure, aborting. errno: %d\n", errno);
        }
        len += rc;
    }
    if (len <= 0) {
        return 0;
    }

    // parse packet pkt = ccnl_
    pkt = ccnl_parse(incoming, len);
    if (!pkt) {
        DEBUGMSG(FATAL, "error parsing packet\n");
        return -1;
    }
    if (!pkt->hmacLen) {
        if (exitBehavior == 0) {
            DEBUGMSG(FATAL, "no signature data found, packet dropped\n");
            return -1;
        }
        if (exitBehavior == 1) {
            DEBUGMSG(INFO, "no signature data found\n");
        }
    }

    // validate packet
    if (pkt->hmacLen) {
        cnt = 1;
        while (keys) {
            DEBUGMSG(VERBOSE, "trying key #%d\n", cnt);
            if (keys->keylen < 0) {
                DEBUGMSG(ERROR, "invalid key length, packet dropped: %d\n", keys->keylen);
                return -1;
            }
            ccnl_hmac256_keyval(keys->key, (size_t) keys->keylen, keyval);
            ccnl_hmac256_sign(keyval, 64, pkt->hmacStart, pkt->hmacLen, signature, &signLen);
            if (!memcmp(signature, pkt->hmacSignature, 32)) {
                DEBUGMSG(INFO, "signature is valid (key #%d)\n", cnt);
                break;
            }
            keys = keys->next;
            cnt++;
        }
        if (!keys) {
            if (exitBehavior == 0) {
                DEBUGMSG(FATAL, "invalid signature, packet dropped\n");
                return -1;
            }
            DEBUGMSG(FATAL, "invalid signature\n");
        }
    }

    // output packet
    write(1, pkt->buf->data, pkt->buf->datalen);

    return 0;
}

// eof
