/*
 * @f ccnl-util.c
 * @b CCN lite, common utility procedures for applications (not the relays)
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

#include "pkt-ccnb-enc.c"
#include "pkt-ccntlv-enc.c"
#include "pkt-ndntlv-enc.c"

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
ccnl_URItoComponents(char **compVector, char *uri)
{
    int i, len;

    if (*uri == '/')
        uri++;

    for (i = 0; *uri && i < (CCNL_MAX_NAME_COMP - 1); i++) {
        compVector[i] = uri;
        while (*uri && *uri != '/')
            uri++;
        if (*uri) {
            *uri = '\0';
            uri++;
        }
        len = unescape_component(compVector[i]);
        compVector[i][len] = '\0';
    }
    compVector[i] = NULL;

    return i;
}

struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, char *nfnexpr)
{
    struct ccnl_prefix_s *p;
    char *compvect[CCNL_MAX_NAME_COMP];
    int cnt, i, len = 0;

    p =  ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
        return NULL;

    if (strlen(uri))
        cnt = ccnl_URItoComponents(compvect, uri);
    else
        cnt = 0;
    p->compcnt = cnt;
    p->suite = suite;

    if (nfnexpr && strlen(nfnexpr) > 0) {
        p->compcnt += 1;
        len = strlen(nfnexpr);
    }

    p->comp = ccnl_malloc(p->compcnt * sizeof(char*));
    p->complen = ccnl_malloc(p->compcnt * sizeof(int));
    if (!p->comp || !p->complen) {
        free_prefix(p);
        return NULL;
    }
    for (i = 0; i < cnt; i++) {
        p->complen[i] = strlen(compvect[i]);
        len += p->complen[i];
    }
    
    p->bytes = ccnl_malloc(len);
    if (!p->bytes) {
        free_prefix(p);
        return NULL;
    }
    for (i = 0, len = 0; i < cnt; i++) {
        p->comp[i] = p->bytes + len;
        memcpy(p->comp[i], compvect[i], p->complen[i]);
        len += p->complen[i];
    }

    if (nfnexpr) {
        if (strlen(nfnexpr) > 0) {
            p->comp[i] = p->bytes + len;
            p->complen[i] = strlen(nfnexpr);
            memcpy(p->comp[i], nfnexpr, p->complen[i]);
            len += p->complen[i];
        }
#ifdef USE_NFN
        p->nfnflags |= CCNL_PREFIX_NFN;
#endif
    }

    // realloc path ...

    return p;
}

int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src)
{
    int len = 0;

//    printf("ccnl_pkt_mkComponent(%d, %s)\n", suite, src);

    switch (suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = strlen(src);
        *(unsigned short*)dst = htons(CCNX_TLV_N_NameSegment);
        dst += sizeof(unsigned short);
        *(unsigned short*)dst = len;
        dst += sizeof(unsigned short);
        memcpy(dst, src, len);
        len += 2*sizeof(unsigned short);
        break;
#endif
    default:
        len = strlen(src);
        memcpy(dst, src, len);
        break;
    }
    return len;
}

struct ccnl_prefix_s*
ccnl_prefix_dup(struct ccnl_prefix_s *prefix)
{
    int i = 0, len;
    struct ccnl_prefix_s *p;

    p = ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    p->compcnt = prefix->compcnt;
    p->suite = prefix->suite;
#ifdef USE_NFN
    p->nfnflags = prefix->nfnflags;
#endif

    p->complen = ccnl_malloc(prefix->compcnt * sizeof(int));
    p->comp = ccnl_malloc(prefix->compcnt * sizeof(char*));

    for (i = 0, len = 0; i < prefix->compcnt; i++)
        len += prefix->complen[i];
    p->bytes = ccnl_malloc(len);
    
    for (i = 0, len = 0; i < prefix->compcnt; i++) {
        p->complen[i] = prefix->complen[i];
        p->comp[i] = p->bytes + len;
        memcpy(p->bytes + len, prefix->comp[i], p->complen[i]);
        len += p->complen[i];
    }

    return p;
}

// ----------------------------------------------------------------------

int
ccnl_pkt2suite(unsigned char *data, int len)
{
    if (len <= 0)
        return -1;

#ifdef USE_SUITE_CCNB
    if (*data == 0x01 || *data == 0x04)
        return CCNL_SUITE_CCNB;
#endif

#ifdef USE_SUITE_CCNTLV
    if (data[0] == CCNX_TLV_V0 && len > 1) {
        if (data[1] == CCNX_TLV_TL_Interest ||
            data[1] == CCNX_TLV_TL_Object)
            return CCNL_SUITE_CCNTLV;
    }
#endif

#ifdef USE_SUITE_NDNTLV
    if (*data == NDN_TLV_Interest || *data == NDN_TLV_Data)
        return CCNL_SUITE_NDNTLV;
#endif

#ifdef USE_SUITE_LOCALRPC
    if (*data == 0x80)
        return CCNL_SUITE_LOCALRPC;
#endif

    return -1;
}

// ----------------------------------------------------------------------

char*
ccnl_addr2ascii(sockunion *su)
{
    static char result[130];

    switch (su->sa.sa_family) {
#ifdef USE_ETHERNET
    case AF_PACKET:
    {
        struct sockaddr_ll *ll = &su->eth;
        strcpy(result, eth2ascii(ll->sll_addr));
        sprintf(result+strlen(result), "/0x%04x",
            ntohs(ll->sll_protocol));
    }
    return result;
#endif
    case AF_INET:
        sprintf(result, "%s/%d", inet_ntoa(su->ip4.sin_addr),
                ntohs(su->ip4.sin_port));
        return result;
//      return inet_ntoa(SA_CAST_IN(sa)->sin_addr);
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

#ifndef CCNL_LINUXKERNEL

static char *prefix_buf1;
static char *prefix_buf2;
static char *buf;

char*
ccnl_prefix_to_path(struct ccnl_prefix_s *pr)
{
    int len = 0, i;

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

    for (i = 0; i < pr->compcnt; i++) {
    int skip = 0;
    if (pr->suite == CCNL_SUITE_CCNTLV) {
        if (ntohs(*(unsigned short*)(pr->comp[i])) != 1) {// !CCNX_TLV_N_UTF8
            len += sprintf(buf + len, "/%%x%02x%02x%.*s",
                       pr->comp[i][0], pr->comp[i][1],
                       pr->complen[i]-4, pr->comp[i]+4);
            continue;
        }
        skip = 4;
    }
#ifdef USE_NFN
    if (pr->compcnt == 1 && (pr->nfnflags & CCNL_PREFIX_NFN) &&
                    !strncmp("call", (char*)pr->comp[i], 4))
        len += sprintf(buf + len, "%.*s",
               pr->complen[i]-skip, pr->comp[i]+skip);
    else
#endif
        len += sprintf(buf + len, "/%.*s",
               pr->complen[i]-skip, pr->comp[i]+skip);
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

#define LAMBDACHAR '@'

#define term_is_var(t)     (!(t)->m)
#define term_is_app(t)     ((t)->m && (t)->n)
#define term_is_lambda(t)  (!(t)->n)

char*
ccnl_lambdaParseVar(char **cpp)
{
    char *p;
    int len;

    p = *cpp;
    if (*p && *p == '\'')
        p++;
    while (*p && (isalnum(*p) || *p == '_' || *p == '=' || *p == '/' || *p == '.'))
	   p++;
    len = p - *cpp;
    p = ccnl_malloc(len+1);
    if (!p)
        return 0;
    memcpy(p, *cpp, len);
    p[len] = '\0';
    *cpp += len;
    return p;
}

struct ccnl_lambdaTerm_s*
ccnl_lambdaStrToTerm(int lev, char **cp, int (*prt)(char* fmt, ...))
{
/* t = (v, m, n)

   var:     v!=0, m=0, n=0
   app:     v=0, m=f, n=arg
   lambda:  v!=0, m=body, n=0
 */
    struct ccnl_lambdaTerm_s *t = 0, *s, *u;

    while (**cp) {
        while (isspace(**cp))
            *cp += 1;

    //  myprintf(stderr, "parseKRIVINE %d %s\n", lev, *cp);

        if (**cp == ')')
            return t;
        if (**cp == '(') {
            *cp += 1;
            s = ccnl_lambdaStrToTerm(lev+1, cp, prt);
            if (!s)
                return 0;
            if (**cp != ')') {
                if (prt) {
                    prt("parseKRIVINE error: missing )\n");
                }
                return 0;
            } else {
                *cp += 1;
            }
        } else if (**cp == LAMBDACHAR) {
            *cp += 1;
            s = ccnl_calloc(1, sizeof(*s));
            s->v = ccnl_lambdaParseVar(cp);
            s->m = ccnl_lambdaStrToTerm(lev+1, cp, prt);
    //      printKRIVINE(dummybuf, s->m, 0);
    //      printf("  after lambda: /%s %s --> <%s>\n", s->v, dummybuf, *cp);
        } else {
            s = ccnl_calloc(1, sizeof(*s));
            s->v = ccnl_lambdaParseVar(cp);
    //      printf("  var: <%s>\n", s->v);
        }
        if (t) {
    //      printKRIVINE(dummybuf, t, 0);
    //      printf("  old term: <%s>\n", dummybuf);
            u = ccnl_calloc(1, sizeof(*u));
            u->m = t;
            u->n = s;
            t = u;
        } else {
            t = s;
        }
//  printKRIVINE(dummybuf, t, 0);
//  printf("  new term: <%s>\n", dummybuf);
    }
//    printKRIVINE(dummybuf, t, 0);
//    printf("  we return <%s>\n", dummybuf);
    return t;
}

int
ccnl_lambdaTermToStr(char *cfg, struct ccnl_lambdaTerm_s *t, char last)
{
    int len = 0;

    if (t->v && t->m) { // Lambda (sequence)
        len += sprintf(cfg + len, "(%c%s", LAMBDACHAR, t->v);
        len += ccnl_lambdaTermToStr(cfg + len, t->m, 'a');
        len += sprintf(cfg + len, ")");
        return len;
    }
    if (t->v) { // (single) variable
        if (isalnum(last))
            len += sprintf(cfg + len, " %s", t->v);
        else
            len += sprintf(cfg + len, "%s", t->v);
        return len;
    }
    // application (sequence)
#ifdef CORRECT_PARENTHESES
    len += sprintf(cfg + len, "(");
    len += printKRIVINE(cfg + len, t->m, '(');
    len += printKRIVINE(cfg + len, t->n, 'a');
    len += sprintf(cfg + len, ")");
#else
    if (t->n->v && !t->n->m) {
        len += ccnl_lambdaTermToStr(cfg + len, t->m, last);
        len += ccnl_lambdaTermToStr(cfg + len, t->n, 'a');
    } else {
        len += ccnl_lambdaTermToStr(cfg + len, t->m, last);
        len += sprintf(cfg + len, " (");
        len += ccnl_lambdaTermToStr(cfg + len, t->n, '(');
        len += sprintf(cfg + len, ")");
    }
#endif
    return len;
}

void
ccnl_lambdaFreeTerm(struct ccnl_lambdaTerm_s *t)
{
    if (t) {
        ccnl_free(t->v);
        ccnl_lambdaFreeTerm(t->m);
        ccnl_lambdaFreeTerm(t->n);
        ccnl_free(t);
    }
}

int
ccnl_lambdaStrToComponents(char **compVector, char *str)
{
    return ccnl_URItoComponents(compVector, str);
}

// ----------------------------------------------------------------------

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
        len = ccnl_ccntlv_fillInterestWithHdr(name, &offs, tmp);
    break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_fillInterest(name, -1, NULL, &offs, tmp);
    break;
#endif
    default:
    break;
    }

    if (len)
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
        len = ccnl_ccntlv_fillContentWithHdr(name, payload, paylen, 
                                             NULL, NULL, // chunknum/lastchunknum
                                             &offs, &contentpos, tmp);
    break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_fillContent(name, payload, paylen,
                                      &offs, &contentpos, NULL, 0, tmp);
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

// eof
