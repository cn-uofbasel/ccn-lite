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

#ifdef USE_SUITE_CCNB

int ccnb_isContent(unsigned char *buf, int len)
{
    int num, typ;

    if (len < 0 || ccnl_ccnb_dehead(&buf, &len, &num, &typ))
        return -1;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        return 0;
    return 1;
}
#endif // USE_SUITE_CCNB

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = (struct ccnx_tlvhdr_ccnx2015_s*)buf;

    if ((unsigned int)len < sizeof(struct ccnx_tlvhdr_ccnx2015_s)) {
        DEBUGMSG(ERROR, "ccntlv header not large enough\n");
        return NULL;
    }
    if (hp->version != CCNX_TLV_V1) {
        DEBUGMSG(ERROR, "ccntlv version %d not supported\n", hp->version);
        return NULL;
    }
    if (ntohs(hp->pktlen) < len) {
        DEBUGMSG(ERROR, "ccntlv packet too small (%d instead of %d bytes)\n",
                 ntohs(hp->pktlen), len);
        return NULL;
    }
    return hp;
}

int ccntlv_isData(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Data;
}

int ccntlv_isFragment(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Fragment;
}

#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef  USE_SUITE_NDNTLV
int ndntlv_isData(unsigned char *buf, int len) {
    int typ;
    int vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, (int *) &typ, &vallen))
        return -1;
    if (typ != NDN_TLV_Data)
        return 0;
    return 1;
}
#endif //USE_SUITE_NDNTLV

// ----------------------------------------------------------------------

int
ccnl_isContent(unsigned char *buf, int len, int suite)
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

int
ccnl_isFragment(unsigned char *buf, int len, int suite)
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
    i->pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    i->pkt->buf = ccnl_mkSimpleInterest(name, opts);
    i->pkt->pfx = ccnl_prefix_dup(name);
    i->flags |= CCNL_PIT_COREPROPAGATES;
    i->from = NULL;
    return i;
}

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs;
    struct ccnl_prefix_s *prefix;
    (void)prefix;

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    ccnl_mkInterest(name, opts, tmp, &len, &offs);

    if (len > 0)
        buf = ccnl_buf_new(tmp + offs, len);
    ccnl_free(tmp);

    return buf;
}

void ccnl_mkInterest(struct ccnl_prefix_s *name, ccnl_interest_opts_u *opts,
                     unsigned char *tmp, int *len, int *offs) {
    ccnl_interest_opts_u default_opts;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            (*len) = ccnl_ccnb_fillInterest(name, NULL, tmp, CCNL_MAX_PACKET_SIZE);
            (*offs) = 0;
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            (*len) = ccnl_ccntlv_prependInterestWithHdr(name, offs, tmp);
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            if (!opts) {
                opts = &default_opts;
            }

            if (!opts->ndntlv.nonce) {
                opts->ndntlv.nonce = rand();
            }

            (*len) = ccnl_ndntlv_prependInterest(name, -1, &(opts->ndntlv), offs, tmp);
            break;
#endif
        default:
            break;
    }
}

struct ccnl_content_s *
ccnl_mkContentObject(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen,
                     ccnl_data_opts_u *opts)
{
    int dataoffset;
    struct ccnl_pkt_s *c_p = ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    c_p->buf = ccnl_mkSimpleContent(name, payload, paylen, &dataoffset, opts);
    c_p->pfx = ccnl_prefix_dup(name);
    c_p->content = c_p->buf->data + dataoffset;
    c_p->contlen = paylen;
    return ccnl_content_new(&c_p);

}

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset,
                     ccnl_data_opts_u *opts)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, contentpos = 0, offs;
    struct ccnl_prefix_s *prefix;
    (void)prefix;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
                  ccnl_prefix_to_str(name, s, CCNL_MAX_PREFIX_SIZE),
                  paylen);

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    ccnl_mkContent(name, payload, paylen, tmp, &len, &contentpos, &offs, opts);

    if (len) {
        buf = ccnl_buf_new(tmp + offs, len);
        if (payoffset)
            *payoffset = contentpos;
    }
    ccnl_free(tmp);

    return buf;
}

void
ccnl_mkContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, unsigned char *tmp,
               int *len, int *contentpos, int *offs, ccnl_data_opts_u *opts) {
    switch (name->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            (*len) = ccnl_ccnb_fillContent(name, payload, paylen, contentpos, tmp);
            (*offs) = 0;
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            unsigned int lcn = 0; // lastchunknum
            (*len) = ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, &lcn,
                                                       contentpos, offs, tmp);
            break;
        }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            (*len) = ccnl_ndntlv_prependContent(name, payload, paylen,
                                                contentpos, &(opts->ndntlv), offs, tmp);
            break;
#endif
        default:
        break;
    }
}

#endif // NEEDS_PACKET_CRAFTING
// eof
