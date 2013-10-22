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
 */

#define CCNL_VERSION "2013-07-27"

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
    DEBUGMSG(49, "ccnl_prefix_cmp (mode=%d, nlen=%d, plen=%d, %d), name=%s: %d (%p)\n",
	     mode, nlen, p->compcnt, name->compcnt,
	     ccnl_prefix_to_path(name), rc, md);
    return rc;
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

    if (*len > 0 && **buf == 0) { // end
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
data2uint(unsigned char *cp, int len)
{
    int i, val;

    for (i = 0, val = 0; i < len; i++)
	if (isdigit(cp[i]))
	    val = 10*val + cp[i] - '0';
	else
	    return -1;
    return val;
}

struct ccnl_buf_s*
ccnl_extract_prefix_nonce_ppkd(unsigned char **data, int *datalen,
			       int *scope, int *aok, int *min, int *max,
			       struct ccnl_prefix_s **prefix,
			       struct ccnl_buf_s **nonce,
			       struct ccnl_buf_s **ppkd,
			       unsigned char **content, int *contlen)
{
    unsigned char *start = *data - 2, *cp;
    int num, typ, len;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf, *n = 0, *pub = 0;
    DEBUGMSG(99, "ccnl_extract_prefix\n");

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) return NULL;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
					   sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (dehead(data, datalen, &num, &typ) == 0) {
	if (num==0 && typ==0) break; // end
	if (typ == CCN_TT_DTAG) {
	    if (num == CCN_DTAG_NAME) {
		for (;;) {
		    if (dehead(data, datalen, &num, &typ) != 0) goto Bail;
		    if (num==0 && typ==0)
			break;
		    if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
			p->compcnt < CCNL_MAX_NAME_COMP) {
			if (hunt_for_end(data, datalen, p->comp + p->compcnt,
				p->complen + p->compcnt) < 0) goto Bail;
			p->compcnt++;
		    } else {
			if (consume(typ, num, data, datalen, 0, 0) < 0)
			    goto Bail;
		    }
		}
		continue;
	    }
	    if (num == CCN_DTAG_SCOPE || num == CCN_DTAG_NONCE ||
		num == CCN_DTAG_MINSUFFCOMP || num == CCN_DTAG_MAXSUFFCOMP ||
					 num == CCN_DTAG_PUBPUBKDIGEST) {
		if (hunt_for_end(data, datalen, &cp, &len) < 0) goto Bail;
		if (num == CCN_DTAG_SCOPE && len == 1 && scope)
		    *scope = isdigit(*cp) && (*cp < '3') ? *cp - '0' : -1;
		if (num == CCN_DTAG_ANSWERORIGKIND && aok)
		    *aok = data2uint(cp, len);
		if (num == CCN_DTAG_MINSUFFCOMP && min)
		    *min = data2uint(cp, len);
		if (num == CCN_DTAG_MAXSUFFCOMP && max)
		    *max = data2uint(cp, len);
		if (num == CCN_DTAG_NONCE && !n)
		    n = ccnl_buf_new(cp, len);
		if (num == CCN_DTAG_PUBPUBKDIGEST && !pub)
		    pub = ccnl_buf_new(cp, len);
		if (num == CCN_DTAG_EXCLUDE) {
		    DEBUGMSG(49, "warning: 'exclude' field ignored\n");
		} else
		    continue;
	    }
	    if (num == CCN_DTAG_CONTENT) {
		if (consume(typ, num, data, datalen, content, contlen) < 0)
		    goto Bail;
		continue;
	    }
	}
	if (consume(typ, num, data, datalen, 0, 0) < 0) goto Bail;
    }
    if (prefix)    *prefix = p;    else free_prefix(p);
    if (nonce)     *nonce = n;     else ccnl_free(n);
    if (ppkd)      *ppkd = pub;    else ccnl_free(pub);

    buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content)
	*content = buf->data + (*content - start);
    for (num = 0; num < p->compcnt; num++)
	    p->comp[num] = buf->data + (p->comp[num] - start);
    return buf;
Bail:
    free_prefix(p);
    free_2ptr_list(n, pub);
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
    DEBUGMSG(25, "ccnl_interface_enqueue interface=%p buf=%p (qlen=%d)\n",
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
		  struct ccnl_buf_s **pkt, struct ccnl_prefix_s **prefix,
		  int minsuffix, int maxsuffix, struct ccnl_buf_s **ppkd)
{
    struct ccnl_interest_s *i = (struct ccnl_interest_s *) ccnl_calloc(1,
					    sizeof(struct ccnl_interest_s));
    DEBUGMSG(99, "ccnl_new_interest\n");

    if (!i)
	return NULL;
    i->from = from;
    i->prefix = *prefix;        *prefix = 0;
    i->pkt  = *pkt;             *pkt = 0;
    i->ppkd = *ppkd;		*ppkd = 0;
    i->minsuffix = minsuffix;
    i->maxsuffix = maxsuffix;
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
	    (i->from->flags & CCNL_FACE_FLAGS_REFLECT))
	    ccnl_face_enqueue(ccnl, fwd->face, buf_dup(i->pkt));
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
    free_3ptr_list(i->ppkd, i->pkt, i);
    return i2;
}

// ----------------------------------------------------------------------
// handling of content messages

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix, struct ccnl_buf_s *ppkd,
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

    if ( (ppkd && !buf_equal(ppkd, c->ppkd)) ||
	 (prefix->compcnt + minsuffix) > (c->name->compcnt + 1) ||
	 (prefix->compcnt + maxsuffix) < (c->name->compcnt + 1) )
	return 0;

    md = prefix->compcnt - c->name->compcnt == 1 ? compute_ccnx_digest(c->pkt) : NULL;
    return ccnl_prefix_cmp(c->name, md, prefix, CMP_MATCH) == prefix->compcnt;
}

struct ccnl_content_s*
ccnl_content_new(struct ccnl_relay_s *ccnl, struct ccnl_buf_s **pkt,
		 struct ccnl_prefix_s **prefix, struct ccnl_buf_s **ppkd,
		 unsigned char *content, int contlen)
{
    struct ccnl_content_s *c;
    DEBUGMSG(99, "ccnl_content_new <%s>\n",
	     prefix==NULL ? NULL : ccnl_prefix_to_path(*prefix));

    c = (struct ccnl_content_s *) ccnl_calloc(1, sizeof(struct ccnl_content_s));
    if (!c) return NULL;
    c->last_used = CCNL_NOW();
    c->content = content;
    c->contentlen = contlen;
    c->pkt = *pkt;        *pkt = NULL;
    c->name = *prefix;    *prefix = NULL;
    if (ppkd) {
	c->ppkd = *ppkd;  *ppkd = NULL;
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

    for (f = ccnl->faces; f; f = f->next)
	f->flags &= ~CCNL_FACE_FLAGS_SERVED; // reply on a face only once
    for (i = ccnl->pit; i;) {
	struct ccnl_pendint_s *pi;
	if (!ccnl_i_prefixof_c(i->prefix, i->ppkd,
			       i->minsuffix, i->maxsuffix, c)) {
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
		ccnl_face_enqueue(ccnl, pi->face, buf_dup(c->pkt));
	    } else // upcall to deliver content to local client
		ccnl_app_RX(ccnl, c);
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
	    DEBUGMSG(7, " retransmit %d <%s>\n", i->retries, ccnl_prefix_to_path(i->prefix));
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
// the core logic of CCN:

int
ccnl_core_RX_i_or_c(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		    unsigned char **data, int *datalen)
{
    int rc= -1, scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    struct ccnl_interest_s *i = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0;
    DEBUGMSG(99, "ccnl_core_RX_i_or_c (%d bytes left)\n", *datalen);

    buf = ccnl_extract_prefix_nonce_ppkd(data, datalen, &scope, &aok, &minsfx,
			 &maxsfx, &p, &nonce, &ppkd, &content, &contlen);
    if (!buf) {
	    DEBUGMSG(6, "  parsing error or no prefix\n"); goto Done;
    }
    if (nonce && ccnl_nonce_find_or_append(relay, nonce)) {
	DEBUGMSG(6, "  dropped because of duplicate nonce\n"); goto Skip;
    }
    if (buf->data[0] == 0x01 && buf->data[1] == 0xd2) { // interest
	DEBUGMSG(6, "  interest=<%s>\n", ccnl_prefix_to_path(p));
	ccnl_print_stats(relay, STAT_RCV_I); //log count recv_interest
	if (p->compcnt > 0 && p->comp[0][0] == (unsigned char) 0xc1) goto Skip;
	if (p->compcnt == 4 && !memcmp(p->comp[0], "ccnx", 4)) {
	    rc = ccnl_mgmt(relay, buf, p, from); goto Done;
	}
	// CONFORM: Step 1:
	if ( aok & 0x01 ) { // honor "answer-from-existing-content-store" flag
	    for (c = relay->contents; c; c = c->next) {
		if (!ccnl_i_prefixof_c(p, ppkd, minsfx, maxsfx, c)) continue;
		// FIXME: should check stale bit in aok here
		DEBUGMSG(7, "  matching content for interest, content %p\n", (void *) c);
		ccnl_print_stats(relay, STAT_SND_C); //log sent_c
		if (from->ifndx >= 0)
		    ccnl_face_enqueue(relay, from, buf_dup(c->pkt));
		else
		    ccnl_app_RX(relay, c);
		goto Skip;
	    }
	}
	// CONFORM: Step 2: check whether interest is already known
	for (i = relay->pit; i; i = i->next) {
	    if (!ccnl_prefix_cmp(i->prefix, NULL, p, CMP_EXACT) &&
		i->minsuffix == minsfx && i->maxsuffix == maxsfx && 
		((!ppkd && !i->ppkd) || buf_equal(ppkd, i->ppkd)) )
		break;
	}
	if (!i) { // this is a new/unknown I request: create and propagate
	    i = ccnl_interest_new(relay, from, &buf, &p, minsfx, maxsfx, &ppkd);
	    if (i) { // CONFORM: Step 3 (and 4)
		DEBUGMSG(7, "  created new interest entry %p\n", (void *) i);
		if (scope > 2)
		    ccnl_interest_propagate(relay, i);
	    }
	} else if (scope > 2 && (from->flags & CCNL_FACE_FLAGS_FWDALLI)) {
	    DEBUGMSG(7, "  old interest, nevertheless propagated %p\n", (void *) i);
	    ccnl_interest_propagate(relay, i);
	}
	if (i) { // store the I request, for the incoming face (Step 3)
	    DEBUGMSG(7, "  appending interest entry %p\n", (void *) i);
	    ccnl_interest_append_pending(i, from);
	}
    } else { // content
	DEBUGMSG(6, "  content=<%s>\n", ccnl_prefix_to_path(p));
	ccnl_print_stats(relay, STAT_RCV_C); //log count recv_content
        
#ifdef CCNL_USE_MGMT_SIGNATUES
        if (p->compcnt == 3 && !memcmp(p->comp[0], "ccnx", 4)
                /*&& !memcmp(p->comp[1], "crypto", 6)*/) {
	    rc = ccnl_crypto(relay, buf, p, from); goto Done;
	}
#endif /*CCNL_USE_MGMT_SIGNATUES*/
        
        // CONFORM: Step 1:
	for (c = relay->contents; c; c = c->next)
	    if (buf_equal(c->pkt, buf)) goto Skip; // content is dup
	c = ccnl_content_new(relay, &buf, &p, &ppkd, content, contlen);
	if (c) { // CONFORM: Step 2 (and 3)
	    if (!ccnl_content_serve_pending(relay, c)) { // unsolicited content
		// CONFORM: "A node MUST NOT forward unsolicited data [...]"
		DEBUGMSG(7, "  removed because no matching interest\n");
		free_content(c);
		goto Skip;
	    }
	    if (relay->max_cache_entries != 0) { // it's set to -1 or a limit
		DEBUGMSG(7, "  adding content to cache\n");
		ccnl_content_add2cache(relay, c);
	    } else {
		DEBUGMSG(7, "  content not added to cache\n");
		free_content(c);
	    }
	}
    }
Skip:
    rc = 0;
Done:
    free_prefix(p);
    free_3ptr_list(buf, nonce, ppkd);
    return rc;
}

int
ccnl_core_RX_datagram(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		      unsigned char **data, int *datalen)
{
    int rc = 0, num, typ;
    DEBUGMSG(6, "ccnl_core_RX_datagram: %d bytes from face=%p (id=%d.%d)\n",
	     *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc >= 0 && *datalen > 0) {
	if (dehead(data, datalen, &num, &typ) || typ != CCN_TT_DTAG) return -1;
	switch (num) {
	case CCN_DTAG_INTEREST:
	case CCN_DTAG_CONTENTOBJ:
	    rc = ccnl_core_RX_i_or_c(relay, from, data, datalen);
	    continue;
#ifdef USE_FRAG
	case CCNL_DTAG_FRAGMENT2012:
	    rc = ccnl_frag_RX_frag2012(ccnl_core_RX_datagram, relay, from, data, datalen);
	    continue;
	case CCNL_DTAG_FRAGMENT2013:
	    rc = ccnl_frag_RX_CCNx2013(ccnl_core_RX_datagram, relay, from, data, datalen);
	    continue;
#endif
	default:
	    DEBUGMSG(15, "  unknown datagram type %d\n", num);
	    return -1;
	}
    }
    return rc;
}

void
ccnl_core_RX(struct ccnl_relay_s *relay, int ifndx, unsigned char *data,
	     int datalen, struct sockaddr *sa, int addrlen)
{
    struct ccnl_face_s *from;
    DEBUGMSG(14, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

    from = ccnl_get_face_or_create(relay, ifndx, sa, addrlen);
    if (!from) return;
    ccnl_core_RX_datagram(relay, from, &data, &datalen);
}
// eof
