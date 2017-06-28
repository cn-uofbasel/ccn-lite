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
    return ccnl_URItoComponents(compVector, NULL, str);
}

// eof
