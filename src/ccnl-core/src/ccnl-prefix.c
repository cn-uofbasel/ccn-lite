/*
 * @f ccnl-prefix.c
 * @b CCN lite, core CCNx protocol logic
 *
 * Copyright (C) 2011-18 University of Basel
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
 * 2017-06-16 created
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <ccnl-os-time.h>
#include <ccnl-pkt-util.h>

#include "ccnl-defs.h"
#include "ccnl-pkt.h"
#include "ccnl-prefix.h"
#include "ccnl-malloc.h"
#include "ccnl-content.h"

#include "ccnl-logging.h"

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
        ccnl_prefix_free(p);
        return NULL;
    }
    p->compcnt = cnt;
    p->suite = suite;
    p->chunknum = NULL;

    return p;
}

void
ccnl_prefix_free(struct ccnl_prefix_s *p)
{
    ccnl_free(p->bytes);
    ccnl_free(p->comp);
    ccnl_free(p->complen);
    ccnl_free(p->chunknum);
    ccnl_free(p);
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
    p->internum = prefix->internum;
#endif

    for (i = 0, len = 0; i < prefix->compcnt; i++)
        len += prefix->complen[i];
    p->bytes = (unsigned char*) ccnl_malloc(len);
    if (!p->bytes) {
        ccnl_prefix_free(p);
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

// TODO: move to a util file?
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
             ccnl_suite2str(suite), uri, (nfnexpr != NULL) ? nfnexpr : "none");

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
        ccnl_prefix_free(p);
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
            {DEBUGMSG(VERBOSE, "1a\n"); goto done;}
        if (pfx->chunknum || nam->chunknum) {
            if (!pfx->chunknum || !nam->chunknum)
                {DEBUGMSG(VERBOSE, "1b\n"); goto done;}
            if (*pfx->chunknum != *nam->chunknum)
                {DEBUGMSG(VERBOSE, "1c\n"); goto done;}
        }
    
#ifdef USE_NFN
        if (nam->nfnflags != pfx->nfnflags) {
            DEBUGMSG(VERBOSE, "1d\n"); 
            goto done;
        }
        if (nam->internum != pfx->internum) {
            DEBUGMSG(VERBOSE, "1e\n"); 
            goto done;
        }
#endif // USE_NFN
    }

#ifdef USE_NFN
    if (mode == CMP_MATCH) {
        if ((nam->nfnflags & CCNL_PREFIX_INTERMEDIATE) != (pfx->nfnflags & CCNL_PREFIX_INTERMEDIATE)) {
            DEBUGMSG(VERBOSE, "2a\n");
            goto done;
        }
        if (nam->internum != pfx->internum) {
            DEBUGMSG(VERBOSE, "2b\n"); 
            goto done;
        }
    }
#endif


    for (i = 0; i < plen && i < nam->compcnt; ++i) {
        comp = i < pfx->compcnt ? pfx->comp[i] : md;
        clen = i < pfx->compcnt ? pfx->complen[i] : 32; // SHA256_DIGEST_LEN
        if (clen != nam->complen[i] || memcmp(comp, nam->comp[i], nam->complen[i])) {
            rc = mode == CMP_EXACT ? -1 : i;
            {DEBUGMSG(VERBOSE, "3: %i\n", i); goto done;}
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

#endif // NEEDS_PREFIX_MATCHING


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
    // len += sprintf(buf + len, "cmpcnt: %i|", pr->compcnt);
    if (pr->nfnflags & CCNL_PREFIX_NFN) {
        len += sprintf(buf + len, "nfn");
    }
    if (pr->nfnflags & CCNL_PREFIX_KEEPALIVE) {
        len += sprintf(buf + len, ":alive");
    }
    if (pr->nfnflags & CCNL_PREFIX_INTERMEDIATE) {
        len += sprintf(buf + len, ":intermediate");
        len += sprintf(buf + len, ":%i", pr->internum);
    }
    if (pr->nfnflags) {
        len += sprintf(buf + len, "[");
    }
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

#else // CCNL_LINUXKERNEL

char*
ccnl_prefix_to_path(struct ccnl_prefix_s *pr)
{
    static char prefix_buf[4096];
    int len= 0, i;

    if (!pr)
        return NULL;
    for (i = 0; i < pr->compcnt; i++) {
        if(!strncmp("call", (char*)pr->comp[i], 4) && strncmp((char*)pr->comp[pr->compcnt-1], "NFN", 3))
            len += sprintf(prefix_buf + len, "%.*s", pr->complen[i], pr->comp[i]);
        else
            len += sprintf(prefix_buf + len, "/%.*s", pr->complen[i], pr->comp[i]);
    }
    prefix_buf[len] = '\0';
    return prefix_buf;
}

#endif // CCNL_LINUXKERNEL
