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

int
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


#ifdef XXX

int
mkHeader(unsigned char *buf, unsigned int num, unsigned int tt)
{
    unsigned char tmp[100];
    int len = 0, i;

    *tmp = 0x80 | ((num & 0x0f) << 3) | tt;
    len = 1;
    num = num >> 4;

    while (num > 0) {
	tmp[len++] = num & 0x7f;
	num = num >> 7;
    }
    for (i = len-1; i >= 0; i--)
	*buf++ = tmp[i];
    return len;
}

int
addBlob(unsigned char *out, char *cp, int cnt)
{
    int len;

    len = mkHeader(out, cnt, CCN_TT_BLOB);
    memcpy(out+len, cp, cnt);
    len += cnt;

    return len;
}


int
mkBlob(unsigned char *out, unsigned int num, unsigned int tt,
       char *cp, int cnt)
{
    int len;

    len = mkHeader(out, num, tt);
    len += addBlob(out+len, cp, cnt);
    out[len++] = 0;

    return len;
}


int
mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt,
	  char *str)
{
    return mkBlob(out, num, tt, str, strlen(str));
}


int
mkInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = unescape_component((unsigned char*) *namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    len += mkHeader(out+len, CCN_DTAG_MAXSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, 1, CCN_TT_UDATA);
    out[len++] = '1';
    out[len++] = 0; // end-of-maxsuffcomp

    if (nonce) {
	len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(unsigned int));
	len += sizeof(unsigned int);
	out[len++] = 0; // end-of-nonce
    }

    out[len++] = 0; // end-of-interest

    return len;
}
#endif

// eof
