/*
 * @f ccnl-core.c
 * @b CCN lite, core CCNx protocol logic
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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
 * 2011-04-09 created
 * 2013-10-12 add crypto support <christopher.scherb@unibas.ch>
 * 2014-03-20 started to move ccnx (pre 2014) specific routines to "fwd-ccnb.c"
 */

#ifndef USE_NFN
# define ccnl_nfn_interest_remove(r,i)  ccnl_interest_remove(r,i)
#endif

// forward reference:
void ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);

// ----------------------------------------------------------------------
// datastructure support functions

#define buf_dup(B)      (B) ? ccnl_buf_new(B->data, B->datalen) : NULL
#define buf_equal(X,Y)  ((X) && (Y) && (X->datalen==Y->datalen) &&\
                         !memcmp(X->data,Y->data,X->datalen))

struct ccnl_prefix_s* ccnl_prefix_new(int suite, int cnt);

int
ccnl_prefix_cmp(struct ccnl_prefix_s *name, unsigned char *md,
                struct ccnl_prefix_s *p, int mode)
/* returns -1 if no match at all (all modes) or exact match failed
   returns  0 if full match (CMP_EXACT)
   returns n>0 for matched components (CMP_MATCH, CMP_LONGEST) */
{
    int i, clen, nlen = name->compcnt + (md ? 1 : 0), rc = -1;
    unsigned char *comp;

    if (mode == CMP_EXACT) {
        if (nlen != p->compcnt)
            goto done;
#ifdef USE_NFN
        if (p->nfnflags != name->nfnflags)
            goto done;
#endif
    }
    for (i = 0; i < nlen && i < p->compcnt; ++i) {
        comp = i < name->compcnt ? name->comp[i] : md;
        clen = i < name->compcnt ? name->complen[i] : 32; // SHA256_DIGEST_LEN
        if (clen != p->complen[i] || memcmp(comp, p->comp[i], p->complen[i])) {
            rc = mode == CMP_EXACT ? -1 : i;
            goto done;
        }
    }
    rc = (mode == CMP_EXACT) ? 0 : i;
done:
    DEBUGMSG(VERBOSE, "ccnl_prefix_cmp (mode=%d, nlen=%d, plen=%d, %d), name=%s"
             " prefix=%s: %d (%p)\n", mode, nlen, p->compcnt, name->compcnt,
             ccnl_prefix_to_path_detailed(name, 0, 0, 0), ccnl_prefix_to_path_detailed(p, 0, 0, 0), rc, md);
    return rc;
}

// ----------------------------------------------------------------------
// addresses, interfaces and faces

int
ccnl_addr_cmp(sockunion *s1, sockunion *s2)
{
    if (s1->sa.sa_family != s2->sa.sa_family)
        return -1;
    switch (s1->sa.sa_family) {
#ifdef USE_ETHERNET
        case AF_PACKET:
            return memcmp(s1->eth.sll_addr, s2->eth.sll_addr, ETH_ALEN);
#endif
        case AF_INET:
            return s1->ip4.sin_addr.s_addr == s2->ip4.sin_addr.s_addr &&
                        s1->ip4.sin_port == s2->ip4.sin_port ? 0 : -1;
#ifdef USE_UNIXSOCKET
        case AF_UNIX:
            return strcmp(s1->ux.sun_path, s2->ux.sun_path);
#endif
        default:
            break;
    }
    return -1;
}

struct ccnl_face_s*
ccnl_get_face_or_create(struct ccnl_relay_s *ccnl, int ifndx,
                       struct sockaddr *sa, int addrlen)
// sa==NULL means: local(=in memory) client, search for existing ifndx being -1
// sa!=NULL && ifndx==-1: search suitable interface for given sa_family
// sa!=NULL && ifndx!=-1: use this (incoming) interface for outgoing
{
    static int seqno, i;
    struct ccnl_face_s *f;
    DEBUGMSG(TRACE, "ccnl_get_face_or_create src=%s\n",
             sa ? ccnl_addr2ascii((sockunion*)sa) : "(local)");

    for (f = ccnl->faces; f; f = f->next) {
        if (!sa) {
            if (f->ifndx == -1)
                return f;
            continue;
        }
        if (ifndx != -1 && !ccnl_addr_cmp(&f->peer, (sockunion*)sa)) {
            f->last_used = CCNL_NOW();
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
    DEBUGMSG(VERBOSE, "  found suitable interface %d for %s\n", ifndx,
             sa ? ccnl_addr2ascii((sockunion*)sa) : "(local)");

    f = (struct ccnl_face_s *) ccnl_calloc(1, sizeof(struct ccnl_face_s));
    if (!f)
        return NULL;
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
    return f;
}

struct ccnl_face_s*
ccnl_face_remove(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_face_s *f2;
    struct ccnl_interest_s *pit;
    struct ccnl_forward_s **ppfwd;

    DEBUGMSG(DEBUG, "face_remove relay=%p face=%p\n",
             (void*)ccnl, (void*)f);

    ccnl_sched_destroy(f->sched);
    ccnl_frag_destroy(f->frag);

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
        else
            pit = ccnl_nfn_interest_remove(ccnl, pit);
    }
    for (ppfwd = &ccnl->fib; *ppfwd;) {
        if ((*ppfwd)->face == f) {
            struct ccnl_forward_s *pfwd = *ppfwd;
            free_prefix(pfwd->prefix);
            *ppfwd = pfwd->next;
            ccnl_free(pfwd);
        } else
            ppfwd = &(*ppfwd)->next;
    }
    while (f->outq) {
        struct ccnl_buf_s *tmp = f->outq->next;
        ccnl_free(f->outq);
        f->outq = tmp;
    }
    f2 = f->next;
    DBL_LINKED_LIST_REMOVE(ccnl->faces, f);
    ccnl_free(f);
    return f2;
}

void
ccnl_interface_cleanup(struct ccnl_if_s *i)
{
    int j;
    DEBUGMSG(TRACE, "ccnl_interface_cleanup\n");

    ccnl_sched_destroy(i->sched);
    for (j = 0; j < i->qlen; j++) {
        struct ccnl_txrequest_s *r = i->queue + (i->qfront+j)%CCNL_MAX_IF_QLEN;
        ccnl_free(r->buf);
    }
    ccnl_close_socket(i->sock);
}

// ----------------------------------------------------------------------
// face and interface queues, scheduling

void
ccnl_interface_CTS(void *aux1, void *aux2)
{
    struct ccnl_relay_s *ccnl = (struct ccnl_relay_s *)aux1;
    struct ccnl_if_s *ifc = (struct ccnl_if_s *)aux2;
    struct ccnl_txrequest_s *r, req;

    DEBUGMSG(TRACE, "interface_CTS interface=%p, qlen=%d, sched=%p\n",
             (void*)ifc, ifc->qlen, (void*)ifc->sched);

    if (ifc->qlen <= 0)
        return;
    r = ifc->queue + ifc->qfront;
    memcpy(&req, r, sizeof(req));
    ifc->qfront = (ifc->qfront + 1) % CCNL_MAX_IF_QLEN;
    ifc->qlen--;

    ccnl_ll_TX(ccnl, ifc, &req.dst, req.buf);
#ifdef USE_SCHEDULER
    ccnl_sched_CTS_done(ifc->sched, 1, req.buf->datalen);
    if (req.txdone)
        req.txdone(req.txdone_face, 1, req.buf->datalen);
#endif
    ccnl_free(req.buf);
}

void
ccnl_interface_enqueue(void (tx_done)(void*, int, int), struct ccnl_face_s *f,
                       struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
                       struct ccnl_buf_s *buf, sockunion *dest)
{
    struct ccnl_txrequest_s *r;
    DEBUGMSG(TRACE, "enqueue interface=%p buf=%p len=%d (qlen=%d)\n",
             (void*)ifc, (void*)buf, buf->datalen, ifc->qlen);

    if (ifc->qlen >= CCNL_MAX_IF_QLEN) {
        DEBUGMSG(WARNING, "  DROPPING buf=%p\n", (void*)buf);
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
    DEBUGMSG(TRACE, "dequeue face=%p (id=%d.%d)\n",
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
    DEBUGMSG(TRACE, "CTS_done face=%p cnt=%d len=%d\n", ptr, cnt, len);

#ifdef USE_SCHEDULER
    struct ccnl_face_s *f = (struct ccnl_face_s*) ptr;
    ccnl_sched_CTS_done(f->sched, cnt, len);
#endif
}

void
ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_buf_s *buf;
    DEBUGMSG(TRACE, "CTS face=%p sched=%p\n", (void*)f, (void*)f->sched);

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
ccnl_face_enqueue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
                 struct ccnl_buf_s *buf)
{
    struct ccnl_buf_s *msg;
    DEBUGMSG(TRACE, "enqueue face=%p (id=%d.%d) buf=%p len=%d\n",
             (void*) to, ccnl->id, to->faceid, (void*) buf, buf->datalen);

    for (msg = to->outq; msg; msg = msg->next) // already in the queue?
        if (buf_equal(msg, buf)) {
            DEBUGMSG(VERBOSE, "    not enqueued because already there\n");
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

// ----------------------------------------------------------------------
// handling of interest messages

struct ccnl_interest_s*
ccnl_interest_new(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
                  char suite,
                  struct ccnl_buf_s **pkt, struct ccnl_prefix_s **prefix,
                  int minsuffix, int maxsuffix)
{
    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
                                            sizeof(struct ccnl_interest_s));
    DEBUGMSG(TRACE, "ccnl_new_interest\n");

    if (!i){
        return NULL;
    }
#ifdef USE_NFN
    i->corePropagates = 1;
#endif
    i->suite = suite;
    i->from = from;
    i->prefix = *prefix;        *prefix = 0;
    i->pkt  = *pkt;             *pkt = 0;
    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        i->details.ccnb.minsuffix = minsuffix;
        i->details.ccnb.maxsuffix = maxsuffix;
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        i->details.ndntlv.minsuffix = minsuffix;
        i->details.ndntlv.maxsuffix = maxsuffix;
        break;
#endif
    }
    i->last_used = CCNL_NOW();
    DBL_LINKED_LIST_ADD(ccnl->pit, i);
    return i;
}

int
ccnl_interest_append_pending(struct ccnl_interest_s *i,
                             struct ccnl_face_s *from)
{
    struct ccnl_pendint_s *pi, *last = NULL;
    DEBUGMSG(TRACE, "ccnl_append_pending\n");

    for (pi = i->pending; pi; pi = pi->next) { // check whether already listed
        if (pi->face == from) {
            DEBUGMSG(DEBUG, "  we found a matching interest, updating time\n");
            pi->last_used = CCNL_NOW();
            return 0;
        }
        last = pi;
    }
    pi = (struct ccnl_pendint_s *) ccnl_calloc(1,sizeof(struct ccnl_pendint_s));
    DEBUGMSG(DEBUG, "  appending a new pendint entry %p\n", (void *) pi);
    if (!pi)
        return -1;
    pi->face = from;
    pi->last_used = CCNL_NOW();
    if (last)
        last->next = pi;
    else
        i->pending = pi;
    return 0;
}

void
ccnl_interest_propagate(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i)
{
    struct ccnl_forward_s *fwd;
    int rc = 0;
#ifdef USE_NACK
    int matching_face = 0;
#endif
    DEBUGMSG(DEBUG, "ccnl_interest_propagate\n");

    // CONFORM: "A node MUST implement some strategy rule, even if it is only to
    // transmit an Interest Message on all listed dest faces in sequence."
    // CCNL strategy: we forward on all FWD entries with a prefix match

    for (fwd = ccnl->fib; fwd; fwd = fwd->next) {

        //Only for matching suite
        if (fwd->suite != i->suite) {
            DEBUGMSG(DEBUG, "  not same suite (%d/%d)\n", fwd->suite, i->suite);
            continue;
        }

        rc = ccnl_prefix_cmp(fwd->prefix, NULL, i->prefix, CMP_LONGEST);

        DEBUGMSG(DEBUG, "  ccnl_interest_propagate, rc=%d/%d\n",
                 rc, fwd->prefix->compcnt);
        if (rc < fwd->prefix->compcnt)
            continue;
        DEBUGMSG(DEBUG, "  ccnl_interest_propagate, fwd==%p\n", (void*)fwd);
        // suppress forwarding to origin of interest, except wireless
        if (!i->from || fwd->face != i->from ||
                                (i->from->flags & CCNL_FACE_FLAGS_REFLECT)) {
            ccnl_nfn_monitor(ccnl, fwd->face, i->prefix, NULL, 0);
            ccnl_face_enqueue(ccnl, fwd->face, buf_dup(i->pkt));
#ifdef USE_NACK
            matching_face = 1;
#endif
        }

    }

#ifdef USE_NACK
    if(!matching_face){
        ccnl_nack_reply(ccnl, i->prefix, i->from, i->suite);
        ccnl_interest_remove(ccnl, i);
    }
#endif

    return;
}

struct ccnl_interest_s*
ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i)
{
    struct ccnl_interest_s *i2;

    DEBUGMSG(TRACE, "ccnl_interest_remove %p\n", (void *) i);
/*
#ifdef USE_NFN
    if (i->corePropagates == 0)
        return i->next;
#endif
*/
    while (i->pending) {
        struct ccnl_pendint_s *tmp = i->pending->next;          \
        ccnl_free(i->pending);
        i->pending = tmp;
    }
    i2 = i->next;
    DBL_LINKED_LIST_REMOVE(ccnl->pit, i);
    free_prefix(i->prefix);

    switch (i->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB: ccnl_free(i->details.ccnb.ppkd); break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: ccnl_free(i->details.ccntlv.keyid); break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV: ccnl_free(i->details.ndntlv.ppkl); break;
#endif
    default:
        break;
    }
    free_2ptr_list(i->pkt, i);
    return i2;
}

// ----------------------------------------------------------------------
// handling of content messages

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
                  int minsuffix, int maxsuffix, struct ccnl_content_s *c)
{
    unsigned char *md;
    DEBUGMSG(TRACE, "ccnl_i_prefixof_c prefix=%s min=%d max=%d\n",
             ccnl_prefix_to_path(prefix), minsuffix, maxsuffix);

    // CONFORM: we do prefix match, honour min. and maxsuffix,

    // NON-CONFORM: "Note that to match a ContentObject must satisfy
    // all of the specifications given in the Interest Message."
    // >> CCNL does not honour the exclusion filtering

    if ( (prefix->compcnt + minsuffix) > (c->name->compcnt + 1) ||
         (prefix->compcnt + maxsuffix) < (c->name->compcnt + 1) )
        return 0;

    md = prefix->compcnt - c->name->compcnt == 1 ? compute_ccnx_digest(c->pkt) : NULL;
    return ccnl_prefix_cmp(c->name, md, prefix, CMP_MATCH) == prefix->compcnt;
}

struct ccnl_content_s*
ccnl_content_new(struct ccnl_relay_s *ccnl, char suite, struct ccnl_buf_s **pkt,
                 struct ccnl_prefix_s **prefix, struct ccnl_buf_s **ppk,
                 unsigned char *content, int contlen)
{
    struct ccnl_content_s *c;
    DEBUGMSG(TRACE, "ccnl_content_new <%s>, %d bytes (pktlen=%d)\n",
             prefix==NULL ? NULL : ccnl_prefix_to_path(*prefix),
             contlen, *pkt ? (*pkt)->datalen : -1);

    c = (struct ccnl_content_s *) ccnl_calloc(1, sizeof(struct ccnl_content_s));
    if (!c) return NULL;
    c->suite = suite;
    c->last_used = CCNL_NOW();
    c->content = content;
    c->contentlen = contlen;
    c->pkt = *pkt;        *pkt = NULL;
    c->name = *prefix;    *prefix = NULL;
    if (ppk) {
        switch (suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB: c->details.ccnb.ppkd = *ppk; break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: c->details.ccntlv.keyid = *ppk; break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV: c->details.ndntlv.ppkl = *ppk; break;
#endif
        }
        *ppk = NULL;
    }
    return c;
}

struct ccnl_content_s*
ccnl_content_remove(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_content_s *c2;
    DEBUGMSG(TRACE, "ccnl_content_remove\n");

    c2 = c->next;
    DBL_LINKED_LIST_REMOVE(ccnl->contents, c);
    free_content(c);
    ccnl->contentcnt--;
    return c2;
}

struct ccnl_content_s*
ccnl_content_add2cache(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_content_s *cit;
    DEBUGMSG(DEBUG, "ccnl_content_add2cache (%d/%d) --> %p\n",
             ccnl->contentcnt, ccnl->max_cache_entries, (void*)c);
    for (cit = ccnl->contents; cit; cit = cit->next) {
        if (c == cit) {
            DEBUGMSG(DEBUG, "--- Already in cache ---\n");
            return NULL;
        }
    }
#ifdef USE_NACK
    if (ccnl_nfnprefix_contentIsNACK(c))
        return NULL;
#endif
    if (ccnl->max_cache_entries > 0 &&
        ccnl->contentcnt >= ccnl->max_cache_entries) { // remove oldest content
        struct ccnl_content_s *c2;
        int age = 0;
        for (c2 = ccnl->contents; c2; c2 = c2->next)
            if (!(c2->flags & CCNL_CONTENT_FLAGS_STATIC) &&
                                        ((age == 0) || c2->last_used < age))
                age = c2->last_used;
        if (c2)
            ccnl_content_remove(ccnl, c2);
    }
    DBL_LINKED_LIST_ADD(ccnl->contents, c);
    ccnl->contentcnt++;
    return c;
}

// deliver new content c to all clients with (loosely) matching interest,
// but only one copy per face
// returns: number of forwards
int
ccnl_content_serve_pending(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    struct ccnl_interest_s *i;
    struct ccnl_face_s *f;
    int cnt = 0;
    DEBUGMSG(TRACE, "ccnl_content_serve_pending\n");

    for (f = ccnl->faces; f; f = f->next){
                f->flags &= ~CCNL_FACE_FLAGS_SERVED; // reply on a face only once
    }
    for (i = ccnl->pit; i;) {
        struct ccnl_pendint_s *pi;

        switch (i->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            if (!ccnl_i_prefixof_c(i->prefix, i->details.ccnb.minsuffix,
                       i->details.ccnb.maxsuffix, c)) {
                // XX must also check i->ppkd
                i = i->next;
                continue;
            }
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            if (ccnl_prefix_cmp(c->name, NULL, i->prefix, CMP_EXACT)) {
                // XX must also check keyid
                i = i->next;
                continue;
            }
            break;
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV:
            if (ccnl_prefix_cmp(c->name, NULL, i->prefix, CMP_EXACT)) {
                // XX must also check keyid
                i = i->next;
                continue;
            }
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            if (!ccnl_i_prefixof_c(i->prefix, i->details.ndntlv.minsuffix,
                       i->details.ndntlv.maxsuffix, c)) {
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
            c->flags |= CCNL_CONTENT_FLAGS_STATIC;
            i = ccnl_interest_remove(ccnl, i);
            return 1;
        }

        // CONFORM: "Data MUST only be transmitted in response to
        // an Interest that matches the Data."
        for (pi = i->pending; pi; pi = pi->next) {
            if (pi->face->flags & CCNL_FACE_FLAGS_SERVED)
            continue;
            pi->face->flags |= CCNL_FACE_FLAGS_SERVED;
            if (pi->face->ifndx >= 0) {
                DEBUGMSG(DEBUG, "  forwarding content <%s>\n",
                         ccnl_prefix_to_path(c->name));

                DEBUGMSG(VERBOSE, "--- Serve to face: %d (buf=%p)\n",
                         pi->face->faceid, (void*) c->pkt);
                ccnl_nfn_monitor(ccnl, pi->face, c->name,
                                 c->content, c->contentlen);
                ccnl_face_enqueue(ccnl, pi->face, buf_dup(c->pkt));
            } else {// upcall to deliver content to local client
                ccnl_app_RX(ccnl, c);
            }
            c->served_cnt++;
            cnt++;
        }
        i = ccnl_interest_remove(ccnl, i);
    }
    return cnt;
}

void
ccnl_do_ageing(void *ptr, void *dummy)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;
    struct ccnl_content_s *c = relay->contents;
    struct ccnl_interest_s *i = relay->pit;
    struct ccnl_face_s *f = relay->faces;
    time_t t = CCNL_NOW();
    DEBUGMSG(TRACE, "ageing t=%d\n", (int)t);

    while (c) {
        if ((c->last_used + CCNL_CONTENT_TIMEOUT) <= t &&
                                !(c->flags & CCNL_CONTENT_FLAGS_STATIC))
            c = ccnl_content_remove(relay, c);
        else
            c = c->next;
    }
    while (i) { // CONFORM: "Entries in the PIT MUST timeout rather
                // than being held indefinitely."
        if ((i->last_used + CCNL_INTEREST_TIMEOUT) <= t ||
                                i->retries > CCNL_MAX_INTEREST_RETRANSMIT) {
            i = ccnl_nfn_interest_remove(relay, i);
        } else {
            // CONFORM: "A node MUST retransmit Interest Messages
            // periodically for pending PIT entries."
            DEBUGMSG(DEBUG, " retransmit %d <%s>\n", i->retries,
                     ccnl_prefix_to_path(i->prefix));
#ifdef USE_NFN
            if (i->corePropagates)
#endif
                ccnl_interest_propagate(relay, i);

            i->retries++;
            i = i->next;
        }
    }
    while (f) {
        if (!(f->flags & CCNL_FACE_FLAGS_STATIC) &&
                (f->last_used + CCNL_FACE_TIMEOUT) <= t){
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
    DEBUGMSG(TRACE, "ccnl_nonce_find_or_append\n");

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

// ----------------------------------------------------------------------
// dispatching the different formats (and respective forwarding semantics):

#include "ccnl-pkt-switch.c"

#include "ccnl-pkt-ccnb.c"
#include "ccnl-pkt-ccntlv.c"
#include "ccnl-pkt-iottlv.c"
#include "ccnl-pkt-ndntlv.c"
#include "ccnl-pkt-localrpc.c" // must come after pkt-ndntlv.c

#include "ccnl-core-fwd.c"

typedef int (*dispatchFct)(struct ccnl_relay_s*, struct ccnl_face_s*,
                           unsigned char**, int*);
dispatchFct ccnl_core_RX_dispatch[CCNL_SUITE_LAST];

void
ccnl_core_RX(struct ccnl_relay_s *relay, int ifndx, unsigned char *data,
             int datalen, struct sockaddr *sa, int addrlen)
{
    struct ccnl_face_s *from;
    int enc, suite = -1, skip;
    dispatchFct dispatch;

    DEBUGMSG(DEBUG, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

    from = ccnl_get_face_or_create(relay, ifndx, sa, addrlen);
    if (!from)
        return;

    // loop through all packets in the received frame (UDP, Ethernet etc)
    while (datalen > 0) {
        // work through explicit code switching
        while (!ccnl_switch_dehead(&data, &datalen, &enc))
            suite = ccnl_enc2suite(enc);
        if (suite == -1)
            suite = ccnl_pkt2suite(data, datalen, &skip);

        if (suite < 0 || suite >= CCNL_SUITE_LAST) {
            DEBUGMSG(WARNING, "?unknown packet format? ccnl_core_RX ifndx=%d, %d bytes starting with 0x%02x at offset %zd\n",
                     ifndx, datalen, *data, data - base);
            return;
        }
        dispatch = ccnl_core_RX_dispatch[suite];
        if (!dispatch) {
            DEBUGMSG(ERROR, "Forwarder not initialized or dispatcher "
                     "for suite %s does not exist.\n", ccnl_suite2str(suite));
            return;
        }
        if (dispatch(relay, from, &data, &datalen) < 0)
            break;
        if (datalen > 0) {
            DEBUGMSG(WARNING, "ccnl_core_RX: %d bytes left\n", datalen);
        }
    }
}

// ----------------------------------------------------------------------

void
ccnl_core_init(void)
{
#ifdef USE_SUITE_CCNB
    ccnl_core_RX_dispatch[CCNL_SUITE_CCNB]     = ccnl_ccnb_forwarder;
#endif
#ifdef USE_SUITE_CCNTLV
    ccnl_core_RX_dispatch[CCNL_SUITE_CCNTLV]   = ccnl_ccntlv_forwarder;
#endif
#ifdef USE_SUITE_IOTTLV
    ccnl_core_RX_dispatch[CCNL_SUITE_IOTTLV]   = ccnl_iottlv_forwarder;
#endif
#ifdef USE_SUITE_LOCALRPC
    ccnl_core_RX_dispatch[CCNL_SUITE_LOCALRPC] = ccnl_localrpc_exec;
#endif
#ifdef USE_SUITE_NDNTLV
    ccnl_core_RX_dispatch[CCNL_SUITE_NDNTLV]   = ccnl_ndntlv_forwarder;
#endif

#ifdef USE_NFN
    ZAM_init();
#endif
}

struct ccnl_buf_s *bufCleanUpList;

void
ccnl_core_addToCleanup(struct ccnl_buf_s *buf)
{
    buf->next = bufCleanUpList;
    bufCleanUpList = buf;
}

void
ccnl_core_cleanup(struct ccnl_relay_s *ccnl)
{
    int k;
    DEBUGMSG(TRACE, "ccnl_core_cleanup %p\n", (void *) ccnl);

    while (ccnl->pit)
        ccnl_interest_remove(ccnl, ccnl->pit);
    while (ccnl->faces)
        ccnl_face_remove(ccnl, ccnl->faces); // also removes all FWD entries
    while (ccnl->contents)
        ccnl_content_remove(ccnl, ccnl->contents);
    while (ccnl->nonces) {
        struct ccnl_buf_s *tmp = ccnl->nonces->next;
        ccnl_free(ccnl->nonces);
        ccnl->nonces = tmp;
    }
    for (k = 0; k < ccnl->ifcount; k++)
        ccnl_interface_cleanup(ccnl->ifs + k);

    while (bufCleanUpList) {
        struct ccnl_buf_s *tmp = bufCleanUpList->next;
        ccnl_free(bufCleanUpList);
        bufCleanUpList = tmp;
    }

#ifdef USE_NFN
    ccnl_nfn_freeKrivine(ccnl);
#endif
}

#include "ccnl-core-util.c"

// eof
