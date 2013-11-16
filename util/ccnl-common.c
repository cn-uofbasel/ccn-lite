/*
 * @f util/ccnl-common.c
 * @b common functions for the CCN-lite utilities
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 * 2013-07-22 created
 * 2013-10-17 extended <christopher.scherb@unibas.ch>
 */

// ----------------------------------------------------------------------

#ifndef CCNL_COMMON_C
#define CCNL_COMMON_C

#include <stdlib.h>
#include <stdio.h>


#  define ccnl_malloc(s)	debug_malloc(s, __FILE__, __LINE__,timestamp())
#  define ccnl_free(p)		debug_free(p, __FILE__, __LINE__)
#  define CCNL_NOW()		current_time()

struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno, size;
    char *tstamp;
} *mem;

char*
timestamp(void)
{
    static char ts[30], *cp;

    sprintf(ts, "%.4g", CCNL_NOW());
    cp = strchr(ts, '.');
    if (!cp)
	strcat(ts, ".0000");
    else if (strlen(cp) > 5)
	cp[5] = '\0';
    else while (strlen(cp) < 5)
	strcat(cp, "0");
    return ts;
}

void
ccnl_get_timeval(struct timeval *tv)
{
    gettimeofday(tv, NULL);
}

int
current_time(void)
{
    struct timeval tv;

    ccnl_get_timeval(&tv);
    return tv.tv_sec;
}

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

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
	char *s; unsigned char *valptr; int vallen; \
	if (consume(typ, num, &buf, &buflen, &valptr, &vallen) < 0) goto Bail; \
	s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
	memcpy(s, valptr, vallen); s[vallen] = '\0'; \
	ccnl_free(VAR); \
	VAR = (unsigned char*) s; \
	continue; \
    } do {} while(0)

#define extractStr2(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
	char *s; unsigned char *valptr; int vallen; \
	if (consume(typ, num, buf, buflen, &valptr, &vallen) < 0) goto Bail; \
	s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
	memcpy(s, valptr, vallen); s[vallen] = '\0'; \
	ccnl_free(VAR); \
	VAR = (unsigned char*) s; \
	continue; \
    } do {} while(0)

#endif //CCNL_COMMON_C
// eof