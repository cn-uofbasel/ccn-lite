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
 * 2013-10-17 extended <christopher.scherb@unibas.ch>
 */

#ifndef CCNL_COMMON_C
#define CCNL_COMMON_C

#define CCNL_UNIX
#define _BSD_SOURCE
#define _SVID_SOURCE

#if defined(USE_FRAG) || defined(USE_MGMT) || defined(USE_NFN) || defined(USE_SIGNATURES)
# define NEEDS_PACKET_CRAFTING
#endif

// ----------------------------------------------------------------------

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>


#include "base64.c"

#include "../ccnl-defs.h"
#include "../ccnl-core.h"
#include "../ccnl-ext-debug.c"

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)
#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)

struct ccnl_prefix_s* ccnl_prefix_new(int suite, int cnt);
int ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#include "../ccnl-pkt-ccnb.c"
#include "../ccnl-pkt-ccntlv.c"
#include "../ccnl-pkt-ndntlv.c"
#include "../ccnl-pkt-localrpc.h"

#define ccnl_core_addToCleanup(b)       do{}while(0)

// include only the utils, not the core routines:
#include "../ccnl-core-util.c"

// ----------------------------------------------------------------------

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; int vallen; \
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, &valptr, &vallen) < 0) \
                goto Bail; \
        s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
        VAR = (unsigned char*) s; \
        continue; \
    } do {} while(0)

#define extractStr2(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; int vallen; \
        if (ccnl_ccnb_consume(typ, num, buf, buflen, &valptr, &vallen) < 0) \
                goto Bail; \
        s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
    VAR = (unsigned char*) s; \
        continue; \
    } do {} while(0)

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNB

#ifdef NEEDS_PACKET_CRAFTING
int
ccnl_ccnb_mkInterest(struct ccnl_prefix_s *name, char *minSuffix,
                     char *maxSuffix, unsigned char *digest, int dlen,
                     unsigned char *publisher, int plen, char *scope,
                     uint32_t *nonce, unsigned char *out)
{
    int len, i, k;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    for (i = 0; i < name->compcnt; i++) {
        len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG); // comp
        k = name->complen[i];
        len += ccnl_ccnb_mkHeader(out+len, k, CCN_TT_BLOB);
        memcpy(out+len, name->comp[i], k);
        len += k;
        out[len++] = 0; // end-of-component
    }
    if (digest) {
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_COMPONENT,
                                 CCN_TT_BLOB, digest, dlen);
        if (!maxSuffix)
            maxSuffix = "0";
    }
    out[len++] = 0; // end-of-name

    if (minSuffix)
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_MINSUFFCOMP,
                                 CCN_TT_UDATA, (unsigned char*) minSuffix,
                                 strlen(minSuffix));
    if (maxSuffix)
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_MAXSUFFCOMP,
                                 CCN_TT_UDATA, (unsigned char*) maxSuffix,
                                 strlen(maxSuffix));
    if (publisher)
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_PUBPUBKDIGEST,
                                 CCN_TT_BLOB, publisher, plen);
    if (scope)
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_SCOPE,
                                 CCN_TT_UDATA, (unsigned char*) scope,
                                 strlen(scope));
    if (nonce)
        len += ccnl_ccnb_mkField(out + len, CCN_DTAG_NONCE,
                                 CCN_TT_UDATA, (unsigned char*) nonce,
                                 sizeof(*nonce));

    out[len++] = 0; // end-of-interest

    return len;
}
#endif

int ccnb_isContent(unsigned char *buf, int len)
{
    int num, typ;

    if (len < 0 || ccnl_ccnb_dehead(&buf, &len, &num, &typ))
        return -1;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        return 0;
    return 1;
}
#endif // USE_SUITE_CCNB

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CCNTLV

#ifdef NEEDS_PACKET_CRAFTING
int
ccntlv_mkInterest(struct ccnl_prefix_s *name, int *dummy,
                  unsigned char *out, int outlen)
{
     int len, offset;

     offset = outlen;
     len = ccnl_ccntlv_prependChunkInterestWithHdr(name, &offset, out);
     if (len > 0)
         memmove(out, out + offset, len);

     return len;
}
#endif

int ccntlv_isObject(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx201411_s *hp = (struct ccnx_tlvhdr_ccnx201411_s*)buf;
    unsigned short payloadlen, hdrlen;

    if (len < sizeof(struct ccnx_tlvhdr_ccnx201411_s)) {
        fprintf(stderr, "ccntlv header not large enough");
        return -1;
    }
    hdrlen = hp->hdrlen; // ntohs(hp->hdrlen);
    payloadlen = ntohs(hp->payloadlen);

    if (hp->version != CCNX_TLV_V0) {
        fprintf(stderr, "ccntlv version %d not supported\n", hp->version);
        return -1;
    }

    if (hdrlen + payloadlen < len) {
        fprintf(stderr, "ccntlv hdr + payload do not fit into received buf\n");
        return -1;
    }
    buf += hdrlen;
    len -= hdrlen;

    if(hp->packettype == CCNX_PT_ContentObject)
        return 1;
    else
        return 0;
}
#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_NDNTLV

#ifdef NEEDS_PACKET_CRAFTING
int
ndntlv_mkInterest(struct ccnl_prefix_s *name, int *nonce,
                  unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ndntlv_prependInterest(name, -1, nonce, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}
#endif

int ndntlv_isData(unsigned char *buf, int len)
{
    int typ, vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen))
        return -1;
    if (typ != NDN_TLV_Data)
        return 0;
    return 1;
}
#endif // USE_SUITE_NDNTLV

#endif //CCNL_COMMON_C
// eof
