/*
 * @f ccnl-core-util.c
 * @b CCN lite, common utility procedures (used by utils as well as relays)
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
 * Copyright (C) 2016, Martine Lenders <mlenders@inf.fu-berlin.de>
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
 * 2014-06-18 created
 */

//#ifndef CCNL_CORE_UTIL_H
//#define CCNL_CORE_UTIL_H

#include "ccnl-core.h"

// ----------------------------------------------------------------------
// collect the USE_* macros in a string

#ifdef CCNL_ARDUINO
const char compile_string[] PROGMEM = ""
#else
const char *compile_string = ""
#endif

#ifdef USE_CCNxDIGEST
        "CCNxDIGEST, "
#endif
#ifdef USE_DEBUG
        "DEBUG, "
#endif
#ifdef USE_DEBUG_MALLOC
        "DEBUG_MALLOC, "
#endif
#ifdef USE_ECHO
        "ECHO, "
#endif
#ifdef USE_LINKLAYER
        "ETHERNET, "
#endif
#ifdef USE_FRAG
        "FRAG, "
#endif
#ifdef USE_HMAC256
        "HMAC256, "
#endif
#ifdef USE_HTTP_STATUS
        "HTTP_STATUS, "
#endif
#ifdef USE_KITE
        "KITE, "
#endif
#ifdef USE_LOGGING
        "LOGGING, "
#endif
#ifdef USE_MGMT
        "MGMT, "
#endif
#ifdef USE_NACK
        "NACK, "
#endif
#ifdef USE_NFN
        "NFN, "
#endif
#ifdef USE_NFN_MONITOR
        "NFN_MONITOR, "
#endif
#ifdef USE_NFN_NSTRANS
        "NFN_NSTRANS, "
#endif
#ifdef USE_SCHEDULER
        "SCHEDULER, "
#endif
#ifdef USE_SIGNATURES
        "SIGNATURES, "
#endif
#ifdef USE_SUITE_CCNB
        "SUITE_CCNB, "
#endif
#ifdef USE_SUITE_CCNTLV
        "SUITE_CCNTLV, "
#endif
#ifdef USE_SUITE_CISTLV
        "SUITE_CISTLV, "
#endif
#ifdef USE_SUITE_IOTTLV
        "SUITE_IOTTLV, "
#endif
#ifdef USE_SUITE_LOCALRPC
        "SUITE_LOCALRPC, "
#endif
#ifdef USE_SUITE_NDNTLV
        "SUITE_NDNTLV, "
#endif
#ifdef USE_UNIXSOCKET
        "UNIXSOCKET, "
#endif
        ;

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

const char*
ccnl_matchMode2str(int mode)
{
    switch (mode) {
    case CMP_EXACT:
        return CONSTSTR("CMP_EXACT");
    case CMP_MATCH:
        return CONSTSTR("CMP_MATCH");
    case CMP_LONGEST:
        return CONSTSTR("CMP_LONGEST");
    }

    return CONSTSTR("?");
}

int
ccnl_prefix_cmp(struct ccnl_prefix_s *pfx, unsigned char *md,
                struct ccnl_prefix_s *nam, int mode)
/* returns -1 if no match at all (all modes) or exact match failed
   returns  0 if full match (CMP_EXACT)
   returns n>0 for matched components (CMP_MATCH, CMP_LONGEST) */
{
    int i, clen, plen = pfx->compcnt + (md ? 1 : 0), rc = -1;
    unsigned char *comp;

    char *s1 = NULL, *s2 = NULL;
    DEBUGMSG(VERBOSE, "prefix_cmp(mode=%s) prefix=<%s> of? name=<%s> digest=%p\n",
             ccnl_matchMode2str(mode),
             (s1 = ccnl_prefix_to_path(pfx)), (s2 = ccnl_prefix_to_path(nam)), (void *) md);
    ccnl_free(s1);
    ccnl_free(s2);

    if (mode == CMP_EXACT) {
        if (plen != nam->compcnt)
            goto done;
        if (pfx->chunknum || nam->chunknum) {
            if (!pfx->chunknum || !nam->chunknum)
                goto done;
            if (*pfx->chunknum != *nam->chunknum)
                goto done;
        }
#ifdef USE_NFN
        if (nam->nfnflags != pfx->nfnflags)
            goto done;
#endif
    }
    for (i = 0; i < plen && i < nam->compcnt; ++i) {
        comp = i < pfx->compcnt ? pfx->comp[i] : md;
        clen = i < pfx->compcnt ? pfx->complen[i] : 32; // SHA256_DIGEST_LEN
        if (clen != nam->complen[i] || memcmp(comp, nam->comp[i], nam->complen[i])) {
            rc = mode == CMP_EXACT ? -1 : i;
            goto done;
        }
    }
    // FIXME: we must also inspect chunknum here!
    rc = (mode == CMP_EXACT) ? 0 : i;
done:
    DEBUGMSG(TRACE, "  cmp result: pfxlen=%d cmplen=%d namlen=%d matchlen=%d\n",
             pfx->compcnt, plen, nam->compcnt, rc);
    return rc;
}

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
                  int minsuffix, int maxsuffix, struct ccnl_content_s *c)
{
    unsigned char *md;
    struct ccnl_prefix_s *p = c->pkt->pfx;

    char *s1 = NULL, *s2 = NULL;
    DEBUGMSG(VERBOSE, "ccnl_i_prefixof_c prefix=<%s> content=<%s> min=%d max=%d\n",
             (s1 = ccnl_prefix_to_path(prefix)), (s2 = ccnl_prefix_to_path(p)),
             // ccnl_prefix_to_path_detailed(prefix,1,0,0),
             // ccnl_prefix_to_path_detailed(p,1,0,0),
             minsuffix, maxsuffix);
    ccnl_free(s1);
    ccnl_free(s2);
    // CONFORM: we do prefix match, honour min. and maxsuffix,

    // NON-CONFORM: "Note that to match a ContentObject must satisfy
    // all of the specifications given in the Interest Message."
    // >> CCNL does not honour the exclusion filtering

    if ( (prefix->compcnt + minsuffix) > (p->compcnt + 1) ||
         (prefix->compcnt + maxsuffix) < (p->compcnt + 1)) {
        DEBUGMSG(TRACE, "  mismatch in # of components\n");
        return 0;
    }

    md = (prefix->compcnt - p->compcnt == 1) ? compute_ccnx_digest(c->pkt->buf) : NULL;
    return ccnl_prefix_cmp(p, md, prefix, CMP_MATCH) == prefix->compcnt;
}

#endif

// ----------------------------------------------------------------------

int
ccnl_is_local_addr(sockunion *su)
{
    if (!su)
        return 0;
#ifdef USE_UNIXSOCKET
    if (su->sa.sa_family == AF_UNIX)
        return -1;
#endif
#ifdef USE_IPV4
    if (su->sa.sa_family == AF_INET)
        return su->ip4.sin_addr.s_addr == htonl(0x7f000001);
#endif
#ifdef USE_IPV6
    if (su->sa.sa_family == AF_INET6) {
        static const unsigned char loopback_bytes[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        static const unsigned char mapped_ipv4_loopback[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
        return ((memcmp(su->ip6.sin6_addr.s6_addr, loopback_bytes, 16) == 0) ||
                (memcmp(su->ip6.sin6_addr.s6_addr, mapped_ipv4_loopback, 16) == 0));
    }

#endif
    return 0;
}

int
ccnl_str2suite(char *cp)
{
    if (!cp)
        return -1;
#ifdef USE_SUITE_CCNB
    if (!strcmp(cp, CONSTSTR("ccnb")))
        return CCNL_SUITE_CCNB;
#endif
#ifdef USE_SUITE_CCNTLV
    if (!strcmp(cp, CONSTSTR("ccnx2015")))
        return CCNL_SUITE_CCNTLV;
#endif
#ifdef USE_SUITE_CISTLV
    if (!strcmp(cp, CONSTSTR("cisco2015")))
        return CCNL_SUITE_CISTLV;
#endif
#ifdef USE_SUITE_IOTTLV
    if (!strcmp(cp, CONSTSTR("iot2014")))
        return CCNL_SUITE_IOTTLV;
#endif
#ifdef USE_SUITE_LOCALRPC
    if (!strcmp(cp, CONSTSTR("localrpc")))
        return CCNL_SUITE_LOCALRPC;
#endif
#ifdef USE_SUITE_NDNTLV
    if (!strcmp(cp, CONSTSTR("ndn2013")))
        return CCNL_SUITE_NDNTLV;
#endif
    return -1;
}

const char*
ccnl_suite2str(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return CONSTSTR("ccnb");
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return CONSTSTR("ccnx2015");
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return CONSTSTR("cisco2015");
#endif
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return CONSTSTR("iot2014");
#endif
#ifdef USE_SUITE_LOCALRPC
    if (suite == CCNL_SUITE_LOCALRPC)
        return CONSTSTR("localrpc");
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return CONSTSTR("ndn2013");
#endif
    return CONSTSTR("?");
}

int
ccnl_suite2defaultPort(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return CCN_UDP_PORT;
#endif
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return NDN_UDP_PORT;
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return NDN_UDP_PORT;
#endif
    return NDN_UDP_PORT;
}

bool
ccnl_isSuite(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return true;
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return true;
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        return true;
#endif
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return true;
#endif
#ifdef USE_SUITE_LOCALRPC
    if (suite == CCNL_SUITE_LOCALRPC)
        return true;
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return true;
#endif
    return false;
}

// ----------------------------------------------------------------------

struct ccnl_prefix_s*
ccnl_prefix_new(int suite, int cnt)
{
    struct ccnl_prefix_s *p;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
        return NULL;
    p->comp = (unsigned char**) ccnl_malloc(cnt * sizeof(unsigned char*));
    p->complen = (int*) ccnl_malloc(cnt * sizeof(int));
    if (!p->comp || !p->complen) {
        free_prefix(p);
        return NULL;
    }
    p->compcnt = cnt;
    p->suite = suite;
    p->chunknum = NULL;

    return p;
}

int
hex2int(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(char *comp) // inplace, returns len after shrinking
{
    char *in = comp, *out = comp;
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
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen)
{
    int len = 0;

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CCNX_TLV_N_NameSegment);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV: {
        unsigned short *sp = (unsigned short*) dst;
        *sp++ = htons(CISCO_TLV_NameComponent);
        len = srclen;
        *sp++ = htons(len);
        memcpy(sp, src, len);
        len += 2*sizeof(unsigned short);
        break;
    }
#endif
    default:
        len = srclen;
        memcpy(dst, src, len);
        break;
    }

    return len;
}

int
ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf)
{
    int len = strlen(src);

    DEBUGMSG_CUTL(TRACE, "ccnl_pkt_prependComponent(%d, %s)\n", suite, src);

    if (*offset < len)
        return -1;
    memcpy(buf + *offset - len, src, len);
    *offset -= len;

#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CCNX_TLV_N_NameSegment);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV) {
        unsigned short *sp = (unsigned short*) (buf + *offset) - 1;
        if (*offset < 4)
            return -1;
        *sp-- = htons(len);
        *sp = htons(CISCO_TLV_NameComponent);
        len += 2*sizeof(unsigned short);
        *offset -= 2*sizeof(unsigned short);
    }
#endif

    return len;
}

// fill in the compVector (watch out: this modifies the uri string)
int
ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri)
{
    int i, len;

    if (*uri == '/')
        uri++;

    for (i = 0; *uri && i < (CCNL_MAX_NAME_COMP - 1); i++) {
        compVector[i] = uri;
        while (*uri && *uri != '/') {
            uri++;
        }
        if (*uri) {
            *uri = '\0';
            uri++;
        }
        len = unescape_component(compVector[i]);

        if (compLens)
            compLens[i] = len;

        compVector[i][len] = '\0';
    }
    compVector[i] = NULL;

    return i;
}

// turn an URI into an internal prefix (watch out: this modifies the uri string)
struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, char *nfnexpr, unsigned int *chunknum)
{
    struct ccnl_prefix_s *p;
    char *compvect[CCNL_MAX_NAME_COMP];
    unsigned int complens[CCNL_MAX_NAME_COMP];
    int cnt, i, len, tlen;

    DEBUGMSG_CUTL(TRACE, "ccnl_URItoPrefix(suite=%s, uri=%s, nfn=%s)\n",
             ccnl_suite2str(suite), uri, nfnexpr);

    if (strlen(uri))
        cnt = ccnl_URItoComponents(compvect, complens, uri);
    else
        cnt = 0;

    if (nfnexpr && *nfnexpr)
        cnt += 1;

    p = ccnl_prefix_new(suite, cnt);
    if (!p)
        return NULL;

    for (i = 0, len = 0; i < cnt; i++) {
        if (i == (cnt-1) && nfnexpr && *nfnexpr)
            len += strlen(nfnexpr);
        else
            len += complens[i];//strlen(compvect[i]);
    }
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        len += cnt * 4; // add TL size
#endif
#ifdef USE_SUITE_CISTLV
    if (suite == CCNL_SUITE_CISTLV)
        len += cnt * 4; // add TL size
#endif

    p->bytes = (unsigned char*) ccnl_malloc(len);
    if (!p->bytes) {
        free_prefix(p);
        return NULL;
    }

    for (i = 0, len = 0, tlen = 0; i < cnt; i++) {
        int isnfnfcomp = i == (cnt-1) && nfnexpr && *nfnexpr;
        char *cp = isnfnfcomp ? nfnexpr : (char*) compvect[i];

        if (isnfnfcomp)
            tlen = strlen(nfnexpr);
        else
            tlen = complens[i];

        p->comp[i] = p->bytes + len;
        tlen = ccnl_pkt_mkComponent(suite, p->comp[i], cp, tlen);
        p->complen[i] = tlen;
        len += tlen;
    }

    p->compcnt = cnt;
#ifdef USE_NFN
    if (nfnexpr && *nfnexpr)
        p->nfnflags |= CCNL_PREFIX_NFN;
#endif

    if(chunknum) {
        p->chunknum = (int*) ccnl_malloc(sizeof(int));
        *p->chunknum = *chunknum;
    }

    return p;
}

struct ccnl_prefix_s*
ccnl_prefix_dup(struct ccnl_prefix_s *prefix)
{
    int i = 0, len;
    struct ccnl_prefix_s *p;

    p = ccnl_prefix_new(prefix->suite, prefix->compcnt);
    if (!p)
        return p;

    p->compcnt = prefix->compcnt;
    p->chunknum = prefix->chunknum;
#ifdef USE_NFN
    p->nfnflags = prefix->nfnflags;
#endif

    for (i = 0, len = 0; i < prefix->compcnt; i++)
        len += prefix->complen[i];
    p->bytes = (unsigned char*) ccnl_malloc(len);
    if (!p->bytes) {
        free_prefix(p);
        return NULL;
    }

    for (i = 0, len = 0; i < prefix->compcnt; i++) {
        p->complen[i] = prefix->complen[i];
        p->comp[i] = p->bytes + len;
        memcpy(p->bytes + len, prefix->comp[i], p->complen[i]);
        len += p->complen[i];
    }

    if (prefix->chunknum) {
        p->chunknum = (int*) ccnl_malloc(sizeof(int));
        *p->chunknum = *prefix->chunknum;
    }

    return p;
}


int
ccnl_prefix_appendCmp(struct ccnl_prefix_s *prefix, unsigned char *cmp,
                      int cmplen)
{
    int lastcmp = prefix->compcnt, i;
    int *oldcomplen = prefix->complen;
    unsigned char **oldcomp = prefix->comp;
    unsigned char *oldbytes = prefix->bytes;

    int prefixlen = 0;

    if (prefix->compcnt + 1 > CCNL_MAX_NAME_COMP)
        return -1;
    for (i = 0; i < lastcmp; i++) {
        prefixlen += prefix->complen[i];
    }

    prefix->compcnt++;
    prefix->comp = (unsigned char**) ccnl_malloc(prefix->compcnt * sizeof(unsigned char*));
    prefix->complen = (int*) ccnl_malloc(prefix->compcnt * sizeof(int));
    prefix->bytes = (unsigned char*) ccnl_malloc(prefixlen + cmplen);

    memcpy(prefix->bytes, oldbytes, prefixlen);
    memcpy(prefix->bytes + prefixlen, cmp, cmplen);

    prefixlen = 0;
    for (i = 0; i < lastcmp; i++) {
        prefix->comp[i] = &prefix->bytes[prefixlen];
        prefix->complen[i] = oldcomplen[i];
        prefixlen += oldcomplen[i];
    }
    prefix->comp[lastcmp] = &prefix->bytes[prefixlen];
    prefix->complen[lastcmp] = cmplen;

    ccnl_free(oldcomp);
    ccnl_free(oldcomplen);
    ccnl_free(oldbytes);

    return 0;
}

// TODO: This function should probably be moved to another file to indicate that it should only be used by application level programs
// and not in the ccnl core. Chunknumbers for NDNTLV are only a convention and there no specification on the packet encoding level.
int
ccnl_prefix_addChunkNum(struct ccnl_prefix_s *prefix, unsigned int chunknum)
{
    if (chunknum >= 0xff) {
      DEBUGMSG_CUTL(WARNING, "addChunkNum is only implemented for "
               "chunknum smaller than 0xff (%d)\n", chunknum);
        return -1;
    }

    switch(prefix->suite) {
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV: {
            unsigned char cmp[2];
            cmp[0] = NDN_Marker_SegmentNumber;
            // TODO: this only works for chunknums smaller than 255
            cmp[1] = chunknum;
            if(ccnl_prefix_appendCmp(prefix, cmp, 2) < 0)
                return -1;
            if (prefix->chunknum)
                ccnl_free(prefix->chunknum);
            prefix->chunknum = (int*) ccnl_malloc(sizeof(int));
            *prefix->chunknum = chunknum;
        }
        break;
#endif

#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            unsigned char cmp[5];
            cmp[0] = 0;
            // TODO: this only works for chunknums smaller than 255
            cmp[1] = CCNX_TLV_N_Chunk;
            cmp[2] = 0;
            cmp[3] = 1;
            cmp[4] = chunknum;
            if(ccnl_prefix_appendCmp(prefix, cmp, 5) < 0)
                return -1;
            if (prefix->chunknum)
                ccnl_free(prefix->chunknum);
            prefix->chunknum = (int*) ccnl_malloc(sizeof(int));
            *prefix->chunknum = chunknum;
        }
        break;
#endif

#ifdef USE_SUITE_CISTLV
        case CCNL_SUITE_CISTLV: {
            unsigned char cmp[5];
            // TODO: this only works for chunknums smaller than 255
            cmp[0] = 0;
            cmp[1] = CISCO_TLV_NameSegment;
            cmp[2] = 0;
            cmp[3] = 1;
            cmp[4] = chunknum;
            if (ccnl_prefix_appendCmp(prefix, cmp, 5) < 0)
                return -1;
            if (prefix->chunknum)
                ccnl_free(prefix->chunknum);
            prefix->chunknum = (int*) ccnl_malloc(sizeof(int));
            *prefix->chunknum = chunknum;
        }
        break;
#endif

        default:
            DEBUGMSG_CUTL(WARNING,
                     "add chunk number not implemented for suite %d\n",
                     prefix->suite);
            return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------

int
ccnl_pkt2suite(unsigned char *data, int len, int *skip)
{
    int enc, suite = -1;
    unsigned char *olddata = data;

    if (skip)
        *skip = 0;

    if (len <= 0)
        return -1;

    DEBUGMSG_CUTL(TRACE, "pkt2suite %d %d\n", data[0], data[1]);

    while (!ccnl_switch_dehead(&data, &len, &enc))
        suite = ccnl_enc2suite(enc);
    if (skip)
        *skip = data - olddata;
    if (suite >= 0)
        return suite;

#ifdef USE_SUITE_CCNB
    if (*data == 0x04)
        return CCNL_SUITE_CCNB;
    if (*data == 0x01 && len > 1 && // check for CCNx2015 and Cisco collision:
                                (data[1] != 0x00 && // interest
                                 data[1] != 0x01 && // data
                                 data[1] != 0x02 && // interestReturn
                                 data[1] != 0x03))  // fragment
        return CCNL_SUITE_CCNB;
#endif

#ifdef USE_SUITE_CCNTLV
    if (data[0] == CCNX_TLV_V1 && len > 1) {
        if (data[1] == CCNX_PT_Interest ||
            data[1] == CCNX_PT_Data ||
            data[1] == CCNX_PT_Fragment ||
            data[1] == CCNX_PT_NACK)
            return CCNL_SUITE_CCNTLV;
    }
#endif

#ifdef USE_SUITE_CISTLV
    if (data[0] == CISCO_TLV_V1 && len > 1) {
        if (data[1] == CISCO_PT_Interest ||
            data[1] == CISCO_PT_Content ||
            data[1] == CISCO_PT_Nack)
            return CCNL_SUITE_CISTLV;
    }
#endif

#ifdef USE_SUITE_NDNTLV
    if (*data == NDN_TLV_Interest || *data == NDN_TLV_Data ||
        *data == NDN_TLV_Fragment)
        return CCNL_SUITE_NDNTLV;
#endif

/*
#ifdef USE_SUITE_IOTTLV
        if (*data == IOT_TLV_Request || *data == IOT_TLV_Reply)
            return CCNL_SUITE_IOTTLV;
#endif

#ifdef USE_SUITE_LOCALRPC
        if (*data == LRPC_PT_REQUEST || *data == LRPC_PT_REPLY)
            return CCNL_SUITE_LOCALRPC;
#endif
    }
*/

    return -1;
}

void
free_packet(struct ccnl_pkt_s *pkt)
{
    if (pkt) {
        if (pkt->pfx) {
            switch (pkt->pfx->suite) {
#ifdef USE_SUITE_CCNB
            case CCNL_SUITE_CCNB:
                ccnl_free(pkt->s.ccnb.nonce);
                ccnl_free(pkt->s.ccnb.ppkd);
                break;
#endif
#ifdef USE_SUITE_CCNTLV
            case CCNL_SUITE_CCNTLV:
                ccnl_free(pkt->s.ccntlv.keyid);
                break;
#endif
#ifdef USE_SUITE_NDNTLV
            case CCNL_SUITE_NDNTLV:
                ccnl_free(pkt->s.ndntlv.nonce);
                ccnl_free(pkt->s.ndntlv.ppkl);
                break;
#endif
#ifdef USE_SUITE_CISTLV
            case CCNL_SUITE_CISTLV:
#endif
#ifdef USE_SUITE_IOTTLV
            case CCNL_SUITE_IOTTLV:
#endif
#ifdef USE_SUITE_LOCALRPC
            case CCNL_SUITE_LOCALRPC:
#endif
            default:
                break;
            }
            free_prefix(pkt->pfx);
        }
        ccnl_free(pkt->buf);
        ccnl_free(pkt);
    }
}

// ----------------------------------------------------------------------
static inline char
_half_byte_to_char(uint8_t half_byte)
{
    return (half_byte < 10) ? ('0' + half_byte) : ('a' + (half_byte - 10));
}

/* translates link-layer address into a string */
char*
ll2ascii(unsigned char *addr, size_t len)
{
    size_t i;
    static char out[CCNL_LLADDR_STR_MAX_LEN + 1];

    out[0] = '\0';

    for (i = 0; i < len; i++) {
        out[(3 * i)] = _half_byte_to_char(addr[i] >> 4);
        out[(3 * i) + 1] = _half_byte_to_char(addr[i] & 0xf);

        if (i != (len - 1)) {
            out[(3 * i) + 2] = ':';
        }
        else {
            out[(3 * i) + 2] = '\0';
        }
    }

    return out;
}

char*
ccnl_addr2ascii(sockunion *su)
{
#ifdef USE_UNIXSOCKET
    static char result[256];
#else
    static char result[25];
#endif

    if (!su)
        return CONSTSTR("(local)");

    switch (su->sa.sa_family) {
#ifdef USE_LINKLAYER
    case AF_PACKET: {
        struct sockaddr_ll *ll = &su->linklayer;
        strcpy(result, ll2ascii(ll->sll_addr, ll->sll_halen));
        sprintf(result+strlen(result), "/0x%04x",
            ntohs(ll->sll_protocol));
        return result;
    }
#endif
#ifdef USE_IPV4
    case AF_INET:
        sprintf(result, "%s/%u", inet_ntoa(su->ip4.sin_addr),
                ntohs(su->ip4.sin_port));
        return result;
#endif
#ifdef USE_IPV6
    case AF_INET6: {
        char str[INET6_ADDRSTRLEN];
        sprintf(result, "%s/%u", inet_ntop(AF_INET6, su->ip6.sin6_addr.s6_addr, str, INET6_ADDRSTRLEN),
                ntohs(su->ip6.sin6_port));
        return result;
                   }
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        strncpy(result, su->ux.sun_path, sizeof(result)-1);
        result[sizeof(result)-1] = 0;
        return result;
#endif
    default:
        break;
    }

    (void) result; // silence compiler warning (if neither USE_LINKLAYER, USE_IPV4, USE_IPV6, nor USE_UNIXSOCKET is set)
    return NULL;
}

// ----------------------------------------------------------------------
#ifdef NEEDS_PREFIX_MATCHING

/* add a new entry to the FIB */
int
ccnl_add_fib_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face)
{
    struct ccnl_forward_s *fwd, **fwd2;

    char *s = NULL;
    DEBUGMSG_CFWD(INFO, "adding FIB for <%s>, suite %s\n",
             (s = ccnl_prefix_to_path(pfx)), ccnl_suite2str(pfx->suite));
    ccnl_free(s);

    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        if (fwd->suite == pfx->suite &&
                        !ccnl_prefix_cmp(fwd->prefix, NULL, pfx, CMP_EXACT)) {
            free_prefix(fwd->prefix);
            fwd->prefix = NULL;
            break;
        }
    }
    if (!fwd) {
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
        if (!fwd)
            return -1;
        fwd2 = &relay->fib;
        while (*fwd2)
            fwd2 = &((*fwd2)->next);
        *fwd2 = fwd;
        fwd->suite = pfx->suite;
    }
    fwd->prefix = pfx;
    fwd->face = face;
    DEBUGMSG_CUTL(DEBUG, "added FIB via %s\n", ccnl_addr2ascii(&fwd->face->peer));

    return 0;
}

#endif

/* prints the current FIB */
void
ccnl_show_fib(struct ccnl_relay_s *relay)
{
#ifndef CCNL_LINUXKERNEL
    struct ccnl_forward_s *fwd;

    printf("%-30s | %-10s | %-9s | Peer\n",
           "Prefix", "Suite",
#ifdef CCNL_RIOT
           "Interface"
#else
           "Socket"
#endif
           );
    puts("-------------------------------|------------|-----------|------------------------------------");
    char *s = NULL;
    for (fwd = relay->fib; fwd; fwd = fwd->next) {
        printf("%-30s | %-10s |        %02i | %s\n", (s = ccnl_prefix_to_path(fwd->prefix)),
                                     ccnl_suite2str(fwd->suite), (int)
                                     /* TODO: show correct interface instead of always 0 */
#ifdef CCNL_RIOT
                                     (relay->ifs[0]).if_pid,
#else
                                     (relay->ifs[0]).sock,
#endif
                                     ccnl_addr2ascii(&fwd->face->peer));
        ccnl_free(s);
    }
#endif
}

// ----------------------------------------------------------------------

#ifndef CCNL_LINUXKERNEL

char*
ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip,
                             int escape_components, int call_slash)
{
    (void) ccntlv_skip;
    (void) call_slash;
    int len = 0, i, j;
    /*static char *prefix_buf1;
    static char *prefix_buf2;
    static char *buf;*/

#if defined(CCNL_ARDUINO) || defined(CCNL_RIOT)
# define PREFIX_BUFSIZE 50
#else
# define PREFIX_BUFSIZE 2048
#endif

    if (!pr)
        return NULL;

    /*if (!buf) {
        struct ccnl_buf_s *b;
        b = ccnl_buf_new(NULL, PREFIX_BUFSIZE);
        //ccnl_core_addToCleanup(b);
        prefix_buf1 = (char*) b->data;
        b = ccnl_buf_new(NULL, PREFIX_BUFSIZE);
        //ccnl_core_addToCleanup(b);
        prefix_buf2 = (char*) b->data;
        buf = prefix_buf1;
    } else if (buf == prefix_buf2)
        buf = prefix_buf1;
    else
        buf = prefix_buf2;
    */
    char *buf = (char*) ccnl_malloc(PREFIX_BUFSIZE);
    if (buf == NULL) {
        DEBUGMSG_CUTL(ERROR, "ccnl_prefix_to_path_detailed: malloc failed, exiting\n");
        return NULL;
    }

#ifdef USE_NFN
    if (pr->nfnflags & CCNL_PREFIX_NFN)
        len += sprintf(buf + len, "nfn");
    if (pr->nfnflags & CCNL_PREFIX_THUNK)
        len += sprintf(buf + len, "thunk");
    if (pr->nfnflags)
        len += sprintf(buf + len, "[");
#endif

/*
Not sure why a component starting with a call is not printed with a leading '/'
A call should also be printed with a '/' because this function prints a prefix
and prefix components are visually separated with a leading '/'.
One possibility is to not have a '/' before any nfn expression.
#ifdef USE_NFN
        if (pr->compcnt == 1 && (pr->nfnflags & CCNL_PREFIX_NFN) &&
            !strncmp("call", (char*)pr->comp[i] + skip, 4)) {
            len += sprintf(buf + len, "%.*s",
                           pr->complen[i]-skip, pr->comp[i]+skip);
        } else
#endif
*/

    int skip = 0;

#if (defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)) // && defined(USE_NFN)
    // In the future it is possibly helpful to see the type information
    // in the logging output. However, this does not work with NFN because
    // it uses this function to create the names in NFN expressions
    // resulting in CCNTLV type information names within expressions.
    if (ccntlv_skip && (0
#ifdef USE_SUITE_CCNTLV
       || pr->suite == CCNL_SUITE_CCNTLV
#endif
#ifdef USE_SUITE_CISTLV
       || pr->suite == CCNL_SUITE_CISTLV
#endif
                         ))
        skip = 4;
#endif

    for (i = 0; i < pr->compcnt; i++) {
#ifdef USE_NFN
        if((strncmp("call", (char*)pr->comp[i]+skip, 4) && strncmp("(call", (char*)pr->comp[i]+skip, 5)) || call_slash)
        {
#endif
            len += sprintf(buf + len, "/");
#ifdef USE_NFN
        }else{
            len += sprintf(buf + len, " ");
        }
#endif

        for (j = skip; j < pr->complen[i]; j++) {
            char c = pr->comp[i][j];
            char *fmt;
            fmt = (c < 0x20 || c == 0x7f
                            || (escape_components && c == '/' )) ?
#ifdef CCNL_ARDUINO
                  (char*)PSTR("%%%02x") : (char*)PSTR("%c");
            len += sprintf_P(buf + len, fmt, c);
#else
                  (char *) "%%%02x" : (char *) "%c";
            len += sprintf(buf + len, fmt, c);
#endif
            if(len > PREFIX_BUFSIZE) {
                DEBUGMSG(ERROR, "BUFSIZE SMALLER THAN OUTPUT LEN");
                break;
            }
        }
    }

#ifdef USE_NFN
    if (pr->nfnflags)
        len += sprintf(buf + len, "]");
#endif

    buf[len] = '\0';

    return buf;
}

#endif // CCNL_LINUXKERNEL

// ----------------------------------------------------------------------

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs;

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillInterest(name, NULL, tmp, CCNL_MAX_PACKET_SIZE);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = ccnl_ccntlv_prependInterestWithHdr(name, &offs, tmp);
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependInterestWithHdr(name, &offs, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependRequest(name, NULL, &offs, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_prependInterest(name, -1, nonce, &offs, tmp);
        break;
#endif
    default:
        break;
    }

    if (len > 0)
        buf = ccnl_buf_new(tmp + offs, len);
    ccnl_free(tmp);

    return buf;
}

struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, contentpos = 0, offs;

    char *s = NULL;
    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
             (s = ccnl_prefix_to_path(name)), paylen);
    ccnl_free(s);

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, payload, paylen, &contentpos, tmp);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV: {
        unsigned int lcn = 0; // lastchunknum
        len = ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, &lcn,
                                                &contentpos, &offs, tmp);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependContentWithHdr(name, payload, paylen,
                                                NULL, // lastchunknum
                                                &offs, &contentpos, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependReply(name, payload, paylen,
                                         &offs, &contentpos, NULL, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_prependContent(name, payload, paylen,
                                         &contentpos, NULL, &offs, tmp);
        break;
#endif
    default:
        break;
    }

    if (len) {
        buf = ccnl_buf_new(tmp + offs, len);
        if (payoffset)
            *payoffset = contentpos;
    }
    ccnl_free(tmp);

    return buf;
}

#endif // NEEDS_PACKET_CRAFTING
// eof
