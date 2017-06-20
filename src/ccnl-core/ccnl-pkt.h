/*
 * @f ccnl-pkt.h
 * @b CCN lite, core CCNx protocol logic
 *
 * Copyright (C) 2011-18 University of Basel
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
 * 2017-06-16 created
 */

#ifndef CCNL_PKT_H
#define CCNL_PKT_H

#include <stddef.h>

#include "ccnl-buf.h"
#include "ccnl-prefix.h"

// packet flags:  0000ebtt
#define CCNL_PKT_REQUEST    0x01 // "Interest"
#define CCNL_PKT_REPLY      0x02 // "Object", "Data"
#define CCNL_PKT_FRAGMENT   0x03 // "Fragment"
#define CCNL_PKT_FRAG_BEGIN 0x04 // see also CCNL_DATA_FRAG_FLAG_FIRST etc
#define CCNL_PKT_FRAG_END   0x08

struct ccnl_pktdetail_ccnb_s {
    int minsuffix, maxsuffix, aok, scope;
    struct ccnl_buf_s *nonce;
    struct ccnl_buf_s *ppkd;        // publisher public key digest
};

struct ccnl_pktdetail_ccntlv_s {
    struct ccnl_buf_s *keyid;       // publisher keyID
};

struct ccnl_pktdetail_iottlv_s {
    int ttl;
};

struct ccnl_pktdetail_ndntlv_s {
    int minsuffix, maxsuffix, mbf, scope;
    struct ccnl_buf_s *nonce;      // nonce
    struct ccnl_buf_s *ppkl;       // publisher public key locator
};

struct ccnl_pkt_s {
    struct ccnl_buf_s *buf;        // the packet's bytes
    struct ccnl_prefix_s *pfx;     // prefix/name
    unsigned char *content;        // pointer into the data buffer
    int contlen;
    unsigned int type;   // suite-specific value (outermost type)
    union {
        int final_block_id;
        unsigned int seqno;
    } val;
    union {
        struct ccnl_pktdetail_ccnb_s   ccnb;
        struct ccnl_pktdetail_ccntlv_s ccntlv;
        struct ccnl_pktdetail_iottlv_s iottlv;
        struct ccnl_pktdetail_ndntlv_s ndntlv;
    } s;                           // suite specific packet details
#ifdef USE_HMAC256
    unsigned char *hmacStart;
    int hmacLen;
    unsigned char *hmacSignature;
#endif
    unsigned int flags;
    char suite;
};

void
ccnl_pkt_free(struct ccnl_pkt_s *pkt);

int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen);

int
ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#endif // EOF
