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
ccnl_printExpr(struct list_s *t, char last)
{
    if (!t)
        return;
    if (t->var) {
        printf("%s", t->var);
        return;
    }
    printf("(");
    last = '(';
    do {
        if (last != '(')
            printf(" ");
        ccnl_printExpr(t->first, last);
        last = 'a';
        t = t->rest;
    } while (t);
    printf(")");
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
                printf("parse error: missing )\n");
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

int
emit(struct list_s *t, int *pos, int len)
{
    char *cp = "??";

    if (!t)
        return 0;
    if (t->var) {
        switch(t->var[0]) {
        case 'a':
            cp = "T=about (metadata)"; break;
        case 'b':
            cp = "T=block size"; break;
        case 'd':
            cp = "T=data hash ptr"; break;
        case 'g':
            cp = "T=hash pointer group"; break;
        case 'l':
            cp = "T=locator"; break;
        case 'm':
            cp = "T=manifest"; break;
        case 'n':
            cp = "T=name"; break;
        case 's':
            cp = "T=data size"; break;
        case 't':
            cp = "T=tree hash ptr"; break;
        case '/':
            cp = t->var; len++; break;
        case '0':
            cp = t->var; len++; break;
        default:
            if (isdigit(t->var[0])) {
                cp = t->var; len++;
            }
            break;
        }
        printf("  %s, L=%d\n", cp, len);
        *pos -= 1;
        return len + 1;
    }

    int len2 = emit(t->rest, pos, 0);
    return len + emit(t->first, pos, len2);
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

    printf("Parsing m-expression:\n  <%s>\n", sexpr);

    t = ccnl_strToList(0, &sexpr);
    if (t) {
        int pos = 100;
        printf("Parsed expression:\n");
        ccnl_printExpr(t->first, ' ');
        printf("\n");
        emit(t->first, &pos, 0);
        printf("Position after emit: %d\n", pos);
       
    }

    return 0;
}

// eof
