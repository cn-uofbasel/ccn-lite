/*
 * @f ccnl-util.c
 * @b CCN lite, common utility procedures for applications (not the relays)
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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

#ifndef CCNL_UTIL_C
#define CCNL_UTIL_C
#pragma once

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
ccnl_URItoPrefix(char* uri)
{
    struct ccnl_prefix_s *prefix = ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    char *compvect[CCNL_MAX_NAME_COMP];
    int i, len;

    if (!prefix)
	return NULL;
    prefix->compcnt = ccnl_URItoComponents(compvect, uri);
    prefix->comp = ccnl_malloc(prefix->compcnt * sizeof(char*));
    prefix->complen = ccnl_malloc(prefix->compcnt * sizeof(int));
    if (!prefix->comp || !prefix->complen) {
	free_prefix(prefix);
	return NULL;
    }
    for (i = 0, len = 0; i < prefix->compcnt; i++) {
	prefix->complen[i] = strlen(compvect[i]);
	len += prefix->complen[i];
    }
    prefix->path = ccnl_malloc(len);
    if (!prefix->path) {
	free_prefix(prefix);
	return NULL;
    }
    for (i = 0, len = 0; i < prefix->compcnt; i++) {
	prefix->comp[i] = prefix->path + len;
	memcpy(prefix->comp[i], compvect[i], prefix->complen[i]);
	len += prefix->complen[i];
    }

    return prefix;
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
    while (*p && (isalnum(*p) || *p == '_' || *p == '=' || *p == '/'))
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

//	myprintf(stderr, "parseKRIVINE %d %s\n", lev, *cp);

	if (**cp == ')')
	    return t;
	if (**cp == '(') {
	    *cp += 1;
	    s = ccnl_lambdaStrToTerm(lev+1, cp, prt);
	    if (!s)
		return 0;
	    if (**cp != ')') {
		if (prt)
		    prt("parseKRIVINE error: missing )\n");
		return 0;
	    } else
		*cp += 1;
	} else if (**cp == LAMBDACHAR) {
	    *cp += 1;
	    s = ccnl_calloc(1, sizeof(*s));
	    s->v = ccnl_lambdaParseVar(cp);
	    s->m = ccnl_lambdaStrToTerm(lev+1, cp, prt);
//	    printKRIVINE(dummybuf, s->m, 0);
//	    printf("  after lambda: /%s %s --> <%s>\n", s->v, dummybuf, *cp);
	} else {
	    s = ccnl_calloc(1, sizeof(*s));
	    s->v = ccnl_lambdaParseVar(cp);
//	    printf("  var: <%s>\n", s->v);
	}
	if (t) {
//	    printKRIVINE(dummybuf, t, 0);
//	    printf("  old term: <%s>\n", dummybuf);
	    u = ccnl_calloc(1, sizeof(*u));
	    u->m = t;
	    u->n = s;
	    t = u;
	} else
	    t = s;
//	printKRIVINE(dummybuf, t, 0);
//	printf("  new term: <%s>\n", dummybuf);
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
/*
    if (term_is_var(t) || term_is_lambda)
	ccnl_free(t->v);
    if (term_is_app(t) || term_is_lambda(t))
	ccnl_lambdaFreeTerm(t->m);
    if (term_is_app(t))
	ccnl_lambdaFreeTerm(t->n);
    ccnl_free(t);
*/
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

int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src)
{
    int len = 0;

//    printf("ccnl_pkt_mkComponent(%d, %s)\n", suite, src);

    switch (suite) {
#ifdef USE_CCNL_CCNTLV
    case CCNL_SUITE_CCNTLV:
	len = strlen(src);
	*(unsigned short*)dst = htons(CCNX_TLV_N_UTF8);
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

// ----------------------------------------------------------------------

#endif //CCNL_UTIL_C
