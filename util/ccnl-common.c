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
// ----------------------------------------------------------------------

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>


#include "base64.c"

#include "../ccnl-const.h"
#include "../ccnl-core.h"

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)
#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)
#define free_prefix(p)  do { if (p) { free(p->comp); free(p->complen); free(p->bytes); free(p); }} while(0)

#define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)
#define DEBUGSTMT(LVL, ...)             do {} while(0)

int debug_level;

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

struct ccnl_prefix_s* ccnl_prefix_new(int suite, int cnt);
int ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#include "../pkt-ccnb.c"
#include "../pkt-ccntlv.c"
#include "../pkt-ndntlv.c"
#include "../pkt-localrpc.h"

#define ccnl_core_addToCleanup(b)       do{}while(0)

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
int ccnb_isContent(unsigned char *buf, int len)
{
    int num, typ;

    if (len < 0 || ccnl_ccnb_dehead(&buf, &len, &num, &typ))
        return -1;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        return 0;
    return 1;
}
#endif

#ifdef USE_SUITE_CCNTLV
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

int ccntlv_isObject(unsigned char *buf, int len)
{
    if (len <= sizeof(struct ccnx_tlvhdr_ccnx201409_s)) {
        fprintf(stderr, "ccntlv header not large enough");
        return -1;
    }
    struct ccnx_tlvhdr_ccnx201409_s *hp = (struct ccnx_tlvhdr_ccnx201409_s*)buf;
    unsigned short hdrlen = ntohs(hp->hdrlen);
    unsigned short payloadlen = ntohs(hp->payloadlen);

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
#endif

#ifdef USE_SUITE_NDNTLV
int
ndntlv_mkInterest(struct ccnl_prefix_s *name, int *nonce,
                  unsigned char *out, int outlen)
{
    int len, offset;

    offset = outlen;
    len = ccnl_ndntlv_fillInterest(name, -1, nonce, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}

int ndntlv_isData(unsigned char *buf, int len)
{
    int typ, vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen))
        return -1;
    if (typ != NDN_TLV_Data)
        return 0;
    return 1;
}
#endif

#endif //CCNL_COMMON_C
// eof
