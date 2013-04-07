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
 */

static struct ccnl_interest_s* ccnl_interest_remove(struct ccnl_relay_s *ccnl,
						    struct ccnl_interest_s *i);

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
ccnl_prefix_cmp(struct ccnl_prefix_s *p1, struct ccnl_prefix_s *p2, char exact)
/* returns -1 if no match at all, or exact match failed
   returns  0 if full match
   returns n>0 for matched components */
{
    int i, maxc;
    DEBUGMSG(99, "ccnl_prefix_cmp\n");

    if (exact && p1->compcnt != p2->compcnt)
	return -1;
    maxc = p1->compcnt;
    if (maxc > p2->compcnt)
	maxc = p2->compcnt;
    if (maxc <= 0)
	return -1;
    for (i = 0; i < maxc; i++) {
	if (p1->complen[i] != p2->complen[i] ||
			memcmp(p1->comp[i], p2->comp[i], p1->complen[i]))
	    return (i == 0 || exact) ? -1 : i;
    }
    if (p1->compcnt == p2->compcnt)
	return 0;
    return exact ? -1 : i;
}

// ----------------------------------------------------------------------
// ccnb parsing support

static int consume(int typ, int num, unsigned char **buf, int *len,
		   unsigned char **valptr, int *vallen);

static int
dehead(unsigned char **buf, int *len, int *num, int *typ)
{
    int i;
    int val = 0;

    if (*len > 0 && **buf == 0) {
	*num = *typ = 0;
	*buf += 1;
	*len -= 1;
	return 0;
    }
    for (i = 0; (unsigned int) i < sizeof(i) && i < *len; i++) {
	unsigned char c = (*buf)[i];
	if ( c & 0x80 ) {
	    *num = (val << 4) | ((c >> 3) & 0xf);
	    *typ = c & 0x7;
	    *buf += i+1;
	    *len -= i+1;
	    return 0;
	}
	val = (val << 7) | c;
    } 
    return -1;
}

static int
hunt_for_end(unsigned char **buf, int *len,
	     unsigned char **valptr, int *vallen)
{
    int typ, num;

    while (dehead(buf, len, &num, &typ) == 0) {
	if (num==0 && typ==0)					return 0;
	if (consume(typ, num, buf, len, valptr, vallen) < 0)	return -1;
    }
    return -1;
}

static int
consume(int typ, int num, unsigned char **buf, int *len,
	unsigned char **valptr, int *vallen)
{
    if (typ == CCN_TT_BLOB || typ == CCN_TT_UDATA) {
	if (valptr)  *valptr = *buf;
	if (vallen)  *vallen = num;
	*buf += num, *len -= num;
	return 0;
    }
    if (typ == CCN_TT_DTAG || typ == CCN_TT_DATTR)
	return hunt_for_end(buf, len, valptr, vallen);
//  case CCN_TT_TAG, CCN_TT_ATTR:
//  case DTAG, DATTR:
    return -1;
}

int
ccnl_extract_prefix_nonce_ppkd(struct ccnl_buf_s *pkt,
			      struct ccnl_prefix_s **prefix,
			      struct ccnl_buf_s **nonce,
			      struct ccnl_buf_s **ppkd,
			      unsigned char **content,
    			      int *contlen)
{
    unsigned char *data = pkt->data, *cp;
    int num, typ, len, datalen = pkt->datalen;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *n = 0, *pub = 0;
    DEBUGMSG(99, "ccnl_extract_prefix\n");

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) return -1;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
					   sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    if (dehead(&data, &datalen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG) goto Bail;
    if (num != CCN_DTAG_INTEREST && num != CCN_DTAG_CONTENTOBJ) goto Bail;

    while (dehead(&data, &datalen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
	    for (;;) {
		if (dehead(&data, &datalen, &num, &typ) != 0) goto Bail;
		if (num==0 && typ==0)
		    break;
		if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
		    p->compcnt < CCNL_MAX_NAME_COMP) {
		    if (consume(typ, num, &data, &datalen,
				p->comp + p->compcnt,
				p->complen + p->compcnt) < 0) goto Bail;
		    p->compcnt++;
		} else {
		    if (consume(typ, num, &data, &datalen, 0, 0) < 0) goto Bail;
		}
	    }
	    continue;
	}
	if (typ == CCN_TT_DTAG && num == CCN_DTAG_NONCE && !n) {
	    if (consume(typ, num, &data, &datalen, &cp, &len) < 0) goto Bail;
	    n = ccnl_buf_new(cp, len);
	    continue;
	}
	if (typ == CCN_TT_DTAG && num == CCN_DTAG_PUBPUBKDIGEST && !pub) {
	    if (consume(typ, num, &data, &datalen, &cp, &len) < 0) goto Bail;
	    pub = ccnl_buf_new(cp, len);
	    continue;
	}
	if (typ == CCN_TT_DTAG && num == CCN_DTAG_CONTENT) {
	    if (dehead(&data, &datalen, &num, &typ) != 0) goto Bail;
	    if (typ != CCN_TT_BLOB) goto Bail;
	    if (content) *content = data;
	    if (contlen) *contlen = num;
	    if (consume(typ, num, &data, &datalen, 0, 0) < 0) goto Bail;
	    continue;
	}
	if (consume(typ, num, &data, &datalen, 0, 0) < 0) goto Bail;
    }
    if (prefix)    *prefix = p;    else free_prefix(p);
    if (nonce)     *nonce = n;     else ccnl_free(n);
    if (ppkd)      *ppkd = pub;    else ccnl_free(pub);
    return 0;
Bail:
    free_prefix(p);
    free_2ptr_list(n, pub);
    return -1;
}

struct ccnl_prefix_s*
ccnl_prefix_clone(struct ccnl_prefix_s *p)
{
    int i, len;
    struct ccnl_prefix_s *p2;

    p2 = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p2) return NULL;
    for (i = 0, len = 0; i < p->compcnt; i++)
	len += p->complen[i];
    p2->path = (unsigned char*) ccnl_malloc(len);
    p2->comp = (unsigned char**) ccnl_malloc(p->compcnt*sizeof(char *));
    p2->complen = (int*) ccnl_malloc(p->compcnt*sizeof(int));
    if (!p2->comp || !p2->complen || !p2->path) goto Bail;
    p2->compcnt = p->compcnt;
    for (i = 0, len = 0; i < p->compcnt; len += p2->complen[i++]) {
	p2->complen[i] = p->complen[i];
	p2->comp[i] = p2->path + len;
	memcpy(p2->comp[i], p->comp[i], p2->complen[i]);
    }
    return p2;
Bail:
    free_prefix(p2);
    return NULL;
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
	case AF_UNIX:
	    return strcmp(s1->ux.sun_path, s2->ux.sun_path);
	default:
	    break;
    }
    return -1;
}

struct ccnl_face_s*
ccnl_get_face_or_create(struct ccnl_relay_s *ccnl, int ifndx,
		       struct sockaddr *sa, int addrlen, int encaps)
// sa==NULL means: local(=in memory) client, search for existing ifndx being -1
// sa!=NULL && ifndx==-1: search suitable interface for given sa_family
// sa!=NULL && ifndx!=-1: use this (incoming) interface for outgoing
{
    static int seqno;
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
	int i;
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
#ifdef USE_ENCAPS
    f->encaps = ccnl_encaps_new(encaps, ccnl->ifs[ifndx].mtu);
#endif

#ifdef USE_SCHEDULER
    if (ifndx >= 0 && ccnl->defaultFaceScheduler)
	f->sched = ccnl->defaultFaceScheduler(ccnl, (void(*)(void*,void*))ccnl_face_CTS);
#endif
    if (ccnl->ifs[ifndx].reflect)	f->flags |= CCNL_FACE_FLAGS_REFLECT;
    if (ccnl->ifs[ifndx].fwdalli)	f->flags |= CCNL_FACE_FLAGS_FWDALLI;

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

#ifdef USE_SCHEDULER
    ccnl_sched_destroy(f->sched);
#endif
#ifdef USE_ENCAPS
    ccnl_encaps_destroy(f->encaps);
#endif
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
	    pit = ccnl_interest_remove(ccnl, pit);
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

#ifdef USE_SCHEDULER
    ccnl_sched_destroy(i->sched);
#endif
    for (j = 0; j < i->qlen; j++) {
	struct ccnl_txrequest_s *r = i->queue + (i->qfront+j)%CCNL_MAX_IF_QLEN;
	ccnl_free(r->buf);
    }
}

// ----------------------------------------------------------------------
// face and interface queues, scheduling

void
ccnl_interface_CTS(void *aux1, void *aux2)
{
    struct ccnl_relay_s *ccnl = (struct ccnl_relay_s *)aux1;
    struct ccnl_if_s *ifc = (struct ccnl_if_s *)aux2;
    struct ccnl_txrequest_s *r, req;
    DEBUGMSG(99, "ccnl_interface_CTS interface=%p, qlen=%d, sched=%p\n",
	     (void*)ifc, ifc->qlen, (void*)ifc->sched);

    if (ifc->qlen <= 0)
	return;
    r = ifc->queue + ifc->qfront;
    memcpy(&req, r, sizeof(req));
    ifc->qfront = (ifc->qfront + 1) % CCNL_MAX_IF_QLEN;
    ifc->qlen--;

//    ccnl_ll_TX(ccnl, ifc, &req.dst, req.buf->data, req.buf->datalen);
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
    DEBUGMSG(2, "ccnl_interface_enqueue interface=%p buf=%p (qlen=%d)\n",
	     (void*)ifc, (void*)buf, ifc->qlen);

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
ccnl_face_dequeue(struct ccnl_face_s *f)
{
    struct ccnl_buf_s *pkt;
    DEBUGMSG(99, "ccnl_face_dequeue face=%p\n", (void *) f);

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
    DEBUGMSG(2, "ccnl_face_CTS_done face=%p cnt=%d len=%d\n", ptr, cnt, len);

#ifdef USE_SCHEDULER
    {
	struct ccnl_face_s *f = (struct ccnl_face_s*) ptr;
	ccnl_sched_CTS_done(f->sched, cnt, len);
    }
#endif
}

void
ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f)
{
    struct ccnl_buf_s *buf;
    DEBUGMSG(2, "ccnl_face_CTS face=%p sched=%p\n", (void*)f, (void*)f->sched);

    if (!f->encaps) {
	buf = ccnl_face_dequeue(f);
	if (buf)
	    ccnl_interface_enqueue(ccnl_face_CTS_done, f,
				   ccnl, ccnl->ifs + f->ifndx, buf, &f->peer);
    }
#ifdef USE_ENCAPS
    else {
	sockunion dst;
	int ifndx = f->ifndx;
	buf = ccnl_encaps_getnextfragment(f->encaps, &ifndx, &dst);
	if (!buf) {
	    buf = ccnl_face_dequeue(f);
	    ccnl_encaps_start(f->encaps, buf, f->ifndx, &f->peer);
	    buf = ccnl_encaps_getnextfragment(f->encaps, &ifndx, &dst);
	}
	if (buf)
	    ccnl_interface_enqueue(ccnl_face_CTS_done, f,
				   ccnl, ccnl->ifs + ifndx, buf, &dst);
    }
#endif
}

int
ccnl_face_enqueue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
		 struct ccnl_buf_s *buf)
{
    struct ccnl_buf_s *msg;
    DEBUGMSG(2, "ccnl_face_enqueue face=%p buf=%p\n", (void*) to, (void*) buf);

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
#ifdef USE_ENCAPS
	int len, cnt = ccnl_encaps_getfragcount(to->encaps, buf->datalen, &len);
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

struct ccnl_interest_s*
ccnl_interest_new(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
		 struct ccnl_buf_s **data, struct ccnl_prefix_s **prefix,
		 struct ccnl_buf_s **ppkd)
{
    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
					    sizeof(struct ccnl_interest_s));
    DEBUGMSG(99, "ccnl_new_interest\n");

    if (!i)
	return NULL;
    i->from = from;
    i->prefix = *prefix;        *prefix = 0;
    i->data = *data;            *data = 0;
    i->ppkd = *ppkd;		*ppkd = 0;
    i->last_used = CCNL_NOW();
    DBL_LINKED_LIST_ADD(ccnl->pit, i);
    return i;
}

struct ccnl_interest_s*
ccnl_interest_find_exact(struct ccnl_relay_s *ccnl,
			 struct ccnl_prefix_s *prefix, struct ccnl_buf_s *ppkd)
{
    struct ccnl_interest_s *i; // , *besti = NULL;
    int rc; // , bestmatch = -1;
    DEBUGMSG(99, "ccnl_interest_find_exact\n");

    for (i = ccnl->pit; i; i = i->next) {
	rc = ccnl_prefix_cmp(prefix, i->prefix, EXACT_MATCH);
	if (rc == 0) {
	    if (!ppkd || !i->ppkd || buf_equal(ppkd, i->ppkd))
		return i;
	    continue;
	}
    }
    return NULL;
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
    struct ccnl_forward_s *fwd, *best = NULL;
    int maxmatch = -1;
    DEBUGMSG(99, "ccnl_interest_propagate\n");

    ccnl_print_stats(ccnl, STAT_SND_I); // log_send_i

    // CONFORM: "A node MUST implement some strategy rule, even if it is only to
    // transmit an Interest Message on all listed dest faces in sequence."
    // CCNL strategy: we forward on the face(s) with the longest prefix match
    for (fwd = ccnl->fib; fwd; fwd = fwd->next) {
	int rc = ccnl_prefix_cmp(fwd->prefix, i->prefix, LOOSE_MATCH);
	if (rc == 0) {
	    best = fwd;
	    break;
	} else if (rc > maxmatch) {
	    maxmatch = rc;
	    best = fwd;
	}
    }
    DEBUGMSG(40, "  ccnl_interest_propagate, best=%p %d\n",
	     (void *) best, maxmatch);
    if (!best)
	return;
    for (fwd = ccnl->fib; fwd; fwd = fwd->next) {
	int rc = ccnl_prefix_cmp(fwd->prefix, i->prefix, LOOSE_MATCH);
	if (rc == maxmatch) {
	    // suppress forwarding to origin of interest, except wireless
	    if (!i->from || fwd->face != i->from ||
				(i->from->flags & CCNL_FACE_FLAGS_REFLECT))
		ccnl_face_enqueue(ccnl, fwd->face, buf_dup(i->data));
	}
    }
    return;
}

struct ccnl_interest_s*
ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i)
{
    struct ccnl_interest_s *i2;
    DEBUGMSG(40, "ccnl_interest_remove %p\n", (void *) i);

    while (i->pending) {
	struct ccnl_pendint_s *tmp = i->pending->next;		\
	ccnl_free(i->pending);
	i->pending = tmp;
    }
    i2 = i->next;
    DBL_LINKED_LIST_REMOVE(ccnl->pit, i);
    free_prefix(i->prefix);
    //    free_4ptr_list(i->nonce, i->ppkd, i->data, i);
    free_3ptr_list(i->ppkd, i->data, i);
    return i2;
}

// ----------------------------------------------------------------------
// handling of content messages

struct ccnl_content_s*
ccnl_content_find_exact(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
		       struct ccnl_buf_s *ppkd)
{
    struct ccnl_content_s *c;
    int rc;
    DEBUGMSG(99, "ccnl_content_find_exact\n");

    for (c = ccnl->contents; c; c = c->next) {
	rc = ccnl_prefix_cmp(prefix, c->prefix, EXACT_MATCH);
	if (rc == 0 && (!ppkd || !c->ppkd || buf_equal(ppkd, c->ppkd)))
	    return c;
    }
    return NULL;
}

struct ccnl_content_s*
ccnl_content_new(struct ccnl_relay_s *ccnl, struct ccnl_buf_s **data,
		struct ccnl_prefix_s **prefix, struct ccnl_buf_s **ppkd,
		unsigned char *content, int contlen)
{
    struct ccnl_content_s *c;
    DEBUGMSG(99, "ccnl_content_new <%s>\n",
	     prefix==NULL ? NULL : ccnl_prefix_to_path(*prefix));

    c = (struct ccnl_content_s *) ccnl_calloc(1, sizeof(struct ccnl_content_s));
    if (!c) return NULL;
    c->last_used = CCNL_NOW();
    c->data = *data;
    *data = NULL;
    c->content = content;
    c->contentlen = contlen;
    c->prefix = *prefix;
    *prefix = NULL;
    if (ppkd) {
	c->ppkd = *ppkd;
	*ppkd = NULL;
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
    DEBUGMSG(99, "ccnl_content_add2cache (%d/%d)\n",
	     ccnl->contentcnt, ccnl->max_cache_entries);

    if (ccnl->max_cache_entries > 0 &&
	ccnl->contentcnt >= ccnl->max_cache_entries) { // remove oldest content
	struct ccnl_content_s /* *old = NULL,*/ *c2;
	int age = 0;
	for (c2 = ccnl->contents; c2; c2 = c2->next)
	    if (!(c2->flags & CCNL_CONTENT_FLAGS_STATIC) &&
					((age == 0) || c2->last_used < age))
		/* old = c2, */ age = c2->last_used;
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

    for (f = ccnl->faces; f; f = f->next)
	f->flags &= ~CCNL_FACE_FLAGS_SERVED; // reply on a face only once
    for (i = ccnl->pit; i;) {
	int rc = ccnl_prefix_cmp(c->prefix, i->prefix, LOOSE_MATCH);
	DEBUGMSG(40, "  serve_pending, prefix match returned %d\n", rc);
	if (rc != -1 && (rc == 0 || c->prefix->compcnt > i->prefix->compcnt)) {
	    struct ccnl_pendint_s *pi;
	    // CONFORM: "If the PublisherPublicKeyDigest element is present
	    // in the Interest, the associated value (a SHA-256 digest) must
	    // be equal to the value of the PublisherPublicKeyDigest of the
	    // ContentObject for there to be a match."
	    if (i->ppkd && !buf_equal(i->ppkd, c->ppkd)) {
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
		    ccnl_print_stats(ccnl, STAT_SND_C); //log sent c
		    ccnl_face_enqueue(ccnl, pi->face, buf_dup(c->data));
		}
		else // upcall to deliver content to local client
		    ccnl_app_RX(ccnl, c);
		c->served_cnt++;
		cnt++;
	    }
	    i = ccnl_interest_remove(ccnl, i);
	} else
	    i = i->next;
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
				i->retries > CCNL_MAX_INTEREST_RETRANSMIT)
	    i = ccnl_interest_remove(relay, i);
	else {
	    // CONFORM: "A node MUST retransmit Interest Messages
	    // periodically for pending PIT entries."
	    DEBUGMSG(1, " retransmit %d\n", i->retries);
	    ccnl_interest_propagate(relay, i);
	    i->retries++;
	    i = i->next;
	}
    }
    while (f) {
	if (!(f->flags & CCNL_FACE_FLAGS_STATIC) &&
				(f->last_used + CCNL_FACE_TIMEOUT) <= t)
	    f = ccnl_face_remove(relay, f);
	else
	    f = f->next;
    }
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
// the core of CCN:

#define testbuf(P,X,Y)	(P->datalen>1 && P->data[0] == X && P->data[1] == Y)
#define ccnl_is_interest(BUF)	testbuf(BUF, 0x01, 0xd2)
#define ccnl_is_content(BUF)	testbuf(BUF, 0x04, 0x82)

int
ccnl_core_RX(struct ccnl_relay_s *relay, int ifndx,
	      unsigned char *data, int datalen,
	      struct sockaddr *sa, int addrlen)
{
    int rc = -1;
    struct ccnl_buf_s *buf = 0;
    struct ccnl_prefix_s *prefix = 0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_face_s *from = 0;
    struct ccnl_buf_s *nonce=0, *ppkd=0;
    unsigned char *content;
    int contlen;
    DEBUGMSG(99, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

    from = ccnl_get_face_or_create(relay, ifndx, sa, addrlen,
			ccnl_is_fragment(data, datalen) ?
			CCNL_ENCAPS_SEQUENCED2012 : CCNL_ENCAPS_NONE);
    if (!from) goto Done;

    DEBUGMSG(5, "  %d bytes, if=%d, face=%p [0x%02x 0x%02x]\n",
	     datalen, from->ifndx, (void*) from, data[0], data[1]);

#ifdef USE_ENCAPS
    buf = ccnl_encaps_handle_fragment(relay, from, data, datalen);
#else
    buf = ccnl_buf_new(data, datalen);
#endif
    if (!buf) goto Done;
    if (ccnl_extract_prefix_nonce_ppkd(buf, &prefix, &nonce, &ppkd,
				      &content, &contlen) || !prefix) {
	DEBUGMSG(6, "  parsing error or no prefix\n"); goto Done;
    }
    if (ccnl_is_interest(buf)) {
	if (prefix->compcnt == 4 && !memcmp(prefix->comp[0], "ccnx", 4)) {
	    rc = ccnl_mgmt(relay, buf, prefix, from);
	    goto Done;
	}
	DEBUGMSG(6, "  interest for <%s>\n", ccnl_prefix_to_path(prefix));
	ccnl_print_stats(relay, STAT_RCV_I); //log count recv_interest
	if (nonce && ccnl_nonce_find_or_append(relay, nonce)) {
	    DEBUGMSG(1, "  dropped because of duplicate nonce\n"); goto Done;
	}
	// CONFORM: Step 1:
	c = ccnl_content_find_exact(relay, prefix, ppkd);
	// NON-CONFORM: "Note that to match a ContentObject must satisfy
	// all of the specifications given in the Interest Message."
	// >> CCNL only considers prefix and PublisherPubKDigest <<
	if (c) { // we have the content, return to interest sending face
	    DEBUGMSG(5, "  matching content for interest, content %p\n", (void *) c);
	    ccnl_print_stats(relay, STAT_SND_C); //log sent_c
	    if (from->ifndx >= 0)
		ccnl_face_enqueue(relay, from, buf_dup(c->data));
	    else
		ccnl_app_RX(relay, c);
	    rc = 0; goto Done;
	}
	// CONFORM: Step 2:
	i = ccnl_interest_find_exact(relay, prefix, ppkd);
	DEBUGMSG(5, "  matching interest is %p\n", (void *) i);
	if (!i) { // this is a new I request: create and propagate
	    i = ccnl_interest_new(relay, from, &buf, &prefix, &ppkd);
	    if (i) { // CONFORM: Step 3 (and 4)
		DEBUGMSG(5, "  created new interest entry %p\n", (void *) i);
		ccnl_interest_propagate(relay, i);
	    }
	} else if (from->flags & CCNL_FACE_FLAGS_FWDALLI) {
	    DEBUGMSG(5, "  old interest, nevertheless propagated %p\n", (void *) i);
	    ccnl_interest_propagate(relay, i);
	}
	if (i) { // store the I request, for the incoming face (Step 3)
	    DEBUGMSG(5, "  appending interest entry %p\n", (void *) i);
	    ccnl_interest_append_pending(i, from);
	}
	rc = 0; goto Done;
    } else if (ccnl_is_content(buf)) {
	ccnl_print_stats(relay, STAT_RCV_C); //log count recv_content
	// CONFORM: Step 1:
	c = ccnl_content_find_exact(relay, prefix, ppkd);
	if (c) goto Done; // content is dup
	c = ccnl_content_new(relay, &buf, &prefix, &ppkd, content, contlen);
	if (c) { // CONFORM: Step 2 (and 3)
	    if (!ccnl_content_serve_pending(relay, c)) { // unsolicited content
		// CONFORM: "A node MUST NOT forward unsolicited data [...]"
		DEBUGMSG(6, "  removed because no matching interest\n");
		free_content(c);
		goto Done;
	    }
	    if (relay->max_cache_entries != 0) { // it's set to -1 or a limit
		DEBUGMSG(5, "  adding content to cache\n");
		ccnl_content_add2cache(relay, c);
	    } else {
		DEBUGMSG(5, "  content not added to cache\n");
		free_content(c);
	    }
	}
	rc = 0; goto Done;
    } else {
	DEBUGMSG(6, "  unknown packet type, discarded\n"); goto Done;
    }
Done:
    free_prefix(prefix);
    free_3ptr_list(buf, nonce, ppkd);
    return rc;
}

// eof
