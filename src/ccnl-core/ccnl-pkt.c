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
#include "ccnl-prefix.h"
#include "ccnl-malloc.h"

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
#ifdef USE_SUITE_IOTTLV
            case CCNL_SUITE_IOTTLV:
#endif
#ifdef USE_SUITE_LOCALRPC
            case CCNL_SUITE_LOCALRPC:
#endif
            default:
                break;
            }
            ccnl_prefix_free(pkt->pfx);
        }
        ccnl_free(pkt->buf);
        ccnl_free(pkt);
    }
}
