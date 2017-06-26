/*
 * @f ccnl-interest.c
 * @b CCN lite (CCNL), core source file (internal data structures)
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
 * File history:
 * 2017-06-16 created
 */

#include "ccnl-interest.h"

#include "ccnl-malloc.h"
#include "ccnl-os-time.h"
#include "ccnl-prefix.h"
#include "ccnl-logging.h"

//FIXME: RELEAY FUNCTION MUST BE RENAMED!

/*struct ccnl_interest_s*
ccnl_interest_new(struct ccnl_face_s *from, struct ccnl_pkt_s **pkt)
{
    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
                                            sizeof(struct ccnl_interest_s));
    char *s = NULL;
    DEBUGMSG_CORE(TRACE,
                  "ccnl_new_interest(prefix=%s, suite=%s)\n",
                  (s = ccnl_prefix_to_path((*pkt)->pfx)),
                  ccnl_suite2str((*pkt)->pfx->suite));
    ccnl_free(s);
    if (!i)
        return NULL;
    i->pkt = *pkt;
    *pkt = NULL;
    i->flags |= CCNL_PIT_COREPROPAGATES;
    i->from = from;
    i->last_used = CCNL_NOW();

    return i;
}
*/

int
ccnl_interest_isSame(struct ccnl_interest_s *i, struct ccnl_pkt_s *pkt)
{
    if (i->pkt->pfx->suite != pkt->suite ||
                ccnl_prefix_cmp(i->pkt->pfx, NULL, pkt->pfx, CMP_EXACT))
        return 0;

    switch (i->pkt->pfx->suite) {
        #ifdef USE_SUITE_CCNB
            case CCNL_SUITE_CCNB:
                return i->pkt->s.ccnb.minsuffix == pkt->s.ccnb.minsuffix &&
                    i->pkt->s.ccnb.maxsuffix == pkt->s.ccnb.maxsuffix &&
                    ((!i->pkt->s.ccnb.ppkd && !pkt->s.ccnb.ppkd) ||
                            buf_equal(i->pkt->s.ccnb.ppkd, pkt->s.ccnb.ppkd));
        #endif
        #ifdef USE_SUITE_NDNTLV
            case CCNL_SUITE_NDNTLV:
                return i->pkt->s.ndntlv.minsuffix == pkt->s.ndntlv.minsuffix &&
                    i->pkt->s.ndntlv.maxsuffix == pkt->s.ndntlv.maxsuffix &&
                    ((!i->pkt->s.ndntlv.ppkl && !pkt->s.ndntlv.ppkl) ||
                            buf_equal(i->pkt->s.ndntlv.ppkl, pkt->s.ndntlv.ppkl));
        #endif
        #ifdef USE_SUITE_CCNTLV
            case CCNL_SUITE_CCNTLV:
        #endif
        #ifdef USE_SUITE_CISTLV
            case CCNL_SUITE_CISTLV:
        #endif
        #ifdef USE_SUITE_IOTTLV
            case CCNL_SUITE_IOTTLV:
        #endif
            default:
                break;
    }
    return 1;
}


int
ccnl_interest_append_pending(struct ccnl_interest_s *i,
                             struct ccnl_face_s *from)
{
    struct ccnl_pendint_s *pi, *last = NULL;
    DEBUGMSG_CORE(TRACE, "ccnl_append_pending\n");

    for (pi = i->pending; pi; pi = pi->next) { // check whether already listed
        if (pi->face == from) {
            DEBUGMSG_CORE(DEBUG, "  we found a matching interest, updating time\n");
            pi->last_used = CCNL_NOW();
            #ifdef USE_TIMEOUT_KEEPCONTENT
                // ensure the pending interest sticks around while the client keeps repeating the same request
                i->last_used = CCNL_NOW();
                i->retries = 0;
            #endif
            return 0;
        }
        last = pi;
    }
    pi = (struct ccnl_pendint_s *) ccnl_calloc(1,sizeof(struct ccnl_pendint_s));
    if (!pi) {
        DEBUGMSG_CORE(DEBUG, "  no mem\n");
        return -1;
    }
    DEBUGMSG_CORE(DEBUG, "  appending a new pendint entry %p\n", (void *) pi);
    pi->face = from;
    pi->last_used = CCNL_NOW();
    if (last)
        last->next = pi;
    else
        i->pending = pi;
    return 0;
}