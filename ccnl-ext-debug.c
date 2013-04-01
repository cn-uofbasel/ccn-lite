/*
 * @f ccnl-ext-debug.c
 * @b CCNL debugging support, dumping routines, memory tracking, stats
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 * 2011-04-19 created
 * 2013-03-18 updated (ms): removed omnet related code
 * 2013-03-31 merged with ccnl-debug.h and ccnl-debug-mem.c
 */

#ifdef USE_DEBUG

#ifndef CCNL_KERNEL
struct ccnl_stats_s {
    void* log_handler;
    FILE *ofp;
    int log_sent;
    int log_recv;
    int log_recv_old;
    int log_sent_old;
    int log_need_rt_seqn;
    int log_content_delivery_rate_per_s;
    double log_start_t;
    double log_cdr_t;
    //
    FILE *ofp_r, *ofp_s, *ofp_q;
    double log_recv_t_i;
    double log_recv_t_c;
    double log_sent_t_i;
    double log_sent_t_c;
};
#endif

#define DEBUGSTMT(LVL, ...) do { \
	if ((LVL)>debug_level) break; \
	__VA_ARGS__; \
} while (0)

#ifndef CCNL_KERNEL
#  define DEBUGMSG(LVL, ...) do {	\
	if ((LVL)>debug_level) break;   \
	fprintf(stderr, "%s: ", timestamp());	\
	fprintf(stderr, __VA_ARGS__);	\
    } while (0)

#else // CCNL_KERNEL
#  define DEBUGMSG(LVL, ...) do {	\
	if ((LVL)>debug_level) break;   \
	printk("%s: ", THIS_MODULE->name);	\
	printk(__VA_ARGS__);		\
    } while (0)
#  define printf(...)	printk(__VA_ARGS__)
#endif // CCNL_KERNEL


// ----------------------------------------------------------------------

char*
eth2ascii(unsigned char *eth)
{
    static char buf[30];

    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char) eth[0], (unsigned char) eth[1],
            (unsigned char) eth[2], (unsigned char) eth[3],
            (unsigned char) eth[4], (unsigned char) eth[5]);
    return buf;
}

char*
ccnl_addr2ascii(sockunion *su)
{
    static char result[130];

    switch (su->sa.sa_family) {
#ifdef USE_ETHERNET
	case AF_PACKET:
	{
	     struct sockaddr_ll *ll = &su->eth;
	     strcpy(result, eth2ascii(ll->sll_addr));
	     sprintf(result+strlen(result), "/0x%04x",
		     ntohs(ll->sll_protocol));
	}
	    return result;
#endif
	case AF_INET:
	    sprintf(result, "%s/%d", inet_ntoa(su->ip4.sin_addr),
		    ntohs(su->ip4.sin_port));
	    return result;
//	    return inet_ntoa(SA_CAST_IN(sa)->sin_addr);
	case AF_UNIX:
	    strcpy(result, su->ux.sun_path);
	    return result;
	default:
	    break;
    }
    return NULL;
}

static char*
ccnl_prefix_to_path(struct ccnl_prefix_s *pr)
{
//    static char prefix_buf[1024];
    static char prefix_buf[4096];
    int len, i, j;

    if (!pr)
	return NULL;
    for (len = 0, i = 0; i < pr->compcnt; i++) {
	if ((len + 1 + 3*pr->complen[i]) >= sizeof(prefix_buf))
	  return (char*) "(...prefix...)";
	prefix_buf[len++] = '/';
	for (j = 0; j < pr->complen[i]; j++) {
	    unsigned char c = pr->comp[i][j];
	    len += sprintf(prefix_buf+len,
		    !isprint(c) || isspace(c) || c=='/' ? "%%%02x" : "%c",
		    c);
	}
    }
    prefix_buf[len] = '\0';
    return prefix_buf;
}

// ----------------------------------------------------------------------

static void
blob(unsigned char *cp, int len)
{
    int i;
    for (i = 0; i < len; i++, cp++)
	printf("%02x", *cp);
}


void
encaps(int e)
{
    if (e == CCNL_ENCAPS_NONE)
	return;
    if (e == CCNL_ENCAPS_SEQUENCED2012)
	printf(" encaps=sequenced2012");
    else
	printf(" encaps=%d", e);
}

enum { // numbers for each data type
    CCNL_BUF = 1,
    CCNL_PREFIX,
    CCNL_RELAY,
    CCNL_FACE,
    CCNL_FWD,
    CCNL_INTEREST,
    CCNL_PENDINT,
    CCNL_CONTENT
};

void
ccnl_dump(int lev, int typ, void *p)
{
    struct ccnl_buf_s      *buf = (struct ccnl_buf_s      *) p;
    struct ccnl_prefix_s   *pre = (struct ccnl_prefix_s   *) p;
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_face_s     *fac = (struct ccnl_face_s     *) p;
    struct ccnl_forward_s  *fwd = (struct ccnl_forward_s  *) p;
    struct ccnl_interest_s *itr = (struct ccnl_interest_s *) p;
    struct ccnl_pendint_s  *pir = (struct ccnl_pendint_s  *) p;
    struct ccnl_content_s  *con = (struct ccnl_content_s  *) p;
    int i, k;

#define INDENT(lev) for (i = 0; i < lev; i++) printf("  ")
    switch(typ) {
    case CCNL_BUF:
	while (buf) {
	    INDENT(lev);
	    printf("%p BUF len=%d next=%p\n", (void *) buf, buf->datalen,
		(void *) buf->next);
	    buf = buf->next;
	}
	break;
    case CCNL_PREFIX:
	INDENT(lev);
	printf("%p PREFIX len=%d val=%s\n",
	       (void *) pre, pre->compcnt, ccnl_prefix_to_path(pre));
	break;
    case CCNL_RELAY:
	INDENT(lev);
	printf("%p RELAY\n", (void *) top); lev++;
	INDENT(lev); printf("interfaces:\n");
	for (k = 0; k < top->ifcount; k++) {
	    INDENT(lev+1);
#ifdef CCNL_KERNEL
	    printf("ifndx=%d sockstruct=%p", k, top->ifs[k].sock);
#else
	    printf("ifndx=%d sock=%d", k, top->ifs[k].sock);
#endif
	    printf(" addr=%s", ccnl_addr2ascii(&top->ifs[k].addr));
//	    printf(" facecnt=%d", top->ifs[k].facecnt);
	    if (top->ifs[k].reflect)
		printf(" reflect=%d", top->ifs[k].reflect);
	    printf("\n");
	}
	if (top->faces) {
	    INDENT(lev); printf("faces:\n");    ccnl_dump(lev+1, CCNL_FACE, top->faces);
	}
	if (top->fib) {
	    INDENT(lev); printf("fib:\n");      ccnl_dump(lev+1, CCNL_FWD, top->fib);
	}
	if (top->pit) {
	    INDENT(lev); printf("pit:\n");      ccnl_dump(lev+1, CCNL_INTEREST, top->pit);
	}
	if (top->contents) {
	    INDENT(lev); printf("contents:\n"); ccnl_dump(lev+1, CCNL_CONTENT, top->contents);
	}
	break;
    case CCNL_FACE:
	while (fac) {
	    INDENT(lev);
	    printf("%p FACE id=%d next=%p prev=%p ifndx=%d flags=%02x",
		   (void*) fac, fac->faceid, (void *) fac->next,
		   (void *) fac->prev, fac->ifndx, fac->flags);
	    if (fac->peer.sa.sa_family == AF_INET)
		printf(" ip=%s", ccnl_addr2ascii(&fac->peer));
	    else if (fac->peer.sa.sa_family == AF_PACKET)
		printf(" eth=%s", ccnl_addr2ascii(&fac->peer));
	    else if (fac->peer.sa.sa_family == AF_UNIX)
		printf(" ux=%s", ccnl_addr2ascii(&fac->peer));
	    else
		printf(" peer=?");
	    if (fac->encaps)
		encaps(fac->encaps->protocol);
	    printf("\n");
	    if (fac->outq) {
		INDENT(lev+1); printf("outq:\n");
		ccnl_dump(lev+2, CCNL_BUF, fac->outq);
	    }
	    fac = fac->next;
	}
	break;
    case CCNL_FWD:
	while (fwd) {
	    INDENT(lev);
	    printf("%p FWD next=%p face=%p\n", (void *) fwd, (void *) fwd->next,
		(void *) fwd->face);
	    ccnl_dump(lev+1, CCNL_PREFIX, fwd->prefix);
	    fwd = fwd->next;
	}
	break;
    case CCNL_INTEREST:
	while (itr) {
	    INDENT(lev);
	    printf("%p INTEREST next=%p prev=%p last=%d retries=%d\n",
		   (void *) itr, (void *) itr->next, (void *) itr->prev,
		   itr->last_used, itr->retries);
	    ccnl_dump(lev+1, CCNL_BUF, itr->data);
	    ccnl_dump(lev+1, CCNL_PREFIX, itr->prefix);
	    if (itr->nonce) {
		INDENT(lev+1);
		printf("%p NONCE=", (void *) itr->nonce);
		blob(itr->nonce->data, itr->nonce->datalen);
		printf("\n");
	    }
	    if (itr->ppkd) {
		INDENT(lev+1);
		printf("%p PUBLISHER=", (void *) itr->ppkd);
		blob(itr->ppkd->data, itr->ppkd->datalen);
		printf("\n");
	    }
	    if (itr->pending) {
		INDENT(lev+1); printf("pending:\n");
		ccnl_dump(lev+2, CCNL_PENDINT, itr->pending);
	    }
	    itr = itr->next;

	}
	break;
    case CCNL_PENDINT:
	while (pir) {
	    INDENT(lev);
	    printf("%p PENDINT next=%p face=%p last=%d\n",
		   (void *) pir, (void *) pir->next,
		   (void *) pir->face, pir->last_used);
	    pir = pir->next;
	}
	break;
    case CCNL_CONTENT:
	while (con) {
	    INDENT(lev);
	    printf("%p CONTENT  next=%p prev=%p last_used=%d served_cnt=%d\n",
		   (void *) con, (void *) con->next, (void *) con->prev,
		   con->last_used, con->served_cnt);
	    ccnl_dump(lev+1, CCNL_PREFIX, con->prefix);
	    ccnl_dump(lev+1, CCNL_BUF, con->data);
	    con = con->next;
	}
	break;
    default:
	INDENT(lev);
	printf("unknown data type %d at %p\n", typ, p);
    }
}

#else // !USE_DEBUG
#  define DEBUGSTMT(LVL, ...)			do {} while(0)
#  define DEBUGMSG(LVL, ...)			do {} while(0)
#endif // !USE_DEBUG

// -----------------------------------------------------------------

#ifdef USE_DEBUG_MALLOC

#  define ccnl_malloc(s)	debug_malloc(s, __FILE__, __LINE__,timestamp())
#  define ccnl_calloc(n,s)	debug_calloc(n, s, __FILE__, __LINE__,timestamp())
#  define ccnl_realloc(p,s)	debug_realloc(p, s, __FILE__, __LINE__)
#  define ccnl_free(p)		debug_free(p, __FILE__, __LINE__)
#  define ccnl_buf_new(p,s)	debug_buf_new(p, s, __FILE__, __LINE__,timestamp())

char* timestamp(void);

struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno, size;
    char *tstamp;
} *mem;

void*
debug_malloc(int s, const char *fn, int lno, char *tstamp)
{
    struct mhdr *h = (struct mhdr *) malloc(s + sizeof(struct mhdr));
    if (!h) return NULL;
    h->next = mem;
    mem = h;
    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
    h->tstamp = strdup(tstamp);
    return ((unsigned char *)h) + sizeof(struct mhdr);
}

void*
debug_calloc(int n, int s, const char *fn, int lno, char *tstamp)
{
    void *p = debug_malloc(n * s, fn, lno, tstamp);
    if (p)
	memset(p, 0, n*s);
    return p;
}


int
debug_unlink(struct mhdr *hdr)
{
    struct mhdr **pp = &mem;

    for (pp = &mem; pp; pp = &((*pp)->next)) {
	if (*pp == hdr) {
	    *pp = hdr->next;
	    return 0;
	}
	if (!(*pp)->next)
	    break;
    }
    return 1;
}

/*
void*
debug_realloc(void *p, int s, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (p) {
	if (debug_unlink(h)) {
	    fprintf(stderr,
		    "%s: @@@ memerror - realloc(%s:%d) at %s:%d does not find memory block\n",
		    timestamp(), h->fname, h->lineno, fn, lno);
	    return NULL;
	}
	h = (struct mhdr *) realloc(h, s+sizeof(struct mhdr));
	if (!h)
	    return NULL;
    } else
      h = (struct mhdr *) malloc(s+sizeof(struct mhdr));
    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
    h->next = mem;
    mem = h;
    return ((unsigned char *)h) + sizeof(struct mhdr);
}
*/

void
debug_free(void *p, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (!p) {
//	fprintf(stderr, "%s: @@@ memerror - free() of NULL ptr at %s:%d\n",
//	   timestamp(), fn, lno);
	return;
    }
    if (debug_unlink(h)) {
	fprintf(stderr,
	   "%s: @@@ memerror - free() at %s:%d does not find memory block %p\n",
		timestamp(), fn, lno, p);
	return;
    }
    if (h->tstamp)
	free(h->tstamp);
    free(h);
}

struct ccnl_buf_s*
debug_buf_new(void *data, int len, const char *fn, int lno, char *tstamp)
{
  struct ccnl_buf_s *b = (struct ccnl_buf_s *) debug_malloc(sizeof(*b) + len, fn,
							    lno, tstamp);

    if (!b)
	return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
	memcpy(b->data, data, len);
    return b;
}

void
debug_memdump()
{
    struct mhdr *h;

    fprintf(stderr, "%s: @@@ memory dump starts\n", timestamp());
    for (h = mem; h; h = h->next) {
	fprintf(stderr, "%s: @@@ mem %p %5d Bytes, allocated by %s:%d @%s\n",
		timestamp(),
		(unsigned char *)h + sizeof(struct mhdr),
		h->size, h->fname, h->lineno, h->tstamp);
    }
    fprintf(stderr, "%s: @@@ memory dump ends\n", timestamp());
}


#else // !USE_DEBUG_MALLOC

# ifndef CCNL_KERNEL
#  define ccnl_malloc(s)	malloc(s)
#  define ccnl_calloc(n,s) 	calloc(n,s)
#  define ccnl_realloc(p,s)	realloc(p,s)
#  define ccnl_free(p)		free(p)
# endif

#endif // !USE_DEBUG_MALLOC


#define free_2ptr_list(a,b)	ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)	ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d)	ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);

#define free_prefix(p)	do{ if(p) \
			free_4ptr_list(p->path,p->comp,p->complen,p); } while(0)
#define free_content(c) do{ free_prefix(c->prefix); \
			free_2ptr_list(c->data, c); } while(0)

// -----------------------------------------------------------------
static int debug_level;


// eof
