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
 */

// ----------------------------------------------------------------------

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

// eof
