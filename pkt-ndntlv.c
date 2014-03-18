/*
 * @f pkt-ndntlv.c
 * @b CCN lite - NDN pkt composing/parsing routines (TLV pkt format March 2014)
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

#include "pkt-ndntlv.h"

// composing

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
    return *offset - oldoffset;
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


// parsing

int
ccnl_ndntlv_varlenint(unsigned char **buf, unsigned int *len, unsigned int *val)
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

int
ccnl_ndntlv_dehead(int lev, unsigned char *base, unsigned char **buf,
		   unsigned int *len, unsigned int *typ, unsigned int *vallen)
{
    if (ccnl_ndntlv_varlenint(buf, len, typ))
	return -1;
    if (ccnl_ndntlv_varlenint(buf, len, vallen))
	return -1;
    return 0;
}



// eof
