/*
 * @f ccnl-pkt-builder.c
 * @b CCN lite - packet builder
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
 *
 */


#include "ccnl-pkt-builder.h"

#ifdef CCNL_RIOT
#include "random.h"
#endif

#ifdef USE_SUITE_CCNB

int8_t ccnb_isContent(unsigned char *buf, size_t len)
{
    uint64_t num;
    uint8_t typ;

    if (ccnl_ccnb_dehead(&buf, &len, &num, &typ)) {
        return 0;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        return 0;
    }
    return 1;
}
#endif // USE_SUITE_CCNB

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(uint8_t *buf, size_t len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = (struct ccnx_tlvhdr_ccnx2015_s*)buf;

    if (len < sizeof(struct ccnx_tlvhdr_ccnx2015_s)) {
        DEBUGMSG(ERROR, "ccntlv header not large enough\n");
        return NULL;
    }
    if (hp->version != CCNX_TLV_V1) {
        DEBUGMSG(ERROR, "ccntlv version %d not supported\n", hp->version);
        return NULL;
    }
    if (ntohs(hp->pktlen) < len) {
        DEBUGMSG(ERROR, "ccntlv packet too small (%d instead of %zu bytes)\n",
                 ntohs(hp->pktlen), len);
        return NULL;
    }
    return hp;
}

int8_t ccntlv_isData(uint8_t *buf, size_t len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Data;
}

int8_t ccntlv_isFragment(uint8_t *buf, size_t len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Fragment;
}

#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef  USE_SUITE_NDNTLV
int8_t ndntlv_isData(uint8_t *buf, size_t len) {
    uint64_t typ;
    size_t vallen;

    if (ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen)) {
        return -1;
    }
    if (typ != NDN_TLV_Data) {
        return 0;
    }
    return 1;
}
#endif //USE_SUITE_NDNTLV

// ----------------------------------------------------------------------

int8_t
ccnl_isContent(uint8_t *buf, size_t len, int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        return ccnb_isContent(buf, len);
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return ccntlv_isData(buf, len);
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        return ndntlv_isData(buf, len);
#endif
    }

    DEBUGMSG(WARNING, "unknown suite %d in %s:%d\n",
                      suite, __func__, __LINE__);
    return -1;
}

int8_t
ccnl_isFragment(uint8_t *buf, size_t len, int suite)
{
    (void) buf;
    (void) len;

    switch(suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return ccntlv_isFragment(buf, len);
#endif
    }

    DEBUGMSG(DEBUG, "unknown suite %d in %s of %s:%d\n",
                    suite, __func__, __FILE__, __LINE__);
    return -1;
}

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_interest_s *
ccnl_mkInterestObject(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts)
{
    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
                                                                       sizeof(struct ccnl_interest_s));
    if (!i) {
        return NULL;
    }
    i->pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    if (!i->pkt) {
        ccnl_free(i);
        return NULL;
    }
    i->pkt->buf = ccnl_mkSimpleInterest(name, opts);
    if (!i->pkt->buf) {
        ccnl_pkt_free(i->pkt);
        ccnl_free(i);
        return NULL;
    }
    i->pkt->pfx = ccnl_prefix_dup(name);
    i->from = NULL;
    return i;
}

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts)
{
    struct ccnl_buf_s *buf = NULL;
    uint8_t *tmp;
    size_t len = 0, offs;
    struct ccnl_prefix_s *prefix;
    (void)prefix;

    tmp = (uint8_t*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    if (!tmp) {
        return NULL;
    }
    offs = CCNL_MAX_PACKET_SIZE;

    if (ccnl_mkInterest(name, opts, tmp, tmp + CCNL_MAX_PACKET_SIZE, &len, &offs)) {
        ccnl_free(tmp);
        return NULL;
    }

    if (len > 0) {
        buf = ccnl_buf_new(tmp + offs, len);
    }
    ccnl_free(tmp);

    return buf;
}

int8_t
ccnl_mkInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts,
                uint8_t *tmp, uint8_t *tmpend, size_t *len, size_t *offs) {
    ccnl_interest_opts_u default_opts = {{ 0 }};

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            ccnl_ccnb_fillInterest(name, NULL, tmp, tmpend, CCNL_MAX_PACKET_SIZE, len);
            (*offs) = 0;
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            if (ccnl_ccntlv_prependInterestWithHdr(name, offs, tmp, len)) {
                DEBUGMSG(ERROR, "Failed to create interest");
                return -1;
            };
            break;
        }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            (void) tmpend;

            if (!opts) {
                opts = &default_opts;
            }

            if (!opts->ndntlv.nonce) {
#ifndef CCNL_RIOT
                opts->ndntlv.nonce = rand();
#else
                opts->ndntlv.nonce = random_uint32();
#endif
            }

            if (ccnl_ndntlv_prependInterest(name, -1, &(opts->ndntlv), offs, tmp, len)) {
                DEBUGMSG(ERROR, "Failed to create interest");
                return -1;
            }
            DEBUGMSG(TRACE, "Packet length: %zd\n", *len);
            break;
#endif
        default:
            break;
    }
    return 0;
}

struct ccnl_content_s *
ccnl_mkContentObject(struct ccnl_prefix_s *name,
                     uint8_t *payload, size_t paylen,
                     ccnl_data_opts_u *opts)
{
    size_t dataoffset = 0;
    struct ccnl_pkt_s *c_p = ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    if (!c_p) {
        return NULL;
    }
    c_p->buf = ccnl_mkSimpleContent(name, payload, paylen, &dataoffset, opts);
    if (!c_p->buf) {
        ccnl_pkt_free(c_p);
        return NULL;
    }
    c_p->pfx = ccnl_prefix_dup(name);
    if (!c_p->pfx) {
        ccnl_pkt_free(c_p);
        return NULL;
    }
    c_p->content = c_p->buf->data + dataoffset;
    c_p->contlen = paylen;
    return ccnl_content_new(&c_p);

}

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     uint8_t *payload, size_t paylen, size_t *payoffset,
                     ccnl_data_opts_u *opts)
{
    struct ccnl_buf_s *buf = NULL;
    uint8_t *tmp;
    size_t len = 0, contentpos = 0, offs;
    struct ccnl_prefix_s *prefix;
    (void)prefix;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %zu bytes)\n",
                  ccnl_prefix_to_str(name, s, CCNL_MAX_PREFIX_SIZE),
                  paylen);

    tmp = (uint8_t*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    if (!tmp) {
        return NULL;
    }
    offs = CCNL_MAX_PACKET_SIZE;

    if (ccnl_mkContent(name, payload, paylen, tmp, &len, &contentpos, &offs, opts)) {
        ccnl_free(tmp);
        return NULL;
    }

    if (len) {
        buf = ccnl_buf_new(tmp + offs, len);
        if (payoffset) {
            *payoffset = contentpos;
        }
    }
    ccnl_free(tmp);

    return buf;
}

int8_t
ccnl_mkContent(struct ccnl_prefix_s *name, uint8_t *payload, size_t paylen, uint8_t *tmp,
               size_t *len, size_t *contentpos, size_t *offs, ccnl_data_opts_u *opts) {
    switch (name->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            ccnl_ccnb_fillContent(name, payload, paylen, contentpos, tmp, tmp + *len, len);
            *offs = 0;
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            uint32_t lastchunknum = 0;
            if (ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, &lastchunknum,
                                                  contentpos, offs, tmp, len)) {
                return -1;
            }
            break;
        }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            if (ccnl_ndntlv_prependContent(name, payload, paylen, contentpos,
                                           &(opts->ndntlv), offs, tmp, len)) {
                return -1;
            }
            break;
#endif
        default:
            break;
    }
    return 0;
}

#endif // NEEDS_PACKET_CRAFTING
// eof
