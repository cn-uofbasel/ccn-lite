/*
 * @f ccnl-core-pfx.c
 * @b CCN lite, procedures regarding names/prefixes
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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
 * 2015-08-25 created
 */

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

struct ccnl_prefix_s*
ccnl_prefix_dup(struct ccnl_prefix_s *prefix)
{
    int i = 0, len;
    struct ccnl_prefix_s *p;

    if (!prefix)
        return NULL;
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
        p->chunknum = ccnl_malloc(sizeof(int));
        *p->chunknum = *prefix->chunknum;
    }

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

    DEBUGMSG_CPFX(TRACE, "ccnl_URItoPrefix(suite=%s, uri=%s, nfn=%s)\n",
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
        p->chunknum = ccnl_malloc(sizeof(int));
        *p->chunknum = *chunknum;
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
      DEBUGMSG_CPFX(WARNING, "addChunkNum is only implemented for "
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
            prefix->chunknum = ccnl_malloc(sizeof(int));
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
            prefix->chunknum = ccnl_malloc(sizeof(int));
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
            prefix->chunknum = ccnl_malloc(sizeof(int));
            *prefix->chunknum = chunknum;
        }
        break;
#endif

        default:
            DEBUGMSG_CPFX(WARNING,
                     "add chunk number not implemented for suite %d\n",
                     prefix->suite);
            return -1;
    }

    return 0;
}

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
#ifdef USE_LOGGING
    char prefixBuf1[CCNL_PREFIX_BUFSIZE], prefixBuf2[CCNL_PREFIX_BUFSIZE];
#endif

    DEBUGMSG_CPFX(VERBOSE, "prefix_cmp(mode=%s) prefix=<%s> of? name=<%s> digest=%p\n",
             ccnl_matchMode2str(mode),
             ccnl_prefix2path(prefixBuf1, CCNL_ARRAY_SIZE(prefixBuf1), pfx),
             ccnl_prefix2path(prefixBuf2, CCNL_ARRAY_SIZE(prefixBuf2), nam),
             (void*) md);

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
    DEBUGMSG_CPFX(TRACE, "  cmp result: pfxlen=%d cmplen=%d namlen=%d matchlen=%d\n",
             pfx->compcnt, plen, nam->compcnt, rc);
    return rc;
}

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
                  int minsuffix, int maxsuffix, struct ccnl_content_s *c)
{
    unsigned char *md;
#ifdef USE_LOGGING
    char prefixBuf1[CCNL_PREFIX_BUFSIZE], prefixBuf2[CCNL_PREFIX_BUFSIZE];
#endif
    struct ccnl_prefix_s *p = c->pkt->pfx;

    DEBUGMSG_CPFX(VERBOSE, "ccnl_i_prefixof_c prefix=<%s> content=<%s> min=%d max=%d\n",
             ccnl_prefix2path(prefixBuf1, CCNL_ARRAY_SIZE(prefixBuf1), prefix),
             ccnl_prefix2path(prefixBuf2, CCNL_ARRAY_SIZE(prefixBuf2), p),
             minsuffix, maxsuffix);
    if (!prefix || !p)
        return 0;
    // CONFORM: we do prefix match, honour min. and maxsuffix,

    // NON-CONFORM: "Note that to match a ContentObject must satisfy
    // all of the specifications given in the Interest Message."
    // >> CCNL does not honour the exclusion filtering

    if ( (prefix->compcnt + minsuffix) > (p->compcnt + 1) ||
         (prefix->compcnt + maxsuffix) < (p->compcnt + 1)) {
        DEBUGMSG_CPFX(TRACE, "  mismatch in # of components\n");
        return 0;
    }

    md = (prefix->compcnt - p->compcnt == 1) ? compute_ccnx_digest(c->pkt->buf) : NULL;
    return ccnl_prefix_cmp(p, md, prefix, CMP_MATCH) == prefix->compcnt;
}

#endif // #ifdef NEEDS_PREFIX_MATCHING

// ----------------------------------------------------------------------

char*
ccnl_prefix2path(char *buf, unsigned int buflen, struct ccnl_prefix_s *pr)
{
    if (!pr || !pr->compcnt)
        strcpy(buf, "nil");
    else
        ccnl_snprintfPrefixPath(buf, buflen, pr);
    return buf;
}

int
ccnl_snprintfPrefixPathDetailed(char *buf, unsigned int buflen, struct ccnl_prefix_s *pr,
                         int ccntlv_skip, int escape_components, int call_slash)
{
    int i, j;
    char *tmpBuf = buf;
    unsigned int remLen = buflen, totalLen = 0, skip = 0;

    // Conform to snprintf standard
    assert((buf != NULL || buflen == 0) && "buf can be (null) only if buflen is zero");

    if (!pr) {
        int numChars = snprintf(buf, buflen, "%p", (void *) NULL);
        if (numChars < 0) goto encodingError;
        else if ((unsigned int) numChars >= buflen) goto notEnoughCapacity;
        return numChars;
    }

#ifdef USE_NFN
    if (pr->nfnflags & CCNL_PREFIX_NFN)
        ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("nfn"));

    if (pr->nfnflags & CCNL_PREFIX_THUNK)
        ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("thunk"));

    if (pr->nfnflags)
        ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("["));
#endif

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
        if (call_slash
                || (strncmp("call", (char*) pr->comp[i]+skip, 4)
                    && strncmp("(call", (char*) pr->comp[i]+skip, 5))) {
#endif
            ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("/"));
#ifdef USE_NFN
        } else {
            ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR(" "));
        }
#endif

        for (j = skip; j < pr->complen[i]; j++) {
            unsigned char c = pr->comp[i][j];
            if (c < 0x20 || c >= 0x7f || (escape_components && c == '/'))
                ccnl_snprintf(&tmpBuf,&remLen,&totalLen, CONSTSTR("%%%02x"), c);
            else
                ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("%c"), c);
        }
    }

#ifdef USE_NFN
    if (pr->nfnflags) {
        ccnl_snprintf(&tmpBuf, &remLen, &totalLen, CONSTSTR("]"));
    }
#endif

    if (!tmpBuf) goto encodingError;
    if (totalLen >= buflen) goto notEnoughCapacity;
    return totalLen;

notEnoughCapacity:
    DEBUGMSG_CPFX(WARNING, "Buffer has not enough capacity for prefix name. Available: %u, needed: %u\n",
                  buflen, totalLen+1);
    return totalLen;

encodingError:
    DEBUGMSG_CPFX(ERROR, "Encoding error occured while creating path of prefix: %p\n",
                  (void *) pr);

    if (buf && buflen > 0) {
        buf[0] = '\0';
    }
    return -1;
}

// ----------------------------------------------------------------------
