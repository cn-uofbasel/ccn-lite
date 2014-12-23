/*
 * @f ccnl-core-util.c
 * @b CCN lite, common utility procedures (used by utils as well as relays)
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
 * 2014-06-18 created
 */

#ifndef CCNL_CORE_UTIL_H
#define CCNL_CORE_UTIL_H

#include "ccnl-core.h"

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
    if (su->sa.sa_family == AF_INET)
        return su->ip4.sin_addr.s_addr == htonl(0x7f000001);
    return 0;
}

int
ccnl_str2suite(char *cp)
{
#ifdef USE_SUITE_CCNB
    if (!strcmp(cp, "ccnb"))
        return CCNL_SUITE_CCNB;
#endif
#ifdef USE_SUITE_CCNTLV
    if (!strcmp(cp, "ccnx2014"))
        return CCNL_SUITE_CCNTLV;
#endif
#ifdef USE_SUITE_IOTTLV
    if (!strcmp(cp, "iot2014"))
        return CCNL_SUITE_IOTTLV;
#endif
#ifdef USE_SUITE_NDNTLV
    if (!strcmp(cp, "ndn2013"))
        return CCNL_SUITE_NDNTLV;
#endif
    return -1;
}

char*
ccnl_suite2str(int suite)
{
#ifdef USE_SUITE_CCNB
    if (suite == CCNL_SUITE_CCNB)
        return "ccnb";
#endif
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        return "ccnx2014";
#endif
#ifdef USE_SUITE_IOTTLV
    if (suite == CCNL_SUITE_IOTTLV)
        return "iot2014";
#endif
#ifdef USE_SUITE_NDNTLV
    if (suite == CCNL_SUITE_NDNTLV)
        return "ndn2013";
#endif
    return "?";
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

    DEBUGMSG(TRACE, "ccnl_pkt_prependComponent(%d, %s)\n", suite, src);

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

    DEBUGMSG(TRACE, "ccnl_URItoPrefix(suite=%s, uri=%s, nfn=%s)\n",
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
    
    p->bytes = ccnl_malloc(len);
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
        p->chunknum = ccnl_malloc(sizeof(int));
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
    p->bytes = ccnl_malloc(len);
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
        p->chunknum = ccnl_malloc(sizeof(int));
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
      DEBUGMSG(WARNING, "addChunkNum is only implemented for "
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
            if(prefix->chunknum == NULL) {
                prefix->chunknum = (unsigned int*) ccnl_malloc(sizeof(int));
            }
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
            if(prefix->chunknum == NULL) {
                prefix->chunknum = (unsigned int*) ccnl_malloc(sizeof(int));
            }
            *prefix->chunknum = chunknum;
        }
        break;
#endif

        default:
            DEBUGMSG(WARNING,
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

    while (!ccnl_switch_dehead(&data, &len, &enc))
        suite = ccnl_enc2suite(enc);
    if (skip)
        *skip = data - olddata;
    if (suite >= 0)
        return suite;

#ifdef USE_SUITE_CCNB
    if (*data == 0x01 || *data == 0x04)
        return CCNL_SUITE_CCNB;
#endif

#ifdef USE_SUITE_CCNTLV
    if (data[0] == CCNX_TLV_V0 && len > 1) {
        if (data[1] == CCNX_PT_Interest ||
            data[1] == CCNX_PT_Data ||
            data[1] == CCNX_PT_NACK) 
            return CCNL_SUITE_CCNTLV;
    } 
#endif

#ifdef USE_SUITE_NDNTLV
    if (*data == NDN_TLV_Interest || *data == NDN_TLV_Data)
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

// ----------------------------------------------------------------------

char*
ccnl_addr2ascii(sockunion *su)
{
    static char result[130];

    switch (su->sa.sa_family) {
#ifdef USE_ETHERNET
    case AF_PACKET: {
        struct sockaddr_ll *ll = &su->eth;
        strcpy(result, eth2ascii(ll->sll_addr));
        sprintf(result+strlen(result), "/0x%04x",
            ntohs(ll->sll_protocol));
        return result;
    }
#endif
    case AF_INET:
        sprintf(result, "%s/%d", inet_ntoa(su->ip4.sin_addr),
                ntohs(su->ip4.sin_port));
        return result;
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        strcpy(result, su->ux.sun_path);
        return result;
#endif
    default:
        break;
    }

    return NULL;
}

// ----------------------------------------------------------------------

#ifndef CCNL_LINUXKERNEL

static char *prefix_buf1;
static char *prefix_buf2;
static char *buf;

char*
ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip,
                             int escape_components, int call_slash)
{
    int len = 0, i, j;

    if (!pr)
        return NULL;

    if (!buf) {
        struct ccnl_buf_s *b;
        b = ccnl_buf_new(NULL, 2048);
        ccnl_core_addToCleanup(b);
        prefix_buf1 = (char*) b->data;
        b = ccnl_buf_new(NULL, 2048);
        ccnl_core_addToCleanup(b);
        prefix_buf2 = (char*) b->data;
        buf = prefix_buf1;
    } else if (buf == prefix_buf2)
        buf = prefix_buf1;
    else
        buf = prefix_buf2;

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

#if defined(USE_SUITE_CCNTLV) && defined(USE_NFN)
    // In the future it is possibly helpful to see the type information in the logging output
    // However, this does not work with NFN because it uses this function to create the names in NFN expressions
    // resulting in CCNTLV type information names within expressions.
    if (pr->suite == CCNL_SUITE_CCNTLV && ccntlv_skip)
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
            if (c < 0x20 || c == 0x7f || (escape_components && c == '/' ))
                len += sprintf(buf + len, "%%%02x", c);
            else 
                len += sprintf(buf + len, "%c", c);
        }
    }

#ifdef USE_NFN
    if (pr->nfnflags)
        len += sprintf(buf + len, "]");
#endif

    buf[len] = '\0';

    return buf;
}

char*
ccnl_prefix_to_path(struct ccnl_prefix_s *pr){
    return ccnl_prefix_to_path_detailed(pr, 1, 0, 0);
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs;

    tmp = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
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

    DEBUGMSG(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
             ccnl_prefix_to_path(name), paylen);

    tmp = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, payload, paylen, &contentpos, tmp);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, 
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
                                         &offs, &contentpos, NULL, tmp);
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

#endif // CCNL_LINUXKERNEL

#endif // CCNL_CORE_UTIL_H
// eof
