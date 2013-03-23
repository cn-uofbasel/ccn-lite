/*
 * @f ccnl__debug.c
 * @b basel bare bones ccn relay, data structure dumping routines
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
 */


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

static int debug_level;

// ----------------------------------------------------------------------

#ifndef CCNL_KERNEL

#endif // !CCNL_KERNEL


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


// eof
