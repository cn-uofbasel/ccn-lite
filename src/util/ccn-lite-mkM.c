/*
 * @f util/ccn-lite-mkM.c
 * @b make a (CCNx) manifest object
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-11-16  created
 */

#define USE_SUITE_CCNTLV
// #define USE_SUITE_CISTLV
// #define USE_SUITE_IOTTLV
// #define USE_SUITE_NDNTLV

#define USE_CCNxMANIFEST
#define USE_HMAC256
#define USE_NAMELESS

#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.c"

struct list_s {
    char *var;
    struct list_s *first;
    struct list_s *rest;
};

// ----------------------------------------------------------------------

char*
ccnl_parseVar(char **cpp)
{
    char *p;
    int len, string = 0;

    p = *cpp;
    if (*p && *p == '\''){ // Parse a String between ''
        string = 1;
        p++;
        while(*p != '\''){
            p++;
        }
    }
    else{
        while (*p && (isalnum(*p) || *p == '_' || *p == '=' || *p == '/' || *p == '.')){
           p++;
        }
    }
    len = p - *cpp + string;
    p = ccnl_malloc(len+1);
    if (!p)
        return 0;
    memcpy(p, *cpp, len);
    p[len] = '\0';
    *cpp += len;
    return p;
}

void
ccnl_printExpr(struct list_s *t, char last, char *out)
{
    if (!t)
        return;
    if (t->var) {
        strcat(out, t->var);
        return;
    }
    strcat(out, "(");
    last = '(';
    do {
        if (last != '(')
            strcat(out, " ");
        out += strlen(out);
        ccnl_printExpr(t->first, last, out);
        out += strlen(out);
        last = 'a';
        t = t->rest;
    } while (t);
    strcat(out, ")");
    return;
}


struct list_s*
ccnl_strToList(int lev, char **cp)
{
    struct list_s *lst = 0, *s, *u, **r = &lst;

    while (**cp) {
        while (isspace(**cp))
            *cp += 1;
        if (**cp == ')') {
            return lst;
        }
        if (**cp == '(') {
            *cp += 1;
            s = ccnl_strToList(lev+1, cp);
            if (!s)
                return 0;
            if (**cp != ')') {
                DEBUGMSG(FATAL, "parse error: missing )\n");
                return 0;
            } else {
                *cp += 1;
            }
        } else {
            s = ccnl_calloc(1, sizeof(*s));
            s->var = ccnl_parseVar(cp);
        }
        u = ccnl_calloc(1, sizeof(*u));
        u->first = s;
        *r = u;
        r = &u->rest;
    }

    return lst;
}

// ----------------------------------------------------------------------

void
hash2digest(unsigned char *md, char *cp)
{
    int cnt;
    for (cnt = 0; cnt < SHA256_DIGEST_LENGTH; cnt++, cp += 2) {
        md[cnt] = hex2int(cp[0]) * 16 + hex2int(cp[1]);
    }
}


// ----------------------------------------------------------------------

int
emit(struct list_s *lst, unsigned short len, int *offset, unsigned char *buf)
{
    int typ = -1, len2;
    char *cp = "??", dummy[200];
    struct ccnl_prefix_s *pfx = NULL;

    if (!lst)
        return len;

    DEBUGMSG(TRACE, "   emit starts: len=%d\n", len);
    if (lst->var) { // atomic
        switch(lst->var[0]) {
        case 'a':
            cp = "T=about (metadata)";
            typ = CCNX_MANIFEST_HG_METADATA;
            break;
        case 'b':
            cp = "T=block size";
            typ = CCNX_MANIFEST_MT_BLOCKSIZE;
            break;
        case 'c': // can be removed for manifests
            cp = "T=content";
            typ = CCNX_TLV_TL_Object;
            break;
        case 'd':
            cp = "T=data hash ptr";
            typ = CCNX_MANIFEST_HG_PTR2DATA;
            break;
        case 'g':
            cp = "T=hash pointer group";
            typ = CCNX_MANIFEST_HASHGROUP;
            break;
        case 'l':
            cp = "T=locator";
            typ = CCNX_MANIFEST_MT_LOCATOR;
            break;
        case 'x':
            cp = "T=xternal metadata URI";
            typ = CCNX_MANIFEST_MT_EXTERNALMETADATA;
            break;
        case 'm':
            cp = "T=manifest";
            typ = CCNX_TLV_TL_Manifest;
            break;
        case 'n':
            cp = "T=name";
            typ = CCNX_TLV_M_Name;
            break;
        case 'p': // can be removed for manifests
            cp = "T=payload";
            typ = CCNX_TLV_M_Payload;
            break;
        case 's':
            cp = "T=data size";
            typ = CCNX_MANIFEST_MT_OVERALLDATASIZE;
            break;
        case 't':
            cp = "T=tree hash ptr";
            typ = CCNX_MANIFEST_HG_PTR2MANIFEST;
            break;
        case '/':
            pfx = ccnl_URItoPrefix(lst->var, CCNL_SUITE_CCNTLV, NULL, NULL);
            cp = ccnl_prefix2path(dummy, sizeof(dummy), pfx);
            len2 = ccnl_ccntlv_prependNameComponents(pfx, offset, buf);
            break;
        case '0':
            cp = "hash value";
            *offset -= SHA256_DIGEST_LENGTH;
            hash2digest(buf + *offset, lst->var + 2); // start after "0x"
            len2 = SHA256_DIGEST_LENGTH;
            break;
        default:
            if (isdigit(lst->var[0])) {
                int v = atoi(lst->var);
                len2 = ccnl_ccntlv_prependUInt((unsigned int)v, offset, buf);
                cp = "int value";
            } else { // treat as a blob
                len2 = strlen(lst->var);
                *offset -= len2;
                memcpy(buf + *offset, lst->var, len2);
            }
            break;
        }
        if (typ >= 0) {
            len2 = ccnl_ccntlv_prependTL((unsigned short) typ, len, offset, buf);
            len += len2;
        } else {
            len = len2;
        }
        DEBUGMSG(DEBUG, "  prepending %s (L=%d), returning %dB\n", cp, len2, len);
        
        return len;
    }

    // non-atomic
    len2 = emit(lst->rest, 0, offset, buf);
    DEBUGMSG(TRACE, "   rest: len2=%d, len=%d\n", len2, len);
    return emit(lst->first, len2, offset, buf) + len;
}


// ----------------------------------------------------------------------

char *progname;

void
usage(int exitval)
{
    fprintf(stderr, "usage: %s [options] m-expr\n"
            "  -s SUITE   (ccnx2015)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
            "\nExamples of manifest expressions:\n"
            "  (mfst (nm /the/m/fst) (grp (dptr 0xSHA256HEX)))\n"
            "  (mfst (grp (dptr 0xSHA256HEX) (tptr 0xSHA256HEX)))\n"
            "  (mfst (grp (abt (loc /loc/tr) (blocksz 1024) (sz 3345)) (dptr 0xSHA256HEX)))\n"
            "\nTags:\n"
            "  a  about (metadata)\n"
            "  b  block size\n"
            "  d  data hash ptr\n"
            "  g  hash pointer group\n"
            "  l  locator\n"
            "  n  name\n"
            "  s  data size\n"
            "  t  tree ptr\n"
            , progname);
    exit(exitval);
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt, packettype = CCNL_SUITE_CCNTLV;
    char *sexpr;
    struct list_s *t;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "hs:v:")) != -1) {
        switch (opt) {
        case 's':
            opt = ccnl_str2suite(optarg);
            if (opt < 0 || opt >= CCNL_SUITE_LAST)
                goto Usage;
            packettype = opt;
            break;
#ifdef USE_LOGGING
        case 'v':
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
            break;
#endif
        case 'h':
        default:
Usage:
            usage(1);
        }
    }

    if (optind >= argc)
        goto Usage;
    sexpr = argv[optind++];
    if (optind < argc)
        goto Usage;

    DEBUGMSG(DEBUG, "Parsing m-expression:\n  <%s>\n", sexpr);

    t = ccnl_strToList(0, &sexpr);
    if (t) {
        unsigned char buf[64*1024];
        char out[2048];
        int offset = sizeof(buf);
        
        DEBUGMSG(INFO, "Parsed expression:\n");
        ccnl_printExpr(t->first, ' ', out);
        DEBUGMSG(INFO, "%s\n", out);

        DEBUGMSG(DEBUG, "start to fill from the end:\n");
        emit(t->first, 0, &offset, buf);
        DEBUGMSG(TRACE, "total number of message bytes = %d\n",
                 (int)(sizeof(buf) - offset));
        ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                    sizeof(buf) - offset, 255, &offset, buf);

        write(1, buf + offset, sizeof(buf) - offset);
    }

    return 0;
}

// eof
