/*
 * @f ccnl-pkt-bilder.h
 * @b CCN lite - CCN packet builder
 *
 * Copyright (C) 2011-17, University of Basel
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
 */

#ifndef CCNL_PKT_BUILDER
#define CCNL_PKT_BUILDER

#include "ccnl-core.h"

#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-cistlv.h"
#include "ccnl-pkt-iottlv.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"
#include "ccnl-pkt-localrpc.h"

#ifdef USE_SUITE_CCNB
int ccnb_isContent(unsigned char *buf, int len);
#endif // USE_SUITE_CCNB

#ifdef USE_SUITE_CCNTLV

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(unsigned char *buf, int len);

int ccntlv_isData(unsigned char *buf, int len);

int ccntlv_isFragment(unsigned char *buf, int len);
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_CISTLV

int cistlv_isData(unsigned char *buf, int len);
#endif // USE_SUITE_CISTLV

#ifdef USE_SUITE_IOTTLV
int iottlv_isReply(unsigned char *buf, int len);

int iottlv_isFragment(unsigned char *buf, int len);
#endif // USE_SUITE_IOTTLV

#ifdef  USE_SUITE_NDNTLV
int ndntlv_isData(unsigned char *buf, int len);
#endif //USE_SUITE_NDNTLV

int
ccnl_isContent(unsigned char *buf, int len, int suite);

int
ccnl_isFragment(unsigned char *buf, int len, int suite);

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_content_s *
ccnl_mkContentObject(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen);

struct ccnl_buf_s*
ccnl_mkSimpleContent_scratch(unsigned char *scratch, int scratch_len,
                             struct ccnl_prefix_s *name,
                             unsigned char *payload, int paylen, int *payoffset);

static inline struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset)
{
    struct ccnl_buf_s *buf = NULL;

    unsigned char *tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    buf = ccnl_mkSimpleContent_scratch(tmp, CCNL_MAX_PACKET_SIZE, name,
                                       payload, paylen, payoffset);
    ccnl_free(tmp);

    return buf;
}

void
ccnl_mkContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, unsigned char *tmp,
               int *len, int *contentpos, int *offs);


struct ccnl_interest_s *
ccnl_mkInterestObject(struct ccnl_prefix_s *name, int *nonce);

struct ccnl_buf_s*
ccnl_mkSimpleInterest_scratch(unsigned char *scratch, int scratch_len,
                              struct ccnl_prefix_s *name, int *nonce);

static inline struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce)
{
    struct ccnl_buf_s *buf = NULL;

    unsigned char *tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    buf = ccnl_mkSimpleInterest_scratch(tmp, CCNL_MAX_PACKET_SIZE, name, nonce);
    ccnl_free(tmp);

    return buf;
}

void
ccnl_mkInterest(struct ccnl_prefix_s *name, int *nonce, unsigned char *tmp,
                int *len, int *offs);

#endif

#endif //CCNL_PKT_BUILDER
