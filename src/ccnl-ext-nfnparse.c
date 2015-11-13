/*
 * @f ccnl-ext-nfnparse.c
 * @b CCN lite, parsing support for named-function extension
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-11-08 extracted from ccnl-core-utils.c, can be usefule for apps too
 */

#define LAMBDACHAR '@'

#define term_is_var(t)     (!(t)->m)
#define term_is_app(t)     ((t)->m && (t)->n)
#define term_is_lambda(t)  (!(t)->n)

char*
ccnl_lambdaParseVar(char **cpp)
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

char*
_ccnl_lambdaTermToStr(char *buf, unsigned int *buflen, unsigned int *totalLen, struct ccnl_lambdaTerm_s *t, char last)
{
    if (t->v && t->m) { // Lambda (sequence)
        ccnl_snprintf(&buf, buflen, totalLen, "(%c%s", LAMBDACHAR, t->v);
        buf = _ccnl_lambdaTermToStr(buf, buflen, totalLen, t->m, 'a');
        ccnl_snprintf(&buf, buflen, totalLen, ")");
    } else if (t->v) { // (single) variable
        if (isalnum(last))
            ccnl_snprintf(&buf, buflen, totalLen, " ");
        ccnl_snprintf(&buf, buflen, totalLen, "%s", t->v);
    } else if (t->n->v && !t->n->m) {
        buf = _ccnl_lambdaTermToStr(buf, buflen, totalLen, t->m, last);
        buf = _ccnl_lambdaTermToStr(buf, buflen, totalLen, t->n, 'a');
    } else {
        buf = _ccnl_lambdaTermToStr(buf, buflen, totalLen, t->m, last);
        ccnl_snprintf(&buf, buflen, totalLen, " (");
        buf = _ccnl_lambdaTermToStr(buf, buflen, totalLen, t->n, '(');
        ccnl_snprintf(&buf, buflen, totalLen, ")");
    }

    return buf;
}

int
ccnl_lambdaTermToStr(char *buf, unsigned int buflen, struct ccnl_lambdaTerm_s *t, char last)
{
    unsigned int remLen = buflen, totalLen = 0;
    buf = _ccnl_lambdaTermToStr(buf, &remLen, &totalLen, t, last);

    if (!buf) {
        DEBUGMSG(ERROR, "An encoding error occured while transforming lambdaTerm %p to string.\n",
                 (void*) t);
        return -2;
    }

    if (totalLen >= buflen) {
        DEBUGMSG(ERROR, "Buffer has not enough capacity to store lambdaTerm as string. Available: %u, needed: %u.\n",
                 buflen, totalLen+1);
        return -1;
    }

    return totalLen;
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
    return ccnl_URItoComponents(compVector, NULL, str);
}

// eof
