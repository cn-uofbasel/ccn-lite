/*
 * @f pkt-ccntlv-enc.c
 * @b CCN lite - CCNx pkt composing routines (TLV pkt format Nov 2013)
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

#ifndef PKT_CCNTLV_ENC_C
#define PKT_CCNTLV_ENC_C

#include "pkt-ccntlv.h"

int
ccnl_ccntlv_prependTL(unsigned int type, unsigned short len, int *offset, unsigned char *buf)
{
/*
    if (*offset < 4)
	return -1;
*/
    unsigned short *ip = (unsigned short*) (buf + *offset - 2);
    *ip-- = htons(len);
    *ip = htons(type);
    *offset -= 4;
    return 4;
}

int
ccnl_ccntlv_prependBlob(unsigned short type, unsigned char *blob,
			unsigned short len, int *offset, unsigned char *buf)
{
    if (*offset < (len + 4))
	return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_ccntlv_prependTL(type, len, offset, buf) < 0)
	return -1;
    return len + 4;
}

int
ccnl_ccntlv_prependFixedHdr(unsigned char ver, unsigned char msgtype,
			    unsigned short msglen, unsigned short hdrlen,
			    int *offset, unsigned char *buf)
{
    struct ccnx_tlvhdr_ccnx201311_s *hp;

    if (*offset < 8)
	return -1;
    *offset -= 8;
    hp = (struct ccnx_tlvhdr_ccnx201311_s *)(buf + *offset);
    hp->version = ver;
    hp->msgtype = msgtype;
    hp->msglen = htons(msglen);
    hp->hdrlen = htons(hdrlen);
    hp->reserved = 0;

    return 8 + hdrlen + msglen;
}

int
ccnl_ccntlv_mkInterest(char **namecomp, int scope,
		       int *offset, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, cnt;

    if (scope >= 0) {
	if (scope > 2)
	    return -1;
	buf[--(*offset)] = scope;
	if (ccnl_ccntlv_prependTL(CCNX_TLV_I_Scope, 1, offset, buf) < 0)
	    return -1;
    }

    for (cnt = 0; namecomp[cnt]; cnt++);
    oldoffset2 = *offset;
    while (--cnt >= 0) {
	int len = strlen(namecomp[cnt]);
	if (ccnl_ccntlv_prependBlob(CCNX_TLV_N_UTF8,
				 (unsigned char*) namecomp[cnt], len,
				 offset, buf) < 0)
	    return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_G_Name, oldoffset2 - *offset,
			      offset, buf) < 0)
	return -1;

    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Interest, oldoffset - *offset,
			      offset, buf) < 0)
	return -1;

    return oldoffset - *offset;
}

int
ccnl_ccntlv_mkInterestWithHdr(char **namecomp, int scope,
			      int *offset, unsigned char *buf)
{
    int len;

    len = ccnl_ccntlv_mkInterest(namecomp, scope, offset, buf);
    if (len > 0) {
	len = ccnl_ccntlv_prependFixedHdr(0, CCNX_TLV_TL_Interest, len, 0,
								offset, buf);
    }

    return len;
}

int
// ccnl_ccntlv_mkContent(char **namecomp, unsigned char *payload,
ccnl_ccntlv_mkContent(struct ccnl_prefix_s *name, unsigned char *payload,
		      int paylen, int *offset, int *contentpos, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, cnt;

    if (contentpos)
	*contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_ccntlv_prependBlob(CCNX_TLV_C_Contents, payload, paylen,
							offset, buf) < 0)
	return -1;

    oldoffset2 = *offset;
    for (cnt = 0; cnt < name->compcnt; cnt++) {
	if (ccnl_ccntlv_prependBlob(CCNX_TLV_N_UTF8,
				    name->comp[cnt],
				    name->complen[cnt],
				    offset, buf) < 0)
	    return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_G_Name, oldoffset2 - *offset,
			      offset, buf) < 0)
	return -1;

    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Object, oldoffset - *offset,
			      offset, buf) < 0)
	return -1;

    if (contentpos)
	*contentpos -= *offset;

    return oldoffset - *offset;
}

int
ccnl_ccntlv_mkContentWithHdr(struct ccnl_prefix_s *name, unsigned char *payload,
			     int paylen, int *offset, int *contentpos,
			     unsigned char *buf)
{
    int len;

    len = ccnl_ccntlv_mkContent(name, payload, paylen, offset, contentpos, buf);
    if (len > 0) {
	len = ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V0, CCNX_TLV_TL_Object,
					  len, 0, offset, buf);
    }

    return len;
}

#endif
// eof
