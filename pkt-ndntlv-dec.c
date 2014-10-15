/*
 * @f pkt-ndntlv-dec.c
 * @b CCN lite - NDN pkt decoding routines (TLV pkt format March 2014)
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-03-05 created
 */

#ifndef PKT_NDNTLV_DEC_C
#define PKT_NDNTLV_DEC_C

#include "pkt-ndntlv.h"
#include "ccnl-core.h"


static int
ccnl_ndntlv_varlenint(unsigned char **buf, int *len, int *val)
{
    if (**buf < 253 && *len >= 1) {
	*val = **buf;
	*buf += 1;
	*len -= 1;
    } else if (**buf == 253 && *len >= 3) { // 2 bytes
	*val = ntohs(*(uint16_t*)(*buf + 1));
	*buf += 3;
	*len -= 3;
    } else if (**buf == 254 && *len >= 5) { // 4 bytes
	*val = ntohl(*(uint32_t*)(*buf + 1));
	*buf += 5;
	*len -= 5;
    } else {
	// not implemented yet (or too short)
	return -1;
    }
    return 0;
}


unsigned long int
ccnl_ndntlv_nonNegInt(unsigned char *cp, int len)
{
    unsigned long int val = 0;

    while (len-- > 0) {
	val = (val << 8) | *cp;
	cp++;
    }
    return val;
}


int
ccnl_ndntlv_dehead(unsigned char **buf, int *len,
		   int *typ, int *vallen)
{
    if (ccnl_ndntlv_varlenint(buf, len, typ))
	return -1;
    if (ccnl_ndntlv_varlenint(buf, len, vallen))
	return -1;
    return 0;
}

struct ccnl_buf_s*
ccnl_ndntlv_extract(int hdrlen,
            unsigned char **data, int *datalen,
            int *scope, int *mbf, int *min, int *max, 
            unsigned char *final_block_id,
            int *final_block_id_len,
            struct ccnl_prefix_s **prefix,
            struct ccnl_buf_s **nonce,
            struct ccnl_buf_s **ppkl,
            unsigned char **content, int *contlen)
{
    unsigned char *start = *data - hdrlen;
    int i, len, typ;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf, *n = 0, *pub = 0;
    DEBUGMSG(99, "ccnl_ndntlv_extract\n");

    if (content)
    *content = NULL;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
    return NULL;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                       sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (ccnl_ndntlv_dehead(data, datalen, &typ, &len) == 0) {
    unsigned char *cp = *data;
    int len2 = len;
    switch (typ) {
    case NDN_TLV_Name:
        while (len2 > 0) {
        if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
            goto Bail;

        if (typ == NDN_TLV_NameComponent &&
                    p->compcnt < CCNL_MAX_NAME_COMP) {
            p->comp[p->compcnt] = cp;
            p->complen[p->compcnt] = i;
            p->compcnt++;

        }  // else unknown type: skip
        cp += i;
        len2 -= i;
        }
        break;
    case NDN_TLV_Selectors:
        while (len2 > 0) {
        if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
            goto Bail;

        if (typ == NDN_TLV_MinSuffixComponents && min)
            *min = ccnl_ndntlv_nonNegInt(cp, i);
        if (typ == NDN_TLV_MinSuffixComponents && max)
            *max = ccnl_ndntlv_nonNegInt(cp, i);
        if (typ == NDN_TLV_MustBeFresh && mbf)
            *mbf = 1;
        if (typ == NDN_TLV_Exclude) {
            DEBUGMSG(49, "warning: 'exclude' field ignored\n");
        }
        cp += i;
        len2 -= i;
        }
        break;
    case NDN_TLV_Nonce:
        if (!n)
        n = ccnl_buf_new(*data, len);
        break;
    case NDN_TLV_Scope:
        if (scope)
        *scope = ccnl_ndntlv_nonNegInt(*data, len);
        break;
    case NDN_TLV_Content:
        if (content) {
            *content = *data;
            *contlen = len;
        }
        break;
    case NDN_TLV_MetaInfo:
        if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
            goto Bail;
        if (typ == NDN_TLV_ContentType)
            // Not used
            // ccnl_ndntlv_nonNegInt(cp, i);
        if (typ == NDN_TLV_FreshnessPeriod)
            // Not used
            // ccnl_ndntlv_nonNegInt(cp, i);
        if (typ == NDN_TLV_FinalBlockId) {
            if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i))
                goto Bail;
            if (typ == NDN_TLV_NameComponent && final_block_id && final_block_id_len) {
                final_block_id = cp;
                *final_block_id_len = i;
            } else if(typ == NDN_TLV_NameComponent) {
                printf("FinalBlockId is not defined");
            }
        }
        break;
    default:
        break;
    }
    *data += len;
    *datalen -= len;
    }
    if (*datalen > 0)
    goto Bail;

    if (prefix)    *prefix = p;    else free_prefix(p);
    if (nonce)     *nonce = n;     else ccnl_free(n);
    if (ppkl)      *ppkl = pub;    else ccnl_free(pub);

    buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content && *content)
    *content = buf->data + (*content - start);
    for (i = 0; i < p->compcnt; i++)
        p->comp[i] = buf->data + (p->comp[i] - start);
    return buf;
Bail:
    free_prefix(p);
    free_2ptr_list(n, pub);
    return NULL;
}

#endif
// eof
