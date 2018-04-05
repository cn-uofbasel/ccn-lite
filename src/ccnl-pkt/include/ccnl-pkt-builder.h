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

#include "ccnl-pkt.h"
#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-cistlv.h"
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
                     unsigned char *payload, int paylen,
                     ccnl_data_opts_u *opts);

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset,
                     ccnl_data_opts_u *opts);

void
ccnl_mkContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, unsigned char *tmp,
               int *len, int *contentpos, int *offs, ccnl_data_opts_u *opts);


struct ccnl_interest_s *
ccnl_mkInterestObject(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts);

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts);

void
ccnl_mkInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts, unsigned char *tmp,
                int *len, int *offs);

#endif

#endif //CCNL_PKT_BUILDER
