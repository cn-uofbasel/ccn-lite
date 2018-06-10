/*
 * @f ccnl-relay.c
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

#ifndef CCNL_LINUXKERNEL
#ifdef USE_NFN
#include "ccnl-nfn-common.h"
#endif
#include "ccnl-core.h"
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#else //CCNL_LINUXKERNEL
#include <ccnl-core.h>
#endif //CCNL_LINUXKERNEL

#ifdef CCNL_RIOT
#include "ccn-lite-riot.h"
#endif



struct ccnl_face_s*
ccnl_get_face_or_create(struct ccnl_relay_s *ccnl, int ifndx,
                       struct sockaddr *sa, int addrlen)
// sa==NULL means: local(=in memory) client, search for existing ifndx being -1
// sa!=NULL && ifndx==-1: search suitable interface for given sa_family
// sa!=NULL && ifndx!=-1: use this (incoming) interface for outgoing
{
    static int seqno;
    int i;
    struct ccnl_face_s *f;

    DEBUGMSG_CORE(TRACE, "ccnl_get_face_or_create src=%s\n",
             ccnl_addr2ascii((sockunion*)sa));

    for (f = ccnl->faces; f; f = f->next) {
        if (!sa) {
            if (f->ifndx == -1)
                return f;
            continue;
        }
        if (ifndx != -1 && (f->ifndx == ifndx) &&
            !ccnl_addr_cmp(&f->peer, (sockunion*)sa)) {
            f->last_used = CCNL_NOW();
#ifdef CCNL_RIOT
            ccnl_evtimer_reset_face_timeout(f);
#endif
            return f;
        }
    }

    if (sa && ifndx == -1) {
        for (i = 0; i < ccnl->ifcount; i++) {
            if (sa->sa_family != ccnl->ifs[i].addr.sa.sa_family)
                continue;
            ifndx = i;
            break;
        }
        if (ifndx == -1) // no suitable interface found
            return NULL;
    }
    DEBUGMSG_CORE(VERBOSE, "  found suitable interface %d for %s\n", ifndx,
                ccnl_addr2ascii((sockunion*)sa));

    f = (struct ccnl_face_s *) ccnl_calloc(1, sizeof(struct ccnl_face_s));
    if (!f) {
        DEBUGMSG_CORE(VERBOSE, "  no memory for face\n");
        return NULL;
    }
    f->faceid = ++seqno;
    f->ifndx = ifndx;

    if (ifndx >= 0) {
        if (ccnl->defaultFaceScheduler)
            f->sched = ccnl->defaultFaceScheduler(ccnl,
                                          (void(*)(void*,void*))ccnl_face_CTS);
        if (ccnl->ifs[ifndx].reflect)   f->flags |= CCNL_FACE_FLAGS_REFLECT;
        if (ccnl->ifs[ifndx].fwdalli)   f->flags |= CCNL_FACE_FLAGS_FWDALLI;
    }

    if (sa)
        memcpy(&f->peer, sa, addrlen);
    else // local client
        f->ifndx = -1;
    f->last_used = CCNL_NOW();
    DBL_LINKED_LIST_ADD(ccnl->faces, f);

    TRACEOUT();

#ifdef CCNL_RIOT
    ccnl_evtimer_reset_face_timeout(f);
#endif

    return f;
}

struct ccnl_face_s*
ccnl_face_remove(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_face_s *f2;
    struct ccnl_interest_s *pit;
    struct ccnl_forward_s **ppfwd;

    DEBUGMSG_CORE(DEBUG, "face_remove relay=%p face=%p\n",
             (void*)ccnl, (void*)f);

    ccnl_sched_destroy(f->sched);
#ifdef USE_FRAG
    ccnl_frag_destroy(f->frag);
#endif
    DEBUGMSG_CORE(TRACE, "face_remove: cleaning PIT\n");
    for (pit = ccnl->pit; pit; ) {
        struct ccnl_pendint_s **ppend, *pend;
        if (pit->from == f)
            pit->from = NULL;
        for (ppend = &pit->pending; *ppend;) {
            if ((*ppend)->face == f) {
                pend = *ppend;
                *ppend = pend->next;
                ccnl_free(pend);
            } else
                ppend = &(*ppend)->next;
        }
        if (pit->pending)
            pit = pit->next;
        else {
            DEBUGMSG_CORE(TRACE, "before NFN interest_remove 0x%p\n",
                          (void*)pit);
#ifdef USE_NFN
            pit = ccnl_nfn_interest_remove(ccnl, pit);
#else
            pit = ccnl_interest_remove(ccnl, pit);
#endif
        }
    }
    DEBUGMSG_CORE(TRACE, "face_remove: cleaning fwd table\n");
    for (ppfwd = &ccnl->fib; *ppfwd;) {
        if ((*ppfwd)->face == f) {
            struct ccnl_forward_s *pfwd = *ppfwd;
            ccnl_prefix_free(pfwd->prefix);
            *ppfwd = pfwd->next;
            ccnl_free(pfwd);
        } else
            ppfwd = &(*ppfwd)->next;
    }
    DEBUGMSG_CORE(TRACE, "face_remove: cleaning pkt queue\n");
    while (f->outq) {
        struct ccnl_buf_s *tmp = f->outq->next;
        ccnl_free(f->outq);
        f->outq = tmp;
    }
    DEBUGMSG_CORE(TRACE, "face_remove: unlinking1 %p %p\n",
             (void*)f->next, (void*)f->prev);
    f2 = f->next;
    DEBUGMSG_CORE(TRACE, "face_remove: unlinking2\n");
    DBL_LINKED_LIST_REMOVE(ccnl->faces, f);
    DEBUGMSG_CORE(TRACE, "face_remove: unlinking3\n");
    ccnl_free(f);

    TRACEOUT();
    return f2;
}

void
ccnl_interface_enqueue(void (tx_done)(void*, int, int), struct ccnl_face_s *f,
                       struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
                       struct ccnl_buf_s *buf, sockunion *dest)
{
    struct ccnl_txrequest_s *r;

    DEBUGMSG_CORE(TRACE, "enqueue interface=%p buf=%p len=%zd (qlen=%d)\n",
                  (void*)ifc, (void*)buf,
                  buf ? buf->datalen : -1, ifc ? ifc->qlen : -1);

    if (ifc->qlen >= CCNL_MAX_IF_QLEN) {
        DEBUGMSG_CORE(WARNING, "  DROPPING buf=%p\n", (void*)buf);
        ccnl_free(buf);
        return;
    }
    r = ifc->queue + ((ifc->qfront + ifc->qlen) % CCNL_MAX_IF_QLEN);
    r->buf = buf;
    memcpy(&r->dst, dest, sizeof(sockunion));
    r->txdone = tx_done;
    r->txdone_face = f;
    ifc->qlen++;

#ifdef USE_SCHEDULER
    ccnl_sched_RTS(ifc->sched, 1, buf->datalen, ccnl, ifc);
#else
    ccnl_interface_CTS(ccnl, ifc);
#endif
}

struct ccnl_buf_s*
ccnl_face_dequeue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_buf_s *pkt;
    DEBUGMSG_CORE(TRACE, "dequeue face=%p (id=%d.%d)\n",
             (void *) f, ccnl->id, f->faceid);

    if (!f->outq)
        return NULL;
    pkt = f->outq;
    f->outq = pkt->next;
    if (!pkt->next)
        f->outqend = NULL;
    pkt->next = NULL;
    return pkt;
}

void
ccnl_face_CTS_done(void *ptr, int cnt, int len)
{
    DEBUGMSG_CORE(TRACE, "CTS_done face=%p cnt=%d len=%d\n", ptr, cnt, len);

#ifdef USE_SCHEDULER
    struct ccnl_face_s *f = (struct ccnl_face_s*) ptr;
    ccnl_sched_CTS_done(f->sched, cnt, len);
#endif
}

void
ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_buf_s *buf;
    DEBUGMSG_CORE(TRACE, "CTS face=%p sched=%p\n", (void*)f, (void*)f->sched);

    if (!f->frag || f->frag->protocol == CCNL_FRAG_NONE) {
        buf = ccnl_face_dequeue(ccnl, f);
        if (buf)
            ccnl_interface_enqueue(ccnl_face_CTS_done, f,
                                   ccnl, ccnl->ifs + f->ifndx, buf, &f->peer);
    }
#ifdef USE_FRAG
    else {
        sockunion dst;
        int ifndx = f->ifndx;
        buf = ccnl_frag_getnext(f->frag, &ifndx, &dst);
        if (!buf) {
            buf = ccnl_face_dequeue(ccnl, f);
            ccnl_frag_reset(f->frag, buf, f->ifndx, &f->peer);
            buf = ccnl_frag_getnext(f->frag, &ifndx, &dst);
        }
        if (buf) {
            ccnl_interface_enqueue(ccnl_face_CTS_done, f,
                                   ccnl, ccnl->ifs + ifndx, buf, &dst);
#ifndef USE_SCHEDULER
            ccnl_face_CTS(ccnl, f); // loop to push more fragments
#endif
        }
    }
#endif
}

int
ccnl_send_pkt(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
                struct ccnl_pkt_s *pkt)
{
    return ccnl_face_enqueue(ccnl, to, buf_dup(pkt->buf));
}

int
ccnl_face_enqueue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
                 struct ccnl_buf_s *buf)
{
    struct ccnl_buf_s *msg;
    if (buf == NULL) {
        DEBUGMSG_CORE(ERROR, "enqueue face: buf most not be NULL\n");
        return -1;
    }
    DEBUGMSG_CORE(TRACE, "enqueue face=%p (id=%d.%d) buf=%p len=%zd\n",
             (void*) to, ccnl->id, to->faceid, (void*) buf, buf ? buf->datalen : -1);

    for (msg = to->outq; msg; msg = msg->next) // already in the queue?
        if (buf_equal(msg, buf)) {
            DEBUGMSG_CORE(VERBOSE, "    not enqueued because already there\n");
            ccnl_free(buf);
            return -1;
        }
    buf->next = NULL;
    if (to->outqend)
        to->outqend->next = buf;
    else
        to->outq = buf;
    to->outqend = buf;
#ifdef USE_SCHEDULER
    if (to->sched) {
#ifdef USE_FRAG
        int len, cnt = ccnl_frag_getfragcount(to->frag, buf->datalen, &len);
#else
        int len = buf->datalen, cnt = 1;
#endif
        ccnl_sched_RTS(to->sched, cnt, len, ccnl, to);
    } else
        ccnl_face_CTS(ccnl, to);
#else
    ccnl_face_CTS(ccnl, to);
#endif

    return 0;
}

struct ccnl_interest_s*
ccnl_interest_new(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
                  struct ccnl_pkt_s **pkt)
{
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
                                            sizeof(struct ccnl_interest_s));
    DEBUGMSG_CORE(TRACE,
                  "ccnl_new_interest(prefix=%s, suite=%s)\n",
                  ccnl_prefix_to_str((*pkt)->pfx, s, CCNL_MAX_PREFIX_SIZE),
                  ccnl_suite2str((*pkt)->pfx->suite));

    if (!i)
        return NULL;
    i->pkt = *pkt;
    /* currently, the aging function relies on seconds rather than on milli seconds */
    i->lifetime = (*pkt)->s.ndntlv.interestlifetime / 1000;
    *pkt = NULL;
    i->flags |= CCNL_PIT_COREPROPAGATES;
    i->from = from;
    i->last_used = CCNL_NOW();
    DBL_LINKED_LIST_ADD(ccnl->pit, i);

#ifdef CCNL_RIOT
    ccnl_evtimer_reset_interest_retrans(i);
    ccnl_evtimer_reset_interest_timeout(i);
#endif

    return i;
}

struct ccnl_interest_s*
ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i)
{
    struct ccnl_interest_s *i2;

/*
    if (!i)
        return NULL;
*/
    DEBUGMSG_CORE(TRACE, "ccnl_interest_remove %p\n", (void *) i);

/*
#ifdef USE_NFN
    if (!(i->flags & CCNL_PIT_COREPROPAGATES))
        return i->next;
#endif
*/

#ifdef CCNL_RIOT
    evtimer_del((evtimer_t *)(&ccnl_evtimer), (evtimer_event_t *)&i->evtmsg_retrans);
    evtimer_del((evtimer_t *)(&ccnl_evtimer), (evtimer_event_t *)&i->evtmsg_timeout);
#endif

    while (i->pending) {
        struct ccnl_pendint_s *tmp = i->pending->next;          \
        ccnl_free(i->pending);
        i->pending = tmp;
    }
    i2 = i->next;
    DBL_LINKED_LIST_REMOVE(ccnl->pit, i);

    if(i->pkt){
        ccnl_pkt_free(i->pkt);
    }
    if(i){
        ccnl_free(i);
    }
    return i2;
}

void
ccnl_interest_propagate(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i)
{
    struct ccnl_forward_s *fwd;
    int rc = 0;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

#if defined(USE_NACK) || defined(USE_RONR)
    int matching_face = 0;
#endif

    if (!i)
        return;
    DEBUGMSG_CORE(DEBUG, "ccnl_interest_propagate\n");

    // CONFORM: "A node MUST implement some strategy rule, even if it is only to
    // transmit an Interest Message on all listed dest faces in sequence."
    // CCNL strategy: we forward on all FWD entries with a prefix match

    for (fwd = ccnl->fib; fwd; fwd = fwd->next) {
        if (!fwd->prefix)
            continue;

        //Only for matching suite
        if (!i->pkt->pfx || fwd->suite != i->pkt->pfx->suite) {
            DEBUGMSG_CORE(VERBOSE, "  not same suite (%d/%d)\n",
                     fwd->suite, i->pkt->pfx ? i->pkt->pfx->suite : -1);
            continue;
        }

        rc = ccnl_prefix_cmp(fwd->prefix, NULL, i->pkt->pfx, CMP_LONGEST);

        DEBUGMSG_CORE(DEBUG, "  ccnl_interest_propagate, rc=%d/%d\n",
                 rc, fwd->prefix->compcnt);
        if (rc < fwd->prefix->compcnt)
            continue;

        DEBUGMSG_CORE(DEBUG, "  ccnl_interest_propagate, fwd==%p\n", (void*)fwd);
        // suppress forwarding to origin of interest, except wireless
        if (!i->from || fwd->face != i->from ||
                                (i->from->flags & CCNL_FACE_FLAGS_REFLECT)) {
            int nonce = 0;
            if (i->pkt != NULL && i->pkt->s.ndntlv.nonce != NULL) {
                if (i->pkt->s.ndntlv.nonce->datalen == 4) {
                    memcpy(&nonce, i->pkt->s.ndntlv.nonce->data, 4);
                }
            }

            DEBUGMSG_CFWD(INFO, "  outgoing interest=<%s> nonce=%i to=%s\n",
                          ccnl_prefix_to_str(i->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE), nonce,
                          fwd->face ? ccnl_addr2ascii(&fwd->face->peer)
                                    : "<tap>");
#ifdef USE_NFN_MONITOR
            ccnl_nfn_monitor(ccnl, fwd->face, i->pkt->pfx, NULL, 0);
#endif //USE_NFN_MONITOR

            // DEBUGMSG(DEBUG, "%p %p %p\n", (void*)i, (void*)i->pkt, (void*)i->pkt->buf);
            if (fwd->tap)
                (fwd->tap)(ccnl, i->from, i->pkt->pfx, i->pkt->buf);
            if (fwd->face)
                ccnl_send_pkt(ccnl, fwd->face, i->pkt);
#if defined(USE_NACK) || defined(USE_RONR)
            matching_face = 1;
#endif
        } else {
            DEBUGMSG_CORE(DEBUG, "  no matching fib entry found\n");
        }
    }

#ifdef USE_RONR
    if (!matching_face) {
        ccnl_interest_broadcast(ccnl, i);
    }
#endif

#ifdef USE_NACK
    if(!matching_face){
        ccnl_nack_reply(ccnl, i->pkt->pfx, i->from, i->pkt->pfx->suite);
        ccnl_interest_remove(ccnl, i);
    }
#endif

    return;
}

void
ccnl_interest_broadcast(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *interest)
{
    sockunion sun;
    struct ccnl_face_s *fibface = NULL;
    extern int ccnl_suite2defaultPort(int suite);
    unsigned i = 0;
    for (i = 0; i < CCNL_MAX_INTERFACES; i++) {
        switch (ccnl->ifs[i].addr.sa.sa_family) {
#ifdef USE_LINKLAYER 
#if !(defined(__FreeBSD__) || defined(__APPLE__))
            case (AF_PACKET): {
                /* initialize address with 0xFF for broadcast */
                uint8_t relay_addr[CCNL_MAX_ADDRESS_LEN];
                memset(relay_addr, CCNL_BROADCAST_OCTET, CCNL_MAX_ADDRESS_LEN);

                sun.sa.sa_family = AF_PACKET;
                memcpy(&(sun.linklayer.sll_addr), relay_addr, CCNL_MAX_ADDRESS_LEN);
                sun.linklayer.sll_halen = CCNL_MAX_ADDRESS_LEN;
                sun.linklayer.sll_protocol = htons(CCNL_ETH_TYPE);

                fibface = ccnl_get_face_or_create(ccnl, i, &sun.sa, sizeof(sun.linklayer));
                break;
                              }
#endif
#endif
#ifdef USE_WPAN
            case (AF_IEEE802154): {
                /* initialize address with 0xFF for broadcast */
                sun.sa.sa_family = AF_IEEE802154;
                sun.wpan.addr.addr_type = IEEE802154_ADDR_SHORT;
                sun.wpan.addr.pan_id = 0xffff;
                sun.wpan.addr.addr.short_addr = 0xffff;

                fibface = ccnl_get_face_or_create(ccnl, i, &sun.sa, sizeof(sun.wpan));
                break;
                              }
#endif
#ifdef USE_IPV4
            case (AF_INET):
                sun.sa.sa_family = AF_INET;
                sun.ip4.sin_addr.s_addr = INADDR_BROADCAST;
                sun.ip4.sin_port = htons(ccnl_suite2defaultPort(interest->pkt->suite));
                fibface = ccnl_get_face_or_create(ccnl, i, &sun.sa, sizeof(sun.ip4));
                break;
#endif
#ifdef USE_IPV6
            case (AF_INET6):
                sun.sa.sa_family = AF_INET6;
                sun.ip6.sin6_addr = in6addr_any;
                sun.ip6.sin6_port = ccnl_suite2defaultPort(interest->pkt->suite);
                fibface = ccnl_get_face_or_create(ccnl, i, &sun.sa, sizeof(sun.ip6));
                break;
#endif
        }
        if (fibface) {
            ccnl_send_pkt(ccnl, fibface, interest->pkt);
            DEBUGMSG_CORE(DEBUG, "  broadcasting interest (%s)\n", ccnl_addr2ascii(&sun));
        }
    }
}

struct ccnl_content_s*
ccnl_content_remove(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_content_s *c2;
    DEBUGMSG_CORE(TRACE, "ccnl_content_remove\n");

    c2 = c->next;
    DBL_LINKED_LIST_REMOVE(ccnl->contents, c);

//    free_content(c);
    if (c->pkt) {
        ccnl_prefix_free(c->pkt->pfx);
        ccnl_free(c->pkt->buf);
        ccnl_free(c->pkt);
    }
    //    ccnl_prefix_free(c->name);
    ccnl_free(c);

    ccnl->contentcnt--;
#ifdef CCNL_RIOT
    evtimer_del((evtimer_t *)(&ccnl_evtimer), (evtimer_event_t *)&c->evtmsg_cstimeout);
#endif
    return c2;
}

struct ccnl_content_s*
ccnl_content_add2cache(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_content_s *cit;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    DEBUGMSG_CORE(DEBUG, "ccnl_content_add2cache (%d/%d) --> %p = %s [%d]\n",
                  ccnl->contentcnt, ccnl->max_cache_entries,
                  (void*)c, ccnl_prefix_to_str(c->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE), (c->pkt->pfx->chunknum)? *(c->pkt->pfx->chunknum) : -1);

    for (cit = ccnl->contents; cit; cit = cit->next) {
        if (ccnl_prefix_cmp(c->pkt->pfx, NULL, cit->pkt->pfx, CMP_EXACT) == 0) {
            DEBUGMSG_CORE(DEBUG, "--- Already in cache ---\n");
            return NULL;
        }
    }
#ifdef USE_NACK
    if (ccnl_nfnprefix_contentIsNACK(c))
        return NULL;
#endif
    if (ccnl->max_cache_entries > 0 &&
        ccnl->contentcnt >= ccnl->max_cache_entries) { // remove oldest content
        struct ccnl_content_s *c2, *oldest = NULL;
        uint32_t age = 0;
        for (c2 = ccnl->contents; c2; c2 = c2->next) {
             if (!(c2->flags & CCNL_CONTENT_FLAGS_STATIC)) {
                 if ((age == 0) || c2->last_used < age) {
                     age = c2->last_used;
                     oldest = c2;
                 }
             }
         }
         if (oldest) {
             DEBUGMSG_CORE(DEBUG, " remove old entry from cache\n");
             ccnl_content_remove(ccnl, oldest);
         }
    }
    if ((ccnl->max_cache_entries <= 0) ||
         (ccnl->contentcnt <= ccnl->max_cache_entries)) {
            DBL_LINKED_LIST_ADD(ccnl->contents, c);
            ccnl->contentcnt++;
#ifdef CCNL_RIOT
            /* set cache timeout timer if content is not static */
            if (!(c->flags & CCNL_CONTENT_FLAGS_STATIC)) {
                ccnl_evtimer_set_cs_timeout(c);
            }
#endif
    }

    return c;
}

int
ccnl_content_serve_pending(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_interest_s *i;
    struct ccnl_face_s *f;
    int cnt = 0;
    DEBUGMSG_CORE(TRACE, "ccnl_content_serve_pending\n");
    char s[CCNL_MAX_PREFIX_SIZE];

 #ifdef USE_TIMEOUT_KEEPALIVE // TODO: shouldn't this be USE_NFN_REQUESTS?
     if (ccnl_nfnprefix_isIntermediate(c->pkt->pfx)) {
         return 1;   // don't forward incoming intermediate content, just cache
     }
 #endif

    for (f = ccnl->faces; f; f = f->next){
                f->flags &= ~CCNL_FACE_FLAGS_SERVED; // reply on a face only once
    }
    for (i = ccnl->pit; i;) {
        struct ccnl_pendint_s *pi;
        if (!i->pkt->pfx)
            continue;

#ifdef USE_NFN_REQUESTS
        if (ccnl_nfnprefix_isKeepalive(i->pkt->pfx) != ccnl_nfnprefix_isKeepalive(c->pkt->pfx)) {
            i = i->next;
            continue;
        }
        if (ccnl_nfnprefix_isIntermediate(i->pkt->pfx) != ccnl_nfnprefix_isIntermediate(c->pkt->pfx)) {
            i = i->next;
            continue;
        }
#endif

        switch (i->pkt->pfx->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            if (!ccnl_i_prefixof_c(i->pkt->pfx, i->pkt->s.ccnb.minsuffix,
                       i->pkt->s.ccnb.maxsuffix, c)) {
                // XX must also check i->ppkd
                i = i->next;
                continue;
            }
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            if (ccnl_prefix_cmp(c->pkt->pfx, NULL, i->pkt->pfx, CMP_EXACT)) {
                // XX must also check keyid
                i = i->next;
                continue;
            }
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            if (!ccnl_i_prefixof_c(i->pkt->pfx, i->pkt->s.ndntlv.minsuffix,
                       i->pkt->s.ndntlv.maxsuffix, c)) {
                // XX must also check i->ppkl,
                i = i->next;
                continue;
            }
            break;
#endif
        default:
            i = i->next;
            continue;
        }

        //Hook for add content to cache by callback:
        if(i && ! i->pending){
            DEBUGMSG_CORE(WARNING, "releasing interest 0x%p OK?\n", (void*)i);
            c->flags |= CCNL_CONTENT_FLAGS_STATIC;
            i = ccnl_interest_remove(ccnl, i);

            c->served_cnt++;
            cnt++;
            continue;
            //return 1;

        }

        // CONFORM: "Data MUST only be transmitted in response to
        // an Interest that matches the Data."
        for (pi = i->pending; pi; pi = pi->next) {
            if (pi->face->flags & CCNL_FACE_FLAGS_SERVED)
                continue;
            pi->face->flags |= CCNL_FACE_FLAGS_SERVED;
            if (pi->face->ifndx >= 0) {
                int32_t nonce = 0;
                if (i->pkt != NULL && i->pkt->s.ndntlv.nonce != NULL) {
                    if (i->pkt->s.ndntlv.nonce->datalen == 4) {
                        memcpy(&nonce, i->pkt->s.ndntlv.nonce->data, 4);
                    }
                }

#ifdef USE_NFN_REQUESTS
                struct ccnl_pkt_s *pkt = c->pkt;
                int matches_start_request = ccnl_nfnprefix_isRequest(i->pkt->pfx)
                                            && i->pkt->pfx->request->type == NFN_REQUEST_TYPE_START;
                if (matches_start_request) {
                    nfn_request_content_set_prefix(c, i->pkt->pfx);
                }
#endif
#ifndef CCNL_LINUXKERNEL
                DEBUGMSG_CFWD(INFO, "  outgoing data=<%s>%s nonce=%"PRIi32" to=%s\n",
                          ccnl_prefix_to_str(i->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE),
                          ccnl_suite2str(i->pkt->pfx->suite), nonce,
                          ccnl_addr2ascii(&pi->face->peer));
#else
                DEBUGMSG_CFWD(INFO, "  outgoing data=<%s>%s nonce=%d to=%s\n",
                          ccnl_prefix_to_str(i->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE),
                          ccnl_suite2str(i->pkt->pfx->suite), nonce,
                          ccnl_addr2ascii(&pi->face->peer));
#endif
                DEBUGMSG_CORE(VERBOSE, "    Serve to face: %d (pkt=%p)\n",
                         pi->face->faceid, (void*) c->pkt);
#ifdef USE_NFN_MONITOR
                ccnl_nfn_monitor(ccnl, pi->face, c->pkt->pfx,
                                 c->pkt->content, c->pkt->contlen);
#endif

                ccnl_send_pkt(ccnl, pi->face, c->pkt);

#ifdef USE_NFN_REQUESTS
                if (matches_start_request) {
                    ccnl_pkt_free(c->pkt);
                    c->pkt = pkt;
                }
#endif

            } else {// upcall to deliver content to local client
#ifdef CCNL_APP_RX
                ccnl_app_RX(ccnl, c);
#endif
            }
            c->served_cnt++;
            cnt++;
        }
        i = ccnl_interest_remove(ccnl, i);
    }

#ifdef USE_NFN_REQUESTS
    if (ccnl_nfnprefix_isIntermediate(c->pkt->pfx)) {
        return 1;   //
    }
#endif
    return cnt;
}

#define DEBUGMSG_AGEING(trace, debug, buf, buf_len)    \
DEBUGMSG_CORE(TRACE, "%s %p\n", (trace), (void*) i);   \
DEBUGMSG_CORE(DEBUG, " %s 0x%p <%s>\n", (debug),       \
    (void*)i,                                          \
    ccnl_prefix_to_str(i->pkt->pfx,buf,buf_len));

void
ccnl_do_ageing(void *ptr, void *dummy)
{

    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    struct ccnl_content_s *c = relay->contents;
    struct ccnl_interest_s *i = relay->pit;
    struct ccnl_face_s *f = relay->faces;
    time_t t = CCNL_NOW();
    DEBUGMSG_CORE(VERBOSE, "ageing t=%d\n", (int)t);
    (void) dummy;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    while (c) {
        if ((c->last_used + CCNL_CONTENT_TIMEOUT) <= (uint32_t) t &&
                                !(c->flags & CCNL_CONTENT_FLAGS_STATIC)){
            DEBUGMSG_CORE(TRACE, "AGING: CONTENT REMOVE %p\n", (void*) c);
            c = ccnl_content_remove(relay, c);
        }
        else {
#ifdef USE_SUITE_NDNTLV
            if (c->pkt->suite == CCNL_SUITE_NDNTLV) {
                if ((c->last_used + (c->pkt->s.ndntlv.freshnessperiod / 1000)) <= (uint32_t) t) {
                    c->stale = true;
                }
            }
#endif
            c = c->next;
        }
    }
    while (i) { // CONFORM: "Entries in the PIT MUST timeout rather
                // than being held indefinitely."
        if ((i->last_used + i->lifetime) <= (uint32_t) t ||
                                i->retries >= CCNL_MAX_INTEREST_RETRANSMIT) {
#ifdef USE_NFN_REQUESTS
                if (!ccnl_nfnprefix_isNFN(i->pkt->pfx)) {
                    DEBUGMSG_AGEING("AGING: REMOVE CCN INTEREST", "timeout: remove interest", s, CCNL_MAX_PREFIX_SIZE);
                    i = ccnl_nfn_interest_remove(relay, i);
                } else if (ccnl_nfnprefix_isIntermediate(i->pkt->pfx)) {
                    DEBUGMSG_AGEING("AGING: REMOVE INTERMEDIATE INTEREST", "timeout: remove interest", s, CCNL_MAX_PREFIX_SIZE);
                    i = ccnl_nfn_interest_remove(relay, i);
                } else if (!(ccnl_nfnprefix_isKeepalive(i->pkt->pfx))) {
                    if (i->keepalive == NULL) {
                        if (ccnl_nfn_already_computing(relay, i->pkt->pfx)) {
                            DEBUGMSG_AGEING("AGING: KEEP ALIVE INTEREST", "timeout: already computing", s, CCNL_MAX_PREFIX_SIZE);
                            i->last_used = CCNL_NOW();
                            i->retries = 0;
                        } else {
                            DEBUGMSG_AGEING("AGING: KEEP ALIVE INTEREST", "timeout: request status info", s, CCNL_MAX_PREFIX_SIZE);
                            ccnl_nfn_interest_keepalive(relay, i);
                        }
                    } else {
                        DEBUGMSG_AGEING("AGING: KEEP ALIVE INTEREST", "timeout: wait for status info", s, CCNL_MAX_PREFIX_SIZE);
                    }
                    i = i->next;
                } else {
                    DEBUGMSG_AGEING("AGING: REMOVE KEEP ALIVE INTEREST", "timeout: remove keep alive interest", s, CCNL_MAX_PREFIX_SIZE);
                    struct ccnl_interest_s *origin = i->keepalive_origin;
                    ccnl_nfn_interest_remove(relay, origin);
                    i = ccnl_nfn_interest_remove(relay, i);
                }
#else // USE_NFN_REQUESTS
                DEBUGMSG_AGEING("AGING: REMOVE INTEREST", "timeout: remove interest", s, CCNL_MAX_PREFIX_SIZE);
#ifdef USE_NFN
                i = ccnl_nfn_interest_remove(relay, i);
#else
                i = ccnl_interest_remove(relay, i);
#endif
#endif
        } else {
            // CONFORM: "A node MUST retransmit Interest Messages
            // periodically for pending PIT entries."
            DEBUGMSG_CORE(DEBUG, " retransmit %d <%s>\n", i->retries,
                     ccnl_prefix_to_str(i->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE));
#ifdef USE_NFN
            if (i->flags & CCNL_PIT_COREPROPAGATES){
#endif
                DEBUGMSG_CORE(TRACE, "AGING: PROPAGATING INTEREST %p\n", (void*) i);
                ccnl_interest_propagate(relay, i);
#ifdef USE_NFN
            }
#endif

            i->retries++;
            i = i->next;
        }
    }
    while (f) {
        if (!(f->flags & CCNL_FACE_FLAGS_STATIC) &&
                (f->last_used + CCNL_FACE_TIMEOUT) <= t){
            DEBUGMSG_CORE(TRACE, "AGING: FACE REMOVE %p\n", (void*) f);
            f = ccnl_face_remove(relay, f);
    }
        else
            f = f->next;
    }
}

int
ccnl_nonce_find_or_append(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *nonce)
{
    struct ccnl_buf_s *n, *n2 = 0;
    int i;
    DEBUGMSG_CORE(TRACE, "ccnl_nonce_find_or_append\n");

    for (n = ccnl->nonces, i = 0; n; n = n->next, i++) {
        if (buf_equal(n, nonce))
            return -1;
        if (n->next)
            n2 = n;
    }
    n = ccnl_buf_new(nonce->data, nonce->datalen);
    if (n) {
        n->next = ccnl->nonces;
        ccnl->nonces = n;
        if (i >= CCNL_MAX_NONCES && n2) {
            ccnl_free(n2->next);
            n2->next = 0;
        }
    }
    return 0;
}

int
ccnl_nonce_isDup(struct ccnl_relay_s *relay, struct ccnl_pkt_s *pkt)
{
    if(CCNL_MAX_NONCES < 0){
        struct ccnl_interest_s *i = NULL;
        for (i = relay->pit; i; i = i->next) {
            if(buf_equal(i->pkt->s.ndntlv.nonce, pkt->s.ndntlv.nonce)){
                return 1;
            }
        }
        return 0;
    }
    switch (pkt->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        return pkt->s.ccnb.nonce &&
            ccnl_nonce_find_or_append(relay, pkt->s.ccnb.nonce);
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        return pkt->s.ndntlv.nonce &&
            ccnl_nonce_find_or_append(relay, pkt->s.ndntlv.nonce);
#endif
    default:
        break;
    }
    return 0;
}

#ifdef NEEDS_PREFIX_MATCHING

/* add a new entry to the FIB */
int
ccnl_fib_add_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd, **fwd2;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    DEBUGMSG_CUTL(INFO, "adding FIB for <%s>, suite %s\n",
             ccnl_prefix_to_str(pfx,s,CCNL_MAX_PREFIX_SIZE), ccnl_suite2str(pfx->suite));

    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        if (fwd->suite == pfx->suite &&
                        !ccnl_prefix_cmp(fwd->prefix, NULL, pfx, CMP_EXACT)) {
            ccnl_prefix_free(fwd->prefix);
            fwd->prefix = NULL;
            break;
        }
    }
    if (!fwd) {
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
        if (!fwd)
            return -1;
        fwd2 = &relay->fib;
        while (*fwd2)
            fwd2 = &((*fwd2)->next);
        *fwd2 = fwd;
        fwd->suite = pfx->suite;
    }
    fwd->prefix = pfx;
    fwd->face = face;
    DEBUGMSG_CUTL(DEBUG, "added FIB via %s\n", ccnl_addr2ascii(&fwd->face->peer));

    return 0;
}

/* remove a new entry to the FIB */
int
ccnl_fib_rem_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd;
    int res = -1;
    struct ccnl_forward_s *last = NULL;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    if (pfx != NULL) {
        char *s = NULL;
        DEBUGMSG_CUTL(INFO, "removing FIB for <%s>, suite %s\n",
                      ccnl_prefix_to_str(pfx,s,CCNL_MAX_PREFIX_SIZE), ccnl_suite2str(pfx->suite));
    }

    for (fwd = relay->fib; fwd; last = fwd, fwd = fwd->next) {
        if (((pfx == NULL) || (fwd->suite == pfx->suite)) &&
            ((pfx == NULL) || !ccnl_prefix_cmp(fwd->prefix, NULL, pfx, CMP_EXACT)) &&
            ((face == NULL) || (fwd->face == face))) {
            res = 0;
            if (!last) {
                relay->fib = fwd->next;
            }
            else {
                last->next = fwd->next;
            }
            ccnl_prefix_free(fwd->prefix);
            ccnl_free(fwd);
            break;
        }
    }
    DEBUGMSG_CUTL(DEBUG, "added FIB via %s\n", ccnl_addr2ascii(&fwd->face->peer));

    return res;
}
#endif

/* prints the current FIB */
void
ccnl_fib_show(struct ccnl_relay_s *relay)
{
#ifndef CCNL_LINUXKERNEL
    char s[CCNL_MAX_PREFIX_SIZE];
    struct ccnl_forward_s *fwd;

    printf("%-30s | %-10s | %-9s | Peer\n",
           "Prefix", "Suite",
#ifdef CCNL_RIOT
           "Interface"
#else
           "Socket"
#endif
           );
    puts("-------------------------------|------------|-----------|------------------------------------");

    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        printf("%-30s | %-10s |        %02i | %s\n", ccnl_prefix_to_str(fwd->prefix,s,CCNL_MAX_PREFIX_SIZE),
                                     ccnl_suite2str(fwd->suite), (int)
                                     /* TODO: show correct interface instead of always 0 */
#ifdef CCNL_RIOT
                                     (relay->ifs[0]).if_pid,
#else
                                     (relay->ifs[0]).sock,
#endif
                                     ccnl_addr2ascii(&fwd->face->peer));
    }
#endif
}

void
ccnl_cs_dump(struct ccnl_relay_s *ccnl)
{
#ifndef CCNL_LINUXKERNEL
    struct ccnl_content_s *c = ccnl->contents;
    unsigned i = 0;
    char s[CCNL_MAX_PREFIX_SIZE];
    (void) s;

    while (c) {
        printf("CS[%u]: %s [%d]: %.*s\n", i++,
               ccnl_prefix_to_str(c->pkt->pfx,s,CCNL_MAX_PREFIX_SIZE),
               (c->pkt->pfx->chunknum)? *(c->pkt->pfx->chunknum) : -1,
               c->pkt->contlen, c->pkt->content);
        c = c->next;
    }
#endif
}

void
ccnl_interface_CTS(void *aux1, void *aux2)
{
    struct ccnl_relay_s *ccnl = (struct ccnl_relay_s *)aux1;
    struct ccnl_if_s *ifc = (struct ccnl_if_s *)aux2;
    struct ccnl_txrequest_s *r, req;

    DEBUGMSG_CORE(TRACE, "interface_CTS interface=%p, qlen=%d, sched=%p\n",
             (void*)ifc, ifc->qlen, (void*)ifc->sched);

    if (ifc->qlen <= 0)
        return;

#ifdef USE_STATS
    ifc->tx_cnt++;
#endif

    r = ifc->queue + ifc->qfront;
    memcpy(&req, r, sizeof(req));
    ifc->qfront = (ifc->qfront + 1) % CCNL_MAX_IF_QLEN;
    ifc->qlen--;
#ifndef CCNL_LINUXKERNEL
    assert(ccnl->ccnl_ll_TX_ptr != 0);
#endif
    ccnl->ccnl_ll_TX_ptr(ccnl, ifc, &req.dst, req.buf);
#ifdef USE_SCHEDULER
    ccnl_sched_CTS_done(ifc->sched, 1, req.buf->datalen);
    if (req.txdone)
        req.txdone(req.txdone_face, 1, req.buf->datalen);
#endif
    ccnl_free(req.buf);
}

int
ccnl_cs_add(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_content_s *content;

    content = ccnl_content_add2cache(ccnl, c);
    if (content) {
        ccnl_content_serve_pending(ccnl, content);
        return 0;
    }

    return -1;
}

int
ccnl_cs_remove(struct ccnl_relay_s *ccnl, char *prefix)
{
    struct ccnl_content_s *c;

    if (!ccnl || !prefix) {
        return -1;
    }

    for (c = ccnl->contents; c; c = c->next) {
        char *spref = ccnl_prefix_to_path(c->pkt->pfx);
        if (!spref) {
            return -2;
        }
        if (memcmp(prefix, spref, strlen(spref)) == 0) {
            ccnl_free(spref);
            ccnl_content_remove(ccnl, c);
            return 0;
        }
        ccnl_free(spref);
    }
    return -3;
}

struct ccnl_content_s *
ccnl_cs_lookup(struct ccnl_relay_s *ccnl, char *prefix)
{
    struct ccnl_content_s *c;

    if (!ccnl || !prefix) {
        return NULL;
    }

    for (c = ccnl->contents; c; c = c->next) {
        char *spref = ccnl_prefix_to_path(c->pkt->pfx);
        if (!spref) {
            return NULL;
        }
        if (memcmp(prefix, spref, strlen(spref)) == 0) {
            ccnl_free(spref);
            return c;
        }
        ccnl_free(spref);
    }
    return NULL;
}
