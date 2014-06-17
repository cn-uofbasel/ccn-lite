/*
 * @f pkt-en-ccnb.c
 * @b CCN lite - create and manipulate CCNb protocol data units
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
 * 2013-04-04 modified (ms): #if defined(CCNL_SIMULATION) || defined(CCNL_OMNET)
 * 2014-03-17 renamed to prepare for a world with many wire formats
 */

#if defined(USE_SUITE_CCNB) && !defined(PKT_CCNB_ENC_C)
#define PKT_CCNB_ENC_C

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
mkBinaryInt(unsigned char *out, unsigned int num, unsigned int tt,
	    unsigned int val, int bytes)
{
    int len = mkHeader(out, num, tt);

    if (!bytes) {
	for (bytes = sizeof(val) - 1; bytes > 0; bytes--)
	    if (val >> (8*bytes))
		break;
	bytes++;
    }
    len += mkHeader(out+len, bytes, CCN_TT_BLOB);

    while (bytes > 0) { // big endian
	bytes--;
	out[len++] = 0x0ff & (val >> (8*bytes));
    }

    out[len++] = 0; // end-of-entry
    return len;
}


// ----------------------------------------------------------------------
// (ms): Brought here the following two. I noticed also that some
// of them are replicated elsewhere in the util/ dir. Should we put them
// in one place only ?

int
mkInterest(char **namecomp, unsigned int *nonce, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = strlen(*namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name
    if (nonce) {
	len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
	memcpy(out+len, (void*)nonce, sizeof(unsigned int));
	len += sizeof(unsigned int);
    }
    out[len++] = 0; // end-of-interest

    return len;
}

#if defined(CCNL_SIMULATION) || defined(CCNL_OMNET) || defined(CCNL_NFN) || defined(CCNL_NACK)

static int
mkContent(char **namecomp, char *data, int datalen, unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // content
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    while (*namecomp) {
        len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
        k = strlen(*namecomp);
        len += mkHeader(out+len, k, CCN_TT_BLOB);
        memcpy(out+len, *namecomp++, k);
        len += k;
        out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG); // content obj
    len += mkHeader(out+len, datalen, CCN_TT_BLOB);
    memcpy(out+len, data, datalen);
    len += datalen;
    out[len++] = 0; // end-of-content obj

    out[len++] = 0; // end-of-content

    return len;
}

#endif // CCNL_SIMULATION || CCNL_OMNET

#endif /*CCNL_EN_CCNB*/
// eof
