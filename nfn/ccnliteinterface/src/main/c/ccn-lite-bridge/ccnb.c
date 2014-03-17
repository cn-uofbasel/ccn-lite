/*
 * @f util/ccn-lite-mkI.c
 * @b CLI mkInterest, write to Stdout
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
 * 2013-07-06  created
 */
 #define USE_SIGNATURES

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h> // htonl()
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../../../../../ccnx.h"
#include "../../../../../../ccnl.h"

#include "ccnl-crypto.c"


// ----------------------------------------------------------------------

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
hex2int(char c)
{
    if (c >= '0' && c <= '9')
    return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
    return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(unsigned char *comp) // inplace, returns len after shrinking
{
    unsigned char *in = comp, *out = comp;
    int len;

    for (len = 0; *in; len++) {
    if (in[0] != '%' || !in[1] || !in[2]) {
        *out++ = *in++;
        continue;
    }
    *out++ = hex2int(in[1])*16 + hex2int(in[2]);
    in += 3;
    }
    return len;
}

int
mkContent(char **namecomp,
      unsigned char *publisher,
      int plen,
      char *private_key_path,
      unsigned char *body, int blen,
      unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

    // add signature
// #ifdef USE_SIGNATURES
//     if(private_key_path)
//         len += add_signature(out+len, private_key_path, body, blen);
// #endif
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

    if (publisher) {
        struct timeval t;
        unsigned char tstamp[6];
        uint32_t *sec;
        uint16_t *secfrac;

        gettimeofday(&t, NULL);
        sec = (uint32_t*)(tstamp + 0); // big endian
        *sec = htonl(t.tv_sec);
        secfrac = (uint16_t*)(tstamp + 4);
        *secfrac = htons(4048L * t.tv_usec / 1000000);
        len += mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
        len += mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
        memcpy(out+len, tstamp, sizeof(tstamp));
        len += sizeof(tstamp);
        out[len++] = 0; // end-of-timestamp

        len += mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
        len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
        len += mkHeader(out+len, plen, CCN_TT_BLOB);
        memcpy(out+len, publisher, plen);
        len += plen;
        out[len++] = 0; // end-of-publisher
        out[len++] = 0; // end-of-signedinfo
    }

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}


int
mkInterest(char **namecomp,
       char *minSuffix, char *maxSuffix,
       unsigned char *digest, int dlen,
       unsigned char *publisher, int plen,
       char *scope,
       uint32_t *nonce,
       unsigned char *out)
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
    if (digest) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    len += mkHeader(out+len, dlen, CCN_TT_BLOB);
    memcpy(out+len, digest, dlen);
    len += dlen;
    out[len++] = 0; // end-of-component
    if (!maxSuffix)
        maxSuffix = "0";
    }
    out[len++] = 0; // end-of-name

    if (minSuffix) {
    k = strlen(minSuffix);
    len += mkHeader(out+len, CCN_DTAG_MINSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, minSuffix, k);
    len += k;
    out[len++] = 0; // end-of-minsuffcomp
    }

    if (maxSuffix) {
    k = strlen(maxSuffix);
    len += mkHeader(out+len, CCN_DTAG_MAXSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, maxSuffix, k);
    len += k;
    out[len++] = 0; // end-of-maxsuffcomp
    }

    if (publisher) {
    len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
    len += mkHeader(out+len, plen, CCN_TT_BLOB);
    memcpy(out+len, publisher, plen);
    len += plen;
    out[len++] = 0; // end-of-component
    }

    if (scope) {
    k = strlen(scope);
    len += mkHeader(out+len, CCN_DTAG_SCOPE, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, (unsigned char*)scope, k);
    len += k;
    out[len++] = 0; // end-of-maxsuffcomp
    }

    if (nonce) {
    len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
    len += mkHeader(out+len, sizeof(*nonce), CCN_TT_BLOB);
    memcpy(out+len, (void*)nonce, sizeof(*nonce));
    len += sizeof(*nonce);
    out[len++] = 0; // end-of-nonce
    }

    out[len++] = 0; // end-of-interest

    return len;
}

