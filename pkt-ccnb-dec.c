/*
 * @f pkt-de-ccnb.c
 * @b CCN lite - parse CCNb format
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
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
 * 2011-03-13 created (cft): orig name ccnl-parse-ccnb.c
 * 2014-03-20 extracted from ccnl-core.c
 */

#ifndef PKT_CCNB_DEC_C
#define PKT_CCNB_DEC_C

#include "pkt-ccnb.h"

// ----------------------------------------------------------------------
// ccnb parsing support

static int ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len,
                             unsigned char **valptr, int *vallen);

static int
ccnl_ccnb_dehead(unsigned char **buf, int *len, int *num, int *typ)
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
ccnl_ccnb_hunt_for_end(unsigned char **buf, int *len,
             unsigned char **valptr, int *vallen)
{
    int typ, num;

    while (ccnl_ccnb_dehead(buf, len, &num, &typ) == 0) {
        if (num==0 && typ==0)
            return 0;
        if (ccnl_ccnb_consume(typ, num, buf, len, valptr, vallen) < 0)
            return -1;
    }
    return -1;
}

static int
ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len,
                  unsigned char **valptr, int *vallen)
{
    if (typ == CCN_TT_BLOB || typ == CCN_TT_UDATA) {
        if (valptr)  *valptr = *buf;
        if (vallen)  *vallen = num;
        *buf += num, *len -= num;
        return 0;
    }
    if (typ == CCN_TT_DTAG || typ == CCN_TT_DATTR)
        return ccnl_ccnb_hunt_for_end(buf, len, valptr, vallen);
//  case CCN_TT_TAG, CCN_TT_ATTR:
//  case DTAG, DATTR:
    return -1;
}

int
ccnl_ccnb_data2uint(unsigned char *cp, int len)
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
ccnl_ccnb_extract(unsigned char **data, int *datalen,
                  int *scope, int *aok, int *min, int *max,
                  struct ccnl_prefix_s **prefix,
                  struct ccnl_buf_s **nonce,
                  struct ccnl_buf_s **ppkd,
                  unsigned char **content, int *contlen)
{
    unsigned char *start = *data - 2 /* account for outer TAG hdr */, *cp;
    int num, typ, len, oldpos;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf, *n = 0, *pub = 0;
    DEBUGMSG(99, "ccnl_ccnb_extract\n");

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) return NULL;
    p->suite = CCNL_SUITE_CCNB;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    oldpos = *data - start;
    while (ccnl_ccnb_dehead(data, datalen, &num, &typ) == 0) {
        if (num==0 && typ==0) break; // end
        if (typ == CCN_TT_DTAG) {
            if (num == CCN_DTAG_NAME) {
                p->nameptr = start + oldpos;
                for (;;) {
                    if (ccnl_ccnb_dehead(data, datalen, &num, &typ) != 0)
                        goto Bail;
                    if (num==0 && typ==0)
                        break;
                    if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                        p->compcnt < CCNL_MAX_NAME_COMP) {
                        if (ccnl_ccnb_hunt_for_end(data, datalen, p->comp + p->compcnt,
                                p->complen + p->compcnt) < 0) goto Bail;
                        p->compcnt++;
                    } else {
                        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0)
                            goto Bail;
                    }
                }
                p->namelen = *data - p->nameptr;
#ifdef USE_NFN
                if (p->compcnt > 0 && p->complen[p->compcnt-1] == 3 &&
                                    !memcmp(p->comp[p->compcnt-1], "NFN", 3)) {
                    p->nfnflags |= CCNL_PREFIX_NFN;
                    p->compcnt--;
                    if (p->compcnt > 0 && p->complen[p->compcnt-1] == 5 &&
                                   !memcmp(p->comp[p->compcnt-1], "THUNK", 5)) {
                        p->nfnflags |= CCNL_PREFIX_THUNK;
                        p->compcnt--;
                    }
                }
#endif
                oldpos = *data - start;
                continue;
            }
            if (num == CCN_DTAG_SCOPE || num == CCN_DTAG_NONCE ||
                num == CCN_DTAG_MINSUFFCOMP || num == CCN_DTAG_MAXSUFFCOMP ||
                                         num == CCN_DTAG_PUBPUBKDIGEST) {
                if (ccnl_ccnb_hunt_for_end(data, datalen, &cp, &len) < 0) goto Bail;
                if (num == CCN_DTAG_SCOPE && len == 1 && scope)
                    *scope = isdigit(*cp) && (*cp < '3') ? *cp - '0' : -1;
                if (num == CCN_DTAG_ANSWERORIGKIND && aok)
                    *aok = ccnl_ccnb_data2uint(cp, len);
                if (num == CCN_DTAG_MINSUFFCOMP && min)
                    *min = ccnl_ccnb_data2uint(cp, len);
                if (num == CCN_DTAG_MAXSUFFCOMP && max)
                    *max = ccnl_ccnb_data2uint(cp, len);
                if (num == CCN_DTAG_NONCE && !n)
                    n = ccnl_buf_new(cp, len);
                if (num == CCN_DTAG_PUBPUBKDIGEST && !pub)
                    pub = ccnl_buf_new(cp, len);
                if (num == CCN_DTAG_EXCLUDE) {
                    DEBUGMSG(49, "warning: 'exclude' field ignored\n");
                } else {
                    oldpos = *data - start;
                    continue;
                }
            }
            if (num == CCN_DTAG_CONTENT) {
                if (ccnl_ccnb_consume(typ, num, data, datalen, content, contlen) < 0)
                    goto Bail;
                oldpos = *data - start;
                continue;
            }
        }
        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0) goto Bail;
        oldpos = *data - start;
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
    if (p->nameptr)
        p->nameptr = buf->data + (p->nameptr - start);

    return buf;
Bail:
    free_prefix(p);
    free_2ptr_list(n, pub);
    return NULL;
}

int
ccnl_ccnb_unmkBinaryInt(unsigned char **data, int *datalen,
                        unsigned int *result, unsigned char *width)
{
    unsigned char *cp = *data;
    int len = *datalen, typ, num;
    unsigned int val = 0;

    if (ccnl_ccnb_dehead(&cp, &len, &num, &typ) != 0 || typ != CCN_TT_BLOB)
        return -1;
    if (width) {
      if (*width < num)
          num = *width;
      else
          *width = num;
    }

    // big endian (network order):
    while (num-- > 0 && len > 0) {
        val = (val << 8) | *cp++;
        len--;
    }
    *result = val;

    if (len < 1 || *cp != '\0') // no end-of-entry
        return -1;
    *data = cp+1;
    *datalen = len-1;
    return 0;
}

#endif
// eof
