/*
 * @f pkt-ndntlv-enc.c
 * @b CCN lite - NDN pkt composing (TLV pkt format March 2014)
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

#ifndef PKT_NDNTLV_ENC_C
#define PKT_NDNTLV_ENC_C

#include "pkt-ndntlv.h"

int
ccnl_ndntlv_prependTLval(unsigned long val, int *offset, unsigned char *buf)
{
    int len, i, t;

    if (val < 253)
	len = 0, t = val;
    else if (val <= 0xffff)
	len = 2, t = 253;
    else if (val <= 0xffffffffL)
	len = 4, t = 254;
    else
	len = 8, t = 255;
    if (*offset < (len+1))
	return -1;

    for (i = 0; i < len; i++) {
	buf[--(*offset)] = val & 0xff;
	val = val >> 8;
    }
    buf[--(*offset)] = t;
    return len + 1;
}

int
ccnl_ndntlv_prependTL(int type, unsigned int len,
		      int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    if (ccnl_ndntlv_prependTLval(len, offset, buf) < 0)
	return -1;
    if (ccnl_ndntlv_prependTLval(type, offset, buf) < 0)
	return -1;
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependNonNegInt(int type, unsigned int val,
			     int *offset, unsigned char *buf)
{
    int oldoffset = *offset, len = 0, i;
    static char fill[] = {1, 0, 0, 1, 0, 3, 2, 1, 0};

    while (val) {
	if ((*offset)-- < 1)
	    return -1;
	buf[*offset] = (unsigned char) (val & 0xff);
	len++;
	val = val >> 8;
    }
    for (i = fill[len]; i > 0; i--) {
	if ((*offset)-- < 1)
	    return -1;
	buf[*offset] = 0;
	len++;
    }
    if (ccnl_ndntlv_prependTL(type, len, offset, buf) < 0)
	return -1;
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependBlob(int type, unsigned char *blob, int len,
			int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (*offset < len)
	return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_ndntlv_prependTL(type, len, offset, buf) < 0)
	return -1;
    return oldoffset - *offset;
}

// ----------------------------------------------------------------------

int
ccnl_ndntlv_mkInterest(struct ccnl_prefix_s *name, int scope, int *nonce,
		       int *offset, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, cnt;

    if (scope >= 0) {
	if (scope > 2)
	    return -1;
	if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_Scope, scope, offset, buf) < 0)
	    return -1;
    }

    {
	unsigned char lifetime[2] = { 0x0f, 0xa0 };
	unsigned char mustbefresh[2] = { 0x12, 0x00 };

	if (ccnl_ndntlv_prependBlob(NDN_TLV_InterestLifetime, lifetime, 2,
				    offset, buf) < 0)
	    return -1;

	if (nonce && ccnl_ndntlv_prependBlob(NDN_TLV_Nonce, (unsigned char*) nonce, 4,
				    offset, buf) < 0)
	    return -1;

	if (ccnl_ndntlv_prependBlob(NDN_TLV_Selectors, mustbefresh, 2,
				    offset, buf) < 0)
	    return -1;
    }

    oldoffset2 = *offset;
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
	if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
				    name->comp[cnt],
				    name->complen[cnt],
				    offset, buf) < 0)
	    return -1;
    }
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, oldoffset2 - *offset,
			      offset, buf) < 0)
	return -1;

    if (ccnl_ndntlv_prependTL(NDN_TLV_Interest, oldoffset - *offset,
			      offset, buf) < 0)
	return -1;

    return oldoffset - *offset;
}

int
ccnl_ndntlv_mkContent(struct ccnl_prefix_s *name, unsigned char *payload,
		      int paylen, int *offset, int *contentpos,
		      unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, cnt;

    if (contentpos)
	*contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
				offset, buf) < 0)
	return -1;

    oldoffset2 = *offset;
    for (cnt = 0; cnt < name->compcnt; cnt++) {
	if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
				    name->comp[cnt],
				    name->complen[cnt],
				    offset, buf) < 0)
	    return -1;
    }
    /*
    for (cnt = 0; namecomp[cnt]; cnt++);
    while (--cnt >= 0) {
	int len = strlen(namecomp[cnt]);
	if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
				    (unsigned char*) namecomp[cnt], len,
				    offset, buf) < 0)
	    return -1;
    }
    */
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, oldoffset2 - *offset,
			      offset, buf) < 0)
	return -1;

    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
			      offset, buf) < 0)
	return -1;

    if (contentpos)
	*contentpos -= *offset;

    return oldoffset - *offset;
}

#ifdef XXX
int
ccnl_ndntlv_mkContent2(struct ccnl_prefix_s *name, unsigned char *payload,
		       int paylen, int *offset, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, cnt;

    // fill in backwards
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
				offset, buf)< 0)
	return -1;

    oldoffset2 = *offset;
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
	if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
				    name->comp[cnt], name->complen[cnt],
				    offset, buf) < 0)
	    return -1;
    }
/*
    for (cnt = 0; namecomp[cnt]; cnt++);
    while (--cnt >= 0) {
	int len = strlen(namecomp[cnt]);
	if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
				    (unsigned char*) namecomp[cnt], len,
				    offset, buf) < 0)
	    return -1;
    }
*/
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, oldoffset2 - *offset,
			      offset, buf) < 0)
	return -1;

    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
			      offset, buf) < 0)
	return -1;

    return oldoffset - *offset;
}
#endif

#endif
// eof
