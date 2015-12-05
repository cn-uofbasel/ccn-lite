/*
 * @f util/ccnl-common.c
 * @b common functions for the CCN-lite utilities
 *
 * Copyright (C) 2013-15, Christian Tschudin, University of Basel
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


#ifndef CCNL_UAPI_H_    // if CCNL_UAPI_H_ is defined then the following config is taken care elsewhere in the code composite

#define USE_IPV4

#define USE_LOGGING
#define CCNL_UNIX
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _SVID_SOURCE

#if defined(USE_FRAG) || defined(USE_MGMT) || defined(USE_NFN) || defined(USE_SIGNATURES)
# define NEEDS_PACKET_CRAFTING
#endif

// ----------------------------------------------------------------------

//#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

//extern int getline(char **lineptr, size_t *n, FILE *stream);

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "base64.c"

#include "../ccnl-os-includes.h"
#include "../ccnl-defs.h"
#include "../ccnl-core.h"
#include "../ccnl-ext.h"
#include "../ccnl-ext-debug.c"
#include "../ccnl-os-time.c"
#include "../ccnl-ext-logging.c"

int debug_level = WARNING;

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)
#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)

#define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)

struct ccnl_prefix_s* ccnl_prefix_new(int suite, int cnt);
int ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#include "../ccnl-pkt-switch.c"
#include "../ccnl-pkt-ccnb.c"
#include "../ccnl-pkt-ccntlv.c"
#include "../ccnl-pkt-cistlv.c"
#include "../ccnl-pkt-iottlv.c"
#include "../ccnl-pkt-ndntlv.c"
#include "../ccnl-pkt-localrpc.c"

#define ccnl_core_addToCleanup(b)       do{}while(0)

// include only the utils, not the core routines:
#include "../ccnl-ext.h"
#include "../ccnl-core-util.c"
#ifdef USE_FRAG
#include "../ccnl-ext-frag.c"
#endif
#include "../ccnl-ext-hmac.c"

#else // CCNL_UAPI_H_ is defined

#include "base64.c"
#ifdef RIOT_VERSION
#include "ccnl-defs.h"
#include "net/packet.h"
#include <unistd.h>
#include "sys/socket.h"
#include "ccn-lite-riot.h"
#include "ccnl-headers.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-iottlv.h"
#include "ccnl-pkt-cistlv.h"
#include "ccnl-pkt-ccnb.h"

int debug_level = WARNING;

extern int ccnl_suite2defaultPort(int suite);
#endif

#endif // CCNL_UAPI_H_


// ----------------------------------------------------------------------

const char*
ccnl_enc2str(int enc)
{
    switch(enc) {
    case CCNL_ENC_CCNB:      return CONSTSTR("ccnb");
    case CCNL_ENC_NDN2013:   return CONSTSTR("ndn2013");
    case CCNL_ENC_CCNX2014:  return CONSTSTR("ccnbx2014");
    case CCNL_ENC_IOT2014:   return CONSTSTR("iot2014");
    case CCNL_ENC_LOCALRPC:  return CONSTSTR("localrpc");
    case CCNL_ENC_CISCO2015: return CONSTSTR("cisco2015");
    default:
        break;
    }

    return CONSTSTR("?");
}

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
                     const char *maxSuffix, unsigned char *digest, int dlen,
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
    (void) dummy;
     int len, offset;

     offset = outlen;
     len = ccnl_ccntlv_prependChunkInterestWithHdr(name, &offset, out);
     if (len > 0)
         memmove(out, out + offset, len);

     return len;
}
#endif

struct ccnx_tlvhdr_ccnx2015_s*
ccntlv_isHeader(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = (struct ccnx_tlvhdr_ccnx2015_s*)buf;

    if ((unsigned int)len < sizeof(struct ccnx_tlvhdr_ccnx2015_s)) {
        DEBUGMSG(ERROR, "ccntlv header not large enough\n");
        return NULL;
    }
    if (hp->version != CCNX_TLV_V1) {
        DEBUGMSG(ERROR, "ccntlv version %d not supported\n", hp->version);
        return NULL;
    }
    if (ntohs(hp->pktlen) < len) {
        DEBUGMSG(ERROR, "ccntlv packet too small (%d instead of %d bytes)\n",
                 ntohs(hp->pktlen), len);
        return NULL;
    }
    return hp;
}

int ccntlv_isData(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Data;
}

int ccntlv_isFragment(unsigned char *buf, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp = ccntlv_isHeader(buf, len);

    return hp && hp->pkttype == CCNX_PT_Fragment;
}

#endif // USE_SUITE_CCNTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_CISTLV

#ifdef NEEDS_PACKET_CRAFTING
int
cistlv_mkInterest(struct ccnl_prefix_s *name, int *dummy,
                  unsigned char *out, int outlen)
{
    (void) dummy;
    int len, offset;

    offset = outlen;
    len = ccnl_cistlv_prependChunkInterestWithHdr(name, &offset, out);
    if (len > 0)
        memmove(out, out + offset, len);

    return len;
}
#endif

int cistlv_isData(unsigned char *buf, int len)
{
    struct cisco_tlvhdr_201501_s *hp = (struct cisco_tlvhdr_201501_s*)buf;
    unsigned short hdrlen, pktlen; // payloadlen;

    TRACEIN();

    if (len < (int) sizeof(struct cisco_tlvhdr_201501_s)) {
        DEBUGMSG(ERROR, "cistlv header not large enough");
        return -1;
    }
    hdrlen = hp->hlen; // ntohs(hp->hdrlen);
    pktlen = ntohs(hp->pktlen);
    //    payloadlen = ntohs(hp->payloadlen);

    if (hp->version != CISCO_TLV_V1) {
        DEBUGMSG(ERROR, "cistlv version %d not supported\n", hp->version);
        return -1;
    }

    if (pktlen < len) {
        DEBUGMSG(ERROR, "cistlv packet too small (%d instead of %d bytes)\n",
                 pktlen, len);
        return -1;
    }
    buf += hdrlen;
    len -= hdrlen;

    TRACEOUT();

    if(hp->pkttype == CISCO_PT_Content)
        return 1;
    else
        return 0;
}
#endif // USE_SUITE_CISTLV

// ----------------------------------------------------------------------

#ifdef USE_SUITE_IOTTLV

#ifdef NEEDS_PACKET_CRAFTING
int
iottlv_mkRequest(struct ccnl_prefix_s *name, int *dummy,
                 unsigned char *out, int outlen)
{
    (void) dummy;
    int offset, hoplim = 16;

    offset = outlen;
    if (ccnl_iottlv_prependRequest(name, &hoplim, &offset, out) < 0
              || ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offset, out) < 0)
        return -1;
    memmove(out, out + offset, outlen - offset);

    return outlen - offset;
}
#endif // NEEDS_PACKET_CRAFTING

// return 1 for Reply, 0 for Request, -1 if invalid
int iottlv_isReply(unsigned char *buf, int len)
{
    int enc = 1, suite;
    unsigned int typ;
    int vallen;

    while (!ccnl_switch_dehead(&buf, &len, &enc));
    suite = ccnl_enc2suite(enc);
    if (suite != CCNL_SUITE_IOTTLV)
        return -1;
    DEBUGMSG(DEBUG, "suite ok\n");
    if (len < 1 || ccnl_iottlv_dehead(&buf, &len, &typ, &vallen) < 0)
        return -1;
    DEBUGMSG(DEBUG, "typ=%d, len=%d\n", typ, vallen);
    if (typ == IOT_TLV_Reply)
        return 1;
    if (typ == IOT_TLV_Request)
        return 0;
    return -1;
}

int iottlv_isFragment(unsigned char *buf, int len)
{
    int enc;
    while (!ccnl_switch_dehead(&buf, &len, &enc));
    return ccnl_iottlv_peekType(buf, len) == IOT_TLV_Fragment;
}


#endif // USE_SUITE_IOTTLV

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
#endif // NEEDS_PACKET_CRAFTING

int ndntlv_isData(unsigned char *buf, int len)
{
    int typ;
    int vallen;

    if (len < 0 || ccnl_ndntlv_dehead(&buf, &len, (int*) &typ, &vallen))
        return -1;
    if (typ != NDN_TLV_Data)
        return 0;
    return 1;
}
#endif // USE_SUITE_NDNTLV

// ----------------------------------------------------------------------

#ifdef NEEDS_PACKET_CRAFTING
typedef int (*ccnl_mkInterestFunc)(struct ccnl_prefix_s*, int*, unsigned char*, int);
#endif // NEEDS_PACKET_CRAFTING
typedef int (*ccnl_isContentFunc)(unsigned char*, int);
typedef int (*ccnl_isFragmentFunc)(unsigned char*, int);

#ifdef NEEDS_PACKET_CRAFTING
ccnl_mkInterestFunc
ccnl_suite2mkInterestFunc(int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        return &ccnl_ccnb_fillInterest;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return &ccntlv_mkInterest;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        return &cistlv_mkInterest;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        return &iottlv_mkRequest;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        return &ndntlv_mkInterest;
#endif
    }

    DEBUGMSG(WARNING, "unknown suite %d in %s:%d\n",
                      suite, __func__, __LINE__);
    return NULL;
}
#endif // NEEDS_PACKET_CRAFTING

ccnl_isContentFunc
ccnl_suite2isContentFunc(int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        return &ccnb_isContent;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return &ccntlv_isData;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        return &cistlv_isData;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        return &iottlv_isReply;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        return &ndntlv_isData;
#endif
    }

    DEBUGMSG(WARNING, "unknown suite %d in %s:%d\n",
                      suite, __func__, __LINE__);
    return NULL;
}

ccnl_isFragmentFunc
ccnl_suite2isFragmentFunc(int suite)
{
    switch(suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        return &ccntlv_isFragment;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV:
        return &iottlv_isFragment;
#endif
    }

    DEBUGMSG(DEBUG, "unknown suite %d in %s of %s:%d\n",
                    suite, __func__, __FILE__, __LINE__);
    return NULL;
}

// ----------------------------------------------------------------------

struct key_s {
    struct key_s *next;
    unsigned char* key;
    int keylen;
};

struct key_s*
load_keys_from_file(char *path)
{
    FILE *fp = fopen(path, "r");
    char line[256];
    int cnt = 0;
    struct key_s *klist = NULL, *kend = NULL;

    if (!fp) {
        perror("file open");
        return NULL;
    }
    while (fgets(line, sizeof(line), fp)) {
        unsigned char *key;
        size_t keylen;
        int read = strlen(line);
        DEBUGMSG(TRACE, "  read %d bytes\n", read);
        if (line[read-1] == '\n')
            line[--read] = '\0';
        key = base64_decode(line, read, &keylen);
        if (key && keylen > 0) {
            struct key_s *k = (struct key_s *) calloc(1, sizeof(struct key_s*));
            k->keylen = keylen;
            k->key = key;
            if (kend)
                kend->next = k;
            else
                klist = k;
            kend = k;
            cnt++;
            DEBUGMSG(VERBOSE, "  key #%d: %d bytes\n", cnt, (int)keylen);
            if (keylen < 32) {
                DEBUGMSG(WARNING, "key #%d: should choose a longer key!\n", cnt);
            }
        }
    }
    fclose(fp);
    DEBUGMSG(DEBUG, "read %d keys from file %s\n", cnt, optarg);
    return klist;
}

// ----------------------------------------------------------------------

int
ccnl_parseUdp(char *udp, int suite, char **addr, int *port)
{
    char *tmpAddr = NULL;
    char *tmpPortStr = NULL;
    char *end = NULL;
    int tmpPort;

    if (!udp) {
        *addr = "127.0.0.1";
        *port = ccnl_suite2defaultPort(suite);
        return 0;
    }

    if (!strchr(udp, '/')) {
        DEBUGMSG(ERROR, "invalid UDP destination, missing port: %s\n", udp);
        return -1;
    }
    tmpAddr = strtok(udp, "/");
    tmpPortStr = strtok(NULL, "/");
    if (!tmpPortStr) {
        DEBUGMSG(ERROR, "invalid UDP destination, empty UDP port: %s/\n", udp);
        return -1;
    }
    tmpPort = strtol(tmpPortStr, &end, 10);
    if (*end != '\0') {
        DEBUGMSG(ERROR, "invalid UDP destination, cannot parse port as number: %s\n", tmpPortStr);
        return -1;
    }

    *addr = tmpAddr;
    *port = tmpPort;
    return 0;
}

// eof
