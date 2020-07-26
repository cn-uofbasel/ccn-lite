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

#ifndef CCNL_LINUXKERNEL
#include "ccnl-core.h"

#include "ccnl-pkt.h"
#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"
#include "ccnl-pkt-localrpc.h"
#else
#include "../../ccnl-core/include/ccnl-core.h"

#include "../../ccnl-core/include/ccnl-pkt.h"
#include "../include/ccnl-pkt-ccnb.h"
#include "../include/ccnl-pkt-ccntlv.h"
#include "../include/ccnl-pkt-ndntlv.h"
#include "../include/ccnl-pkt-switch.h"
#include "../include/ccnl-pkt-localrpc.h"
#endif


#ifdef USE_SUITE_CCNB
int8_t ccnb_isContent(uint8_t *buf, size_t len);
#endif // USE_SUITE_CCNB

#ifdef USE_SUITE_CCNTLV

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(uint8_t *buf, size_t len);

int8_t ccntlv_isData(uint8_t *buf, size_t len);

int8_t ccntlv_isFragment(uint8_t *buf, size_t len);
#endif // USE_SUITE_CCNTLV

#ifdef  USE_SUITE_NDNTLV
int8_t ndntlv_isData(uint8_t *buf, size_t len);
#endif //USE_SUITE_NDNTLV

int8_t
ccnl_isContent(uint8_t *buf, size_t len, int suite);

int8_t
ccnl_isFragment(uint8_t *buf, size_t len, int suite);

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_content_s *
ccnl_mkContentObject(struct ccnl_prefix_s *name,
                     uint8_t *payload, size_t paylen,
                     ccnl_data_opts_u *opts);

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     uint8_t *payload, size_t paylen, size_t *payoffset,
                     ccnl_data_opts_u *opts);

int8_t
ccnl_mkContent(struct ccnl_prefix_s *name, uint8_t *payload, size_t paylen, uint8_t *tmp,
               size_t *len, size_t *contentpos, size_t *offs, ccnl_data_opts_u *opts);


struct ccnl_interest_s *
ccnl_mkInterestObject(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts);

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts);

int8_t
ccnl_mkInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts,
                uint8_t *tmp, uint8_t *tmpend, size_t *len, size_t *offs);

#endif

#endif //CCNL_PKT_BUILDER
