/*
 * @f ccnl-pkt.c
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

#include "ccnl-pkt.h"

#include "ccnl-os-time.h"
#include "ccnl-defs.h"
#include "ccnl-pkt-cistlv.h"
#include "ccnl-pkt-ccntlv.h"

#include "ccnl-prefix.h"
#include "ccnl-malloc.h"

#include "ccnl-logging.h"

void
ccnl_pkt_free(struct ccnl_pkt_s *pkt)
{
    if (pkt) {
        if (pkt->pfx) {
            switch (pkt->pfx->suite) {
#ifdef USE_SUITE_CCNB
            case CCNL_SUITE_CCNB:
                ccnl_free(pkt->s.ccnb.nonce);
                ccnl_free(pkt->s.ccnb.ppkd);
                break;
#endif
#ifdef USE_SUITE_CCNTLV
            case CCNL_SUITE_CCNTLV:
                ccnl_free(pkt->s.ccntlv.keyid);
                break;
#endif
#ifdef USE_SUITE_NDNTLV
            case CCNL_SUITE_NDNTLV:
                ccnl_free(pkt->s.ndntlv.nonce);
                ccnl_free(pkt->s.ndntlv.ppkl);
                break;
#endif
#ifdef USE_SUITE_CISTLV
            case CCNL_SUITE_CISTLV:
#endif
#ifdef USE_SUITE_LOCALRPC
            case CCNL_SUITE_LOCALRPC:
#endif
            default:
                break;
            }
            ccnl_prefix_free(pkt->pfx);
        }
        if(pkt->buf){
            ccnl_free(pkt->buf);
        }
        ccnl_free(pkt);
    }
}


struct ccnl_pkt_s *
ccnl_pkt_dup(struct ccnl_pkt_s *pkt){
    struct ccnl_pkt_s * ret = ccnl_malloc(sizeof(struct ccnl_pkt_s));
    if(!pkt){
        return NULL;
    }
    if(!ret){
        return NULL;
    }
    if (pkt->pfx) {
        ret->s = pkt->s;
        switch (pkt->pfx->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            ret->s.ccnb.nonce = buf_dup(pkt->s.ccnb.nonce);
            ret->s.ccnb.ppkd = buf_dup(pkt->s.ccnb.ppkd);
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            ret->s.ccntlv.keyid = buf_dup(pkt->s.ccntlv.keyid);
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            ret->s.ndntlv.nonce = buf_dup(pkt->s.ndntlv.nonce);
            ret->s.ndntlv.ppkl = buf_dup(pkt->s.ndntlv.ppkl);
            break;
#endif
#ifdef USE_SUITE_CISTLV
        case CCNL_SUITE_CISTLV:
#endif
#ifdef USE_SUITE_LOCALRPC
        case CCNL_SUITE_LOCALRPC:
#endif
        default:
            break;
        }
        ret->pfx = ccnl_prefix_dup(pkt->pfx);
        if(!ret->pfx){
            ret->buf = NULL;
            ccnl_pkt_free(ret);
            return NULL;
        }
        ret->pfx->suite = pkt->pfx->suite;
        ret->suite = pkt->suite;
        ret->buf = buf_dup(pkt->buf);
        ret->content = ret->buf->data + (pkt->content - pkt->buf->data);
        ret->contlen = pkt->contlen;
    }
    return ret;
}

int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen)
{
    int len = 0;

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CCNX_TLV_N_NameSegment);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CISCO_TLV_NameComponent);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
    default:
        len = srclen;
        memcpy(dst, src, len);
        break;
    }

    return len;
}

int
ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf)
{
    int len = strlen(src);

    DEBUGMSG_CUTL(TRACE, "ccnl_pkt_prependComponent(%d, %s)\n", suite, src);

    if (*offset < len)
        return -1;
    memcpy(buf + *offset - len, src, len);
    *offset -= len;

#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CCNX_TLV_N_NameSegment);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CISCO_TLV_NameComponent);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif

    return len;
}

