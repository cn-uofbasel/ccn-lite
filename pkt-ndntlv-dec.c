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

#include "pkt-ndntlv.h"


int
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

    while (len-- > 0)
	val = (val << 8) | *cp;
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



// eof
