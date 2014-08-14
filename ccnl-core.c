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

#include "ccnl-core.h"


#define CCNL_VERSION "2014-03-20"

#ifdef CCNL_NFN
#include "krivine-common.h"
#endif

#ifdef CCNL_NFN_MONITOR
#include "json.c"
#endif

int
ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
      struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
        struct configuration_s *config, struct ccnl_interest_s *interest,
         int suite, int start_locally);

void
ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid, int continue_from_remove);

void
ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                                  struct configuration_s *config, int suite);

void ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                     struct ccnl_face_s *from, int suite);

int
ccnl_content_serve_pending(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

struct ccnl_content_s*
ccnl_content_add2cache(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

static struct ccnl_interest_s* 
ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

static struct ccnl_interest_s* 
ccnl_interest_remove_continue_computations(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

// ----------------------------------------------------------------------
// datastructure support functions

#ifndef USE_DEBUG_MALLOC
struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = (struct ccnl_buf_s *) ccnl_malloc(sizeof(*b) + len);

    if (!b) return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data) memcpy(b->data, data, len);
    return b;
}
#endif

#define buf_dup(B)	(B) ? ccnl_buf_new(B->data, B->datalen) : NULL
#define buf_equal(X,Y)	((X) && (Y) && (X->datalen==Y->datalen) &&\
			 !memcmp(X->data,Y->data,X->datalen))

int
ccnl_prefix_cmp(struct ccnl_prefix_s *name, unsigned char *md,
		struct ccnl_prefix_s *p, int mode)
/* returns -1 if no match at all (all modes) or exact match failed
   returns  0 if full match (CMP_EXACT)
   returns n>0 for matched components (CMP_MATCH, CMP_LONGEST) */
{
    int i, clen, nlen = name->compcnt + (md ? 1 : 0), rc = -1;
    unsigned char *comp;

    if (mode == CMP_EXACT && nlen != p->compcnt)
	goto done;
    for (i = 0; i < nlen && i < p->compcnt; i++) {
	comp = i < name->compcnt ? name->comp[i] : md;
	clen = i < name->compcnt ? name->complen[i] : 32; // SHA256_DIGEST_LEN
	if (clen != p->complen[i] || memcmp(comp, p->comp[i], p->complen[i])) {
	    rc = mode == CMP_EXACT ? -1 : i;
	    goto done;
	}
    }
    rc = (mode == CMP_EXACT) ? 0 : i;
done:
    DEBUGMSG(49, "ccnl_prefix_cmp (mode=%d, nlen=%d, plen=%d, %d), name=%s prefix=%s: %d (%p)\n",
	     mode, nlen, p->compcnt, name->compcnt,
	     ccnl_prefix_to_path(name), ccnl_prefix_to_path(p), rc, md);
    return rc;
}

// ----------------------------------------------------------------------
// addresses, interfaces and faces

static void ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);

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
    DEBUGMSG(10, "ccnl_get_face_or_create src=%s\n",
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
    DEBUGMSG(99, "  found suitable interface %d for %s\n", ifndx,
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
	if (ccnl->ifs[ifndx].reflect)	f->flags |= CCNL_FACE_FLAGS_REFLECT;
	if (ccnl->ifs[ifndx].fwdalli)	f->flags |= CCNL_FACE_FLAGS_FWDALLI;
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
    DEBUGMSG(1, "ccnl_face_remove relay=%p face=%p\n", (void*)ccnl, (void*)f);

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
	    pit = ccnl_interest_remove_continue_computations(ccnl, pit);
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
    DEBUGMSG(99, "ccnl_interface_cleanup\n");

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
    DEBUGMSG(25, "ccnl_interface_CTS interface=%p, qlen=%d, sched=%p\n",
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
    DEBUGMSG(9, "ccnl_interface_enqueue interface=%p buf=%p len=%d (qlen=%d)\n",
	     (void*)ifc, (void*)buf, buf->datalen, ifc->qlen);

    if (ifc->qlen >= CCNL_MAX_IF_QLEN) {
	DEBUGMSG(2, "  DROPPING buf=%p\n", (void*)buf);
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
    DEBUGMSG(20, "ccnl_face_dequeue face=%p (id=%d.%d)\n",
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
    DEBUGMSG(99, "ccnl_face_CTS_done face=%p cnt=%d len=%d\n", ptr, cnt, len);

#ifdef USE_SCHEDULER
    struct ccnl_face_s *f = (struct ccnl_face_s*) ptr;
    ccnl_sched_CTS_done(f->sched, cnt, len);
#endif
}

void
ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_buf_s *buf;
    DEBUGMSG(99, "ccnl_face_CTS face=%p sched=%p\n", (void*)f, (void*)f->sched);

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
    DEBUGMSG(20, "ccnl_face_enqueue face=%p (id=%d.%d) buf=%p len=%d\n",
	     (void*) to, ccnl->id, to->faceid, (void*) buf, buf->datalen);

    for (msg = to->outq; msg; msg = msg->next) // already in the queue?
	if (buf_equal(msg, buf)) {
	    DEBUGMSG(31, "    not enqueued because already there\n");
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
    DEBUGMSG(99, "ccnl_new_interest\n");

    if (!i){
        return NULL;
    }
#ifdef CCNL_NFN
    i->propagate = 1;
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
    DEBUGMSG(99, "ccnl_append_pending\n");

    for (pi = i->pending; pi; pi = pi->next) { // check whether already listed
	if (pi->face == from) {
	    DEBUGMSG(40, "  we found a matching interest, updating time\n");
	    pi->last_used = CCNL_NOW();
	    return 0;
	}
	last = pi;
    }
    pi = (struct ccnl_pendint_s *) ccnl_calloc(1,sizeof(struct ccnl_pendint_s));
    DEBUGMSG(40, "  appending a new pendint entry %p\n", (void *) pi);
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
    int matching_face = 0;
    DEBUGMSG(99, "ccnl_interest_propagate\n");

    ccnl_print_stats(ccnl, STAT_SND_I); // log_send_i

    // CONFORM: "A node MUST implement some strategy rule, even if it is only to
    // transmit an Interest Message on all listed dest faces in sequence."
    // CCNL strategy: we forward on all FWD entries with a prefix match

    for (fwd = ccnl->fib; fwd; fwd = fwd->next) {
        int rc = ccnl_prefix_cmp(fwd->prefix, NULL, i->prefix, CMP_LONGEST);

        DEBUGMSG(40, "  ccnl_interest_propagate, rc=%d/%d\n", rc, fwd->prefix->compcnt);
        if (rc < fwd->prefix->compcnt)
            continue;
        DEBUGMSG(40, "  ccnl_interest_propagate, fwd==%p\n", (void*)fwd);
        // suppress forwarding to origin of interest, except wireless
        if (!i->from || fwd->face != i->from ||
            (i->from->flags & CCNL_FACE_FLAGS_REFLECT)){
#ifdef CCNL_NFN_MONITOR
                char monitorpacket[CCNL_MAX_PACKET_SIZE];
                int l = create_packet_log(inet_ntoa(fwd->face->peer.ip4.sin_addr),
                        ntohs(fwd->face->peer.ip4.sin_port),
                        i->prefix, NULL, 0, monitorpacket);
                sendtomonitor(ccnl, monitorpacket, l);
#endif
                ccnl_face_enqueue(ccnl, fwd->face, buf_dup(i->pkt));
                matching_face = 1;
        }

    }

#ifdef CCNL_NACK
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
#ifdef CCNL_NFN
    if(i->propagate == 0) return i->next;
#endif
    struct ccnl_interest_s *i2;
    int it;
    DEBUGMSG(40, "ccnl_interest_remove %p   ", (void *) i);

    for(it = 0; it < i->prefix->compcnt; ++it){
        fprintf(stderr, "/%s", i->prefix->comp[it]);
    }
    fprintf(stderr, "\n");
    while (i->pending) {
        struct ccnl_pendint_s *tmp = i->pending->next;		\
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
//        case CCNL_SUITE_CCNTLV: ccnl_free(i->details.ccntlv.ppkd); break;
#ifdef USE_SUITE_NDNTLV
       case CCNL_SUITE_NDNTLV: ccnl_free(i->details.ndntlv.ppkl); break;
#endif
    }
    free_2ptr_list(i->pkt, i);
    return i2;
}

struct ccnl_interest_s*
ccnl_interest_remove_continue_computations(struct ccnl_relay_s *ccnl, 
		        struct ccnl_interest_s *i){
    struct ccnl_interest_s *interest;
    int faceid = 0;
    DEBUGMSG(99, "ccnl_interest_remove_continue_computations()\n");

#ifdef CCNL_NFN
    if(i != 0 && i->from != 0){
            faceid = i->from->faceid;
    }
#endif
#ifdef CCNL_NACK
    DEBUGMSG(99, "FROM FACEID: %d\n", faceid);
    if(faceid >= 0){
        ccnl_nack_reply(ccnl, i->prefix, i->from, i->suite);
    }
#endif
    interest = ccnl_interest_remove(ccnl, i);
#ifdef CCNL_NFN
    if(faceid < 0){
            ccnl_nfn_continue_computation(ccnl, -i->from->faceid, 1);
    }
#endif
   return interest;
}

// ----------------------------------------------------------------------
// handling of content messages

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
		  int minsuffix, int maxsuffix, struct ccnl_content_s *c)
{
    unsigned char *md;
    DEBUGMSG(99, "ccnl_i_prefixof_c prefix=%s min=%d max=%d\n",
	     ccnl_prefix_to_path(prefix), minsuffix, maxsuffix);

    // CONFORM: we do prefix match, honour min. and maxsuffix,
    // and check the PublisherPublicKeyDigest if present

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
    DEBUGMSG(99, "ccnl_content_new <%s>\n",
         prefix==NULL ? NULL : ccnl_prefix_to_path(*prefix));

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
    DEBUGMSG(99, "ccnl_content_remove\n");

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
    DEBUGMSG(99, "ccnl_content_add2cache (%d/%d) --> %p\n",
         ccnl->contentcnt, ccnl->max_cache_entries, (void*)c);
    for(cit = ccnl->contents; cit; cit = cit->next){
        //DEBUGMSG(99, "--- Already in cache ---\n");
        if(c == cit) return NULL;
    }
#ifdef CCNL_NACK
    if(!strncmp(c->content, ":NACK", 5)){
        return NULL;
    }
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
    DEBUGMSG(99, "ccnl_content_serve_pending\n");

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
        // CONFORM: "Data MUST only be transmitted in response to
        // an Interest that matches the Data."
        for (pi = i->pending; pi; pi = pi->next) {
            if (pi->face->flags & CCNL_FACE_FLAGS_SERVED)
            continue;
            pi->face->flags |= CCNL_FACE_FLAGS_SERVED;
            if (pi->face->ifndx >= 0) {
                DEBUGMSG(6, "  forwarding content <%s>\n",
                 ccnl_prefix_to_path(c->name));
                 ccnl_print_stats(ccnl, STAT_SND_C); //log sent c

            DEBUGMSG(99, "--- Serve to face: %d\n", pi->face->faceid);
    #ifdef CCNL_NFN_MONITOR
                 char monitorpacket[CCNL_MAX_PACKET_SIZE];
                 int l = create_packet_log(inet_ntoa(pi->face->peer.ip4.sin_addr),
                        ntohs(pi->face->peer.ip4.sin_port),
                        c->name, (char*)c->content, c->contentlen, monitorpacket);
                 sendtomonitor(ccnl, monitorpacket, l);
    #endif
                 ccnl_face_enqueue(ccnl, pi->face, buf_dup(c->pkt));
            } else {// upcall to deliver content to local client
                ccnl_app_RX(ccnl, c);
            }
            c->served_cnt++;
            cnt++;
        }
        i = ccnl_interest_remove(ccnl, i);
    }
    DEBUGMSG(99, "Served: %d\n", cnt);
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
    DEBUGMSG(999, "ccnl_do_ageing %d\n", (int)t);

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
				i->retries > CCNL_MAX_INTEREST_RETRANSMIT){
            i = ccnl_interest_remove_continue_computations(relay, i);
        }
	else {
	    // CONFORM: "A node MUST retransmit Interest Messages
	    // periodically for pending PIT entries."
	    DEBUGMSG(7, " retransmit %d <%s>\n", i->retries, ccnl_prefix_to_path(i->prefix));
#ifdef CCNL_NFN
	    if(i->propagate) ccnl_interest_propagate(relay, i);
#else
            ccnl_interest_propagate(relay, i);
#endif
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
    DEBUGMSG(99, "ccnl_nonce_find_or_append\n");

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

void
ccnl_core_cleanup(struct ccnl_relay_s *ccnl)
{
    int k;
    DEBUGMSG(99, "ccnl_core_cleanup %p\n", (void *) ccnl);

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
}

// ----------------------------------------------------------------------
// dispatching the different formats (and respective forwarding semantics):


#ifdef USE_SUITE_CCNB
#  include "fwd-ccnb.c"
#endif

#ifdef USE_SUITE_NDNTLV
#  include "fwd-ndntlv.c"
#endif

int
ccnl_pkt2suite(unsigned char *data, int len)
{
#ifdef USE_SUITE_CCNB
    if (*data == 0x01 || *data == 0x04)
	return CCNL_SUITE_CCNB;
#endif

#ifdef USE_SUITE_CCNTLV
    if (data[0] == 0 && len > 1) {
	if (data[1] == CCNX_TLV_TL_Interest ||
	    data[1] == CCNX_TLV_TL_Object)
	    return CCNL_SUITE_CCNTLV;
    }
#endif

#ifdef USE_SUITE_NDNTLV
    if (*data == 0x05 || *data == 0x06)
	return CCNL_SUITE_NDNTLV;
#endif

    return -1;
}

void
ccnl_core_RX(struct ccnl_relay_s *relay, int ifndx, unsigned char *data,
	     int datalen, struct sockaddr *sa, int addrlen)
{
    struct ccnl_face_s *from;
    DEBUGMSG(14, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

    from = ccnl_get_face_or_create(relay, ifndx, sa, addrlen);
    if (!from) return;

    switch (ccnl_pkt2suite(data, datalen)) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
	ccnl_RX_ccnb(relay, from, &data, &datalen); break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
	ccnl_RX_ccntlv(relay, from, &data, &datalen); break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
	ccnl_RX_ndntlv(relay, from, &data, &datalen); break;
#endif
    default:
	DEBUGMSG(6, "?unknown packet? ccnl_core_RX ifndx=%d, %d bytes %d\n",
		 ifndx, datalen, *data);
	break;
    }
}

#ifdef CCNL_NACK
#include "ccnl-objects.c"

void ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                     struct ccnl_face_s *from, int suite){
    DEBUGMSG(99, "ccnl_nack_reply()\n");
    if(from->faceid <= 0){
        return;
    }
    struct ccnl_content_s *nack = create_content_object(ccnl, prefix, ":NACK", 5, suite);
#ifdef CCNL_NFN_MONITOR
     char monitorpacket[CCNL_MAX_PACKET_SIZE];
     int l = create_packet_log(inet_ntoa(from->peer.ip4.sin_addr),
            ntohs(from->peer.ip4.sin_port),
            nack->name, (char*)nack->content, nack->contentlen, monitorpacket);
     sendtomonitor(ccnl, monitorpacket, l);
#endif
    ccnl_face_enqueue(ccnl, from, nack->pkt);
}
#endif
// eof
