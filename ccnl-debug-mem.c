/*
 * @f b3c__debug-mem.c
 * @b OmNet++ adaption functions for the WOWMOM paper / memory debugging
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
 * 2011-12-10 created
 */

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

// eof
