/*
 * krivine.c
 * a "Krivine lambda expression resolver" for CCN
 *
 * (C) 2013 <christian.tschudin@unibas.ch>
 *
 * 2013-05-10 created
 * 2013-05-15 spooky hash fct added. Encoding still uses "hashXXX"
 *            instead of the hash bits (for readability during devl)
 * 2013-06-09 recursion works, tail-recursion too, now we have
 *            a simple read-eval loop
 * 2016-03-14 integration for nfn <christopher.scherb@unibas.ch>
 */

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "ccnl.h"


#ifndef ABSTRACT_MACHINE
#include "krivine-common.c"
#endif

#define CORRECT_PARENTHESES
#define LAMBDA '@'

#ifdef ABSTRACT_MACHINE
// #define USE_SPOOKY

#  define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)

int debug_level;
#endif /*ABSTRACT_MACHINE*/



struct term_s {
    char *v;
    struct term_s *m, *n;
    // if m is 0, we have a var  v
    // is both m and n are not 0, we have an application  (M N)
    // if n is 0, we have a lambda term  /v M
};

struct hash_s {
    int id;
    char *s1;
    char spooky[16];
};

#define term_is_var(t)     (!(t)->m)
#define term_is_app(t)     ((t)->m && (t)->n)
#define term_is_lambda(t)  (!(t)->n)

// ----------------------------------------------------------------------

#ifdef ABSTRACT_MACHINE
struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno, size;
//    char *tstamp;
} *mem;

void*
debug_malloc(int s, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) malloc(s + sizeof(struct mhdr));
    if (!h) return NULL;
    h->next = mem;
    mem = h;
    h->fname = (char *) fn;
    h->lineno = lno;
    h->size = s;
    return ((unsigned char *)h) + sizeof(struct mhdr);
}

void*
debug_calloc(int n, int s, const char *fn, int lno)
{
    void *p = debug_malloc(n * s, fn, lno);
    if (p)
        memset(p, 0, n*s);
    return p;
}

int
debug_unlink(struct mhdr *hdr)
{
    struct mhdr **pp = &mem;

    for (pp = &mem; pp; pp = &((*pp)->next)) {
        if (*pp == hdr) {
            *pp = hdr->next;
            return 0;
        }
        if (!(*pp)->next)
            break;
    }
    return 1;
}

void
debug_free(void *p, const char *fn, int lno)
{
    struct mhdr *h = (struct mhdr *) (((unsigned char *)p) - sizeof(struct mhdr));

    if (!p) {
//      fprintf(stderr, "%s: @@@ memerror - free() of NULL ptr at %s:%d\n",
//         timestamp(), fn, lno);
        return;
    }
    if (debug_unlink(h)) {
        fprintf(stderr,
           "@@@ memerror - free() at %s:%d does not find memory block %p\n",
                fn, lno, p);
        return;
    }
    free(h);
}
#endif /*ABSTRACT MACHINE*/

char*
debug_strdup(char *s, char *fn, int lno)
{
#ifdef ABSTRACT_MACHINE
    char *p = debug_malloc(strlen(s)+1, fn, lno);
#else 
    char *p = debug_malloc(strlen(s)+1, fn, lno , timestamp());
#endif
    if (!p)
	return 0;
    strcpy(p, s);
    return p;
}

#ifdef ABSTRACT_MACHINE
#define malloc(s)   debug_malloc(s, __FILE__, __LINE__)
#define calloc(n,s) debug_calloc(n, s, __FILE__, __LINE__)
#else
#define malloc(s)   debug_malloc(s, __FILE__, __LINE__, timestamp())
#define calloc(n,s) debug_calloc(n, s, __FILE__, __LINE__, timestamp())
#endif
#define free(p)     debug_free(p, __FILE__, __LINE__)
#define strdup(s)   debug_strdup(s, __FILE__, __LINE__)

// ----------------------------------------------------------------------

#define MAX_STEPS 10000
int steps;

#define MAX_HASHES 5000
struct hash_s hashes[MAX_HASHES];
int hcnt;

char dummybuf[2000];

#define MAX_CONTENTSTORE 5000
struct cs_s {
    char *name;
    char *content;
} cs[MAX_CONTENTSTORE];
int cs_cnt;

char *cs_trigger;     // PIT - currently only keep one, instead of a queue
char *cs_in_name;     // currently only one for incoming msgs
// char *cs_in_content;  // currently only one for incoming msgs

char *global_dict;

// ----------------------------------------------------------------------
struct stack_element_s{
    char *content;
    struct stack_element_s *next;
};

void push_to_stack(struct stack_element_s **top, char *content){
    struct stack_element_s *h = malloc(sizeof(struct stack_element_s));
    if(*top != NULL){
        h->next = *top;
    }else{
        h->next = NULL;
    }
    h->content = strdup(content);
    *top = h;
}

char *pop_from_stack(struct stack_element_s **top){
    struct stack_element_s *h = *top;
    if(*top != NULL){
        char *res = h->content;
        if(h->next == NULL){
            *top = NULL;
        }else{
            *top = h->next;
        }
        free(h);
        return res;
    }
    return NULL;
}

// ----------------------------------------------------------------------

int iscontent(char *cp){
	if(cp[0] == '/')
		return 1;
	return 0;
}

char*
parse_var(char **cpp)
{
    char *p;
    int len;

    p = *cpp;
    while (*p && (isalnum(*p) || *p == '_' || *p == '=' || *p == '/'))
	p++;
    len = p - *cpp;
    p = malloc(len+1);
    if (!p)
	return 0;
    memcpy(p, *cpp, len);
    p[len] = '\0';
    *cpp += len;
    return p;
}

struct term_s*
parsePURE(int lev, char **cp)
{
    struct term_s *t = calloc(1, sizeof(*t));

    while (isspace(**cp))
	*cp += 1;

//    if (islower(**cp) || isupper(**cp)) {
    if (isalnum(**cp)) { // islower(**cp) || isupper(**cp)) {
	t->v = parse_var(cp);
	return t;
    }
    if (**cp == LAMBDA) {
	*cp += 1;
	t->v = parse_var(cp);
	t->m = parsePURE(lev+1, cp);
	return t;
    }
    if (**cp != '(') {
	printf("parse error\n");
	return 0;
    }
    *cp += 1;
    t->m = parsePURE(lev+1, cp);
    t->n = parsePURE(lev+1, cp);
    while (isspace(**cp))
	*cp += 1;
    if (**cp != ')') {
	printf("no closing parenthesis\n");
	return 0;
    }
    *cp += 1;
    return t;
}

struct term_s*
parsePURE_term(int lev, char **cp)
{
    struct term_s *t1, *t2, *t3;

    t1 = parsePURE(lev, cp);
    while (isspace(**cp))
	*cp += 1;
    if (**cp == '\0')
	return t1;

    t2 = parsePURE(lev, cp);
    while (isspace(**cp))
	*cp += 1;
    if (**cp != '\0') {
	printf("parse error: <%s>\n", *cp);
	return 0;
    }
    t3 = calloc(1, sizeof(*t3));
    t3->m = t1;
    t3->n = t2;
    return t3;
}

int
sprint_term(char *cfg, struct term_s *t, char last)
{
    int len;

    if (!t)
	return sprintf(cfg, "nil");

    if (!t->m)
	return sprintf(cfg, isalnum(last) ? " %s" : "%s", t->v);
    else if (!t->n) {
	len = sprintf(cfg, "%c%s", LAMBDA, t->v);
//	len = sprintf(cfg, "\\%c", t->v);
	return len + sprint_term(cfg + len, t->m, cfg[len-1]);
    }
    len = sprintf(cfg, "(");
    len += sprint_term(cfg + len, t->m, '(');
    len += sprint_term(cfg + len, t->n, cfg[len-1]);
    return len + sprintf(cfg + len, ")");
}


int printKRIVINE(char *cfg, struct term_s *t, char last);

struct term_s*
parseKRIVINE(int lev, char **cp)
{
/* t = (v, m, n)

   var:     v!=0, m=0, n=0
   app:     v=0, m=f, n=arg
   lambda:  v!=0, m=body, n=0
 */
    struct term_s *t = 0, *s, *u;

    while (**cp) {
	while (isspace(**cp))
	    *cp += 1;

//	printf("parseKRIVINE %d %s\n", lev, *cp);

	if (**cp == ')')
	    return t;
	if (**cp == '(') {
	    *cp += 1;
	    s = parseKRIVINE(lev+1, cp);
	    if (!s)
		return 0;
	    if (**cp != ')') {
		printf("parseKRIVINE error: missing )\n");
		return 0;
	    } else
		*cp += 1;
	} else if (**cp == LAMBDA) {
	    *cp += 1;
	    s = calloc(1, sizeof(*s));
	    s->v = parse_var(cp);
	    s->m = parseKRIVINE(lev+1, cp);
//	    printKRIVINE(dummybuf, s->m, 0);
//	    printf("  after lambda: /%s %s --> <%s>\n", s->v, dummybuf, *cp);
	} else {
	    s = calloc(1, sizeof(*s));
	    s->v = parse_var(cp);
//	    printf("  var: <%s>\n", s->v);
	}
	if (t) {
//	    printKRIVINE(dummybuf, t, 0);
//	    printf("  old term: <%s>\n", dummybuf);
	    u = calloc(1, sizeof(*u));
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
printKRIVINE(char *cfg, struct term_s *t, char last)
{
    int len = 0;

    if (t->v && t->m) { // Lambda (sequence)
	len += sprintf(cfg + len, "(%c%s", LAMBDA, t->v);
	len += printKRIVINE(cfg + len, t->m, 'a');
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
	len += printKRIVINE(cfg + len, t->m, last);
	len += printKRIVINE(cfg + len, t->n, 'a');
    } else {
	len += printKRIVINE(cfg + len, t->m, last);
	len += sprintf(cfg + len, " (");
	len += printKRIVINE(cfg + len, t->n, '(');
	len += sprintf(cfg + len, ")");
    }
#endif
    return len;
}

// ----------------------------------------------------------------------
// synchronous (procedure style) content store interface

/*void
ccn_push(char *name, char *content)
{
    printf("<< push(%s --> %s)\n", name, content);
    if (cs_cnt >= MAX_CONTENTSTORE) {
	printf("** ccn_push: %d MAX_CONTENTSTORE reached, aborting\n", cs_cnt);
	exit(1);
    }
    cs[cs_cnt].name = strdup(name);
    cs[cs_cnt].content = strdup(content);
    cs_cnt++;
}
*/
char*
ccn_pull(char *name)
{
    int i;

    printf(">> pull(%s)    --> ", name);
    for (i = 0; i < cs_cnt; i++)
	if (!strcmp(cs[i].name, name)) {
	    printf("%s\n", cs[i].content);
	    return cs[i].content;
	}
    printf("?\n");
    return 0;
}

void
ccn_dump()
{
    int i;

    printf("Dump of content store:\n");
    for (i = 0; i < cs_cnt; i++)
	printf("  %s --> %s\n", cs[i].name, cs[i].content);
}

// ----------------------------------------------------------------------
// asynchronous (message exchange style) content store interface

void
ccn_listen_for(char *name)  // install a trigger
{
//    printf("ccn_listen_for(%s)\n", name);

    if (cs_trigger)
	printf("ERROR: cs_trigger already set to %s, new request %s ignored\n",
	       cs_trigger, name);
    else
	cs_trigger = name;
}

char*
ccn_name2content(char *name)    // synchronous lookup of the (local?) CS
{
    int i;

    //printf("ccn_name2content(%s)\n", name);

    if (!name)
	return 0;

    for (i = 0; i < cs_cnt; i++)
	if (!strcmp(cs[i].name, name))
	    return cs[i].content;
    return 0;
}

void
ccn_store(char *name, char *content)  // synchronous add content to the CS
{
    int i;
//    printf("ccn_store(%s, %s)\n", name, content);

    if (cs_cnt >= MAX_CONTENTSTORE) {
	printf("** ccn_store: %d MAX_CONTENTSTORE reached, aborting\n", cs_cnt);
	exit(1);
    }
    for (i = 0; i < cs_cnt; i++) {
	if (!strcmp(name, cs[i].name)) {
	    if (strcmp(content, cs[i].content))
		printf("** content store clash: should store name %s again, with differing content: old=<%s> new=<%s>\n", name, cs[i].content, content);
	    return;
	}
    }
    cs[cs_cnt].name = strdup(name);
    cs[cs_cnt].content = strdup(content);
    cs_cnt++;
}

void
ccn_store_update(char *name, char *content)  // synchronous add content to the CS
{
    int i;
//    printf("ccn_store(%s, %s)\n", name, content);
    for (i = 0; i < cs_cnt; i++) {
	if (!strcmp(name, cs[i].name)) {
	    if (strcmp(content, cs[i].content)){
		free(cs[i].content);
    		cs[i].content = strdup(content);
	    }
	    return;
	}
    }
}

void
ccn_request(char *name)   // send an interest
{
    printf("ccn_request(%s)\n", name);

    ccn_listen_for(name);
}

void
ccn_announce(char *name, char *content)  // make new content available
{
//    printf("ccn_announce(%s, %s)\n", name, content);

    ccn_store(name, content);

/*
    if (cs_in_name)
	printf("ERROR: cs_incoming already set to %s, new content %s ignored\n",
	       cs_in_name, name);
    else
	cs_in_name = strdup(name);
*/
}

// ----------------------------------------------------------------------

//#include "spooky-c-master/spooky-c.c"

char*
mkHash(char *s1)
{
    struct hash_s *h = hashes;
    int i;
    char *cp;
    char hval[16];

#if !defined(USE_SPOOKY)
    uint32_t i32;
    for (i = 0; i < hcnt; i++, h++) {
	if (!strcmp(h->s1, s1))
	    break;
    }
    i32 = ntohl(i);
    memset(hval, 0, sizeof(hval));
    memcpy(hval + 12, &i32, 4);
#else
    spooky_hash128(s1, strlen(s1), (uint64_t*) hval, (uint64_t*) &hval[8]);

    for (i = 0; i < hcnt; i++, h++) {
//	if (!strcmp(h->s1, s1))
	if (!memcmp(h->spooky, hval, sizeof(hval)))
	    break;
    }
#endif

    if (i >= hcnt) {
	if (hcnt >= MAX_HASHES) {
	    printf("** running out of hash ids: %d\n", hcnt);
	    exit(1);
	}
	hcnt++;
	h->s1 = strdup(s1);
	h->id = hcnt;
	memcpy(h->spooky, hval, sizeof(hval));
    }

    cp = malloc(10);
    sprintf(cp, "hash%03d", h->id);

/*
    cp = malloc(34);
    cp[0]='H';
    for (i = 0; i < 16; i++)
	sprintf(cp + 1 + 2*i, "%02x", (unsigned char)h->spooky[i]);
    cp[33] = '\0';
//    cp[5] = '\0';
*/
    return cp;
}

int
hashbits2id(char *bits)
{
    int i;
    for (i = 0; i < hcnt; i++)
	if (!memcpy(hashes[i].spooky, bits, 16))
	    return hashes[i].id;
    return -1;
}

void
dump_hashes()
{
    int i;

    if (hcnt)
	printf("Hash values:\n");
    for (i = 0; i < hcnt; i++) {
	printf("  hash%03d ", hashes[i].id);
#ifdef USE_SPOOKY
	{
	int j;
	for (j = 0; j < sizeof(hashes[i].spooky); j++)
	    printf("%02x", (unsigned char) hashes[i].spooky[j]);
	}
	printf(" = spooky");
#else
	printf("= hash");
#endif
	printf("(%s)\n", hashes[i].s1);
    }
}

// ----------------------------------------------------------------------

int
str2config(char *cfg, char **en, char **astack, char **rstack, char **code)
{
    int len;
    char *cp;

//    printf("str2config %s\n", cfg);

    if (strncmp(cfg, "CFG", 3)) {
	printf("wrong type (expected CFG|... instead of %s)\n", cfg);
	return 1;
    }
    cfg = strchr(cfg, '|');

    // env
    cp = strchr(cfg+1, '|');
    len = cp - cfg;
    if (len > 1) {
	*en = malloc(len);
	memcpy(*en, cfg+1, len - 1);
	(*en)[len-1] = 0;
    } else
	*en = 0;
    cfg = cp;

    // arg stack
    cp = strchr(cfg+1, '|');
    len = cp - cfg;
    if (len > 1) {
	*astack = malloc(len);
	memcpy(*astack, cfg+1, len - 1);
	(*astack)[len-1] = 0;
    } else
	*astack = 0;
    cfg = cp;

    // result stack
    cp = strchr(cfg+1, '|');
    len = cp - cfg;
    if (len > 1) {
	*rstack = malloc(len);
	memcpy(*rstack, cfg+1, len - 1);
	(*rstack)[len-1] = 0;
    } else
	*rstack = 0;

    *code = strdup(cp + 1);

    return 0;
}

// ----------------------------------------------------------------------

int
str2stack(char *cfg, char **head, char **tail)
{
    int len;
    char *cp;

//    printf("str2stack %s\n", cfg);

    if (!cfg) {
	*head = *tail = 0;
	return 1;
    }
    cfg = strchr(cfg, '|');

    cp = strchr(cfg+1, '|');
    len = cp - cfg;
    if (len > 1) {
	*head = malloc(len);
	memcpy(*head, cfg+1, len - 1);
	(*head)[len-1] = 0;
    } else
	*head = 0;
    cfg = cp+1;
    if (*cfg)
	*tail = strdup(cfg);
    else
	*tail = 0;

    return 0;
}

int
str2closure(char *str, char **en, char **termstr)
{
    int len;
    char *cp;

//    printf("str2closure %s\n", str);

    str = strchr(str, '|');

    cp = strchr(str+1, '|');
    len = cp - str;
    if (len > 1) {
	*en = malloc(len);
	memcpy(*en, str+1, len - 1);
	(*en)[len-1] = 0;
    } else
	*en = 0;
    *termstr = strdup(cp + 1);

    return 0;
}

char*
env2str(char **ename, char *env, char *v, char *cname)
{   // adds a new binding, creates a new environment
//    printf("extending the dict %s\n", env);
    if (env) // insert it at the front
	sprintf(dummybuf, "ENV|%s|%s|%s", v, cname, env + 4);
    else
	sprintf(dummybuf, "ENV|%s|%s", v, cname);
    *ename = mkHash(dummybuf);
    return strdup(dummybuf);
}


char*
closure2str(char **cname, char *en, struct term_s *t)
{
    char tmp[1000];

    sprint_term(dummybuf, t, 0);

    if (en)
	sprintf(tmp,  "CLO|%s|%s", en, dummybuf);
    else
	sprintf(tmp,  "CLO||%s", dummybuf);

    if (cname)
	*cname = mkHash(tmp); // +A
    return strdup(tmp);
}

char*
closure2str2(char **cname, char *en, char *expr)
{
    char tmp[1000];

    if (en)
	sprintf(tmp,  "CLO|%s|%s", en, expr);
    else
	sprintf(tmp,  "CLO||%s", expr);

    if (cname)
	*cname = mkHash(tmp);
    return strdup(tmp);
}

char*
argstack2str(char **asname, char *cname, char *tailname)
{
    char tmp[1000];

    if (!cname)
	strcpy(tmp, "AST||");
    if (!tailname)
	sprintf(tmp,  "AST|%s|", cname);
    else
	sprintf(tmp,  "AST|%s|%s", cname, tailname);
    if (asname)
	*asname = mkHash(tmp);
    return strdup(tmp);
}

char*
resstack2str(char **rsname, char *val, char *tail)
{
    char tmp[1000];

    strcpy(tmp, "RST");

    if (val && *val)
	sprintf(tmp + strlen(tmp), "|%s", val);
    if (tail && *tail)
	sprintf(tmp + strlen(tmp), "|%s", tail);
    if (strlen(tmp) == 3) {
	if (rsname)
	    *rsname = 0;
	return 0;
    }
    if (rsname)
	*rsname = mkHash(tmp);
    
    return strdup(tmp);
/*    if (strlen(tmp) == 3)
	strcpy(tmp, "RST|");
    if (rsname)
	*rsname = mkHash(tmp);
    return strdup(tmp);
*/
}

char*
config2str(char **cfg, char *ename, char *astack, char *rstack, char *code)
{
    char tmp[1000];

    strcpy(tmp, "CFG|");

/*
    printf("len %d %d %d %d ",
	   ename ? strlen(ename) : -1,
	   astack ? strlen(astack) : -1,
	   rstack ? strlen(rstack) : -1,
	   code ? strlen(code) : -1);
*/

    if (ename)
	sprintf(tmp+strlen(tmp),  "%s|", ename);
    else
	sprintf(tmp+strlen(tmp),  "|");
    if (astack)
	sprintf(tmp+strlen(tmp),  "%s|", astack);
    else
	sprintf(tmp+strlen(tmp),  "|");
    if (rstack)
	sprintf(tmp+strlen(tmp),  "%s|", rstack);
    else
	sprintf(tmp+strlen(tmp),  "|");
    if (code)
	strcpy(tmp+strlen(tmp), code);

//    printf("%d\n", strlen(tmp));

    if (cfg)
	*cfg = mkHash(tmp);
//    printf("%d <%s>\n", strlen(tmp), tmp);
    return strdup(tmp);
}

char*
env_find(char *env, char *v)
{
    char *cp = 0;
    int len;

    if (!env)
	return 0;
    env = strchr(env, '|');
    while (env && *env) {
	env++;
	cp = strchr(env, '|');
	if (!cp)
	    return 0;
	len = cp - env;
//	printf("comparing %s, len=%d, with %s\n", v, len, env);
	if (strlen(v) == len && !strncmp(env, v, len)) {
	    cp++;
	    cp = parse_var(&cp);
	    return cp;
	}
	env = strchr(cp+1, '|');
    }
    return 0;
}

//Pops the next argument and resolves a thunk
int pop(struct ccnl_relay_s * ccnl, char **tail, char *stackname, char *res)
{
    char *a1, *cp;
    char thunkid[10];
    int len;
    struct ccnl_content_s *c;
    cp = ccn_name2content(stackname); // should be split operation
    a1 = strchr(cp, '|');
    if (!a1) {
	printf("not enough args on stack\n");
	return 0;
    }
    cp = strchr(a1+1, '|');
    if(cp)
        len = cp - a1;
    else 
        len = strlen(a1);
    
    
    if(!strncmp(a1+1, "THUNK", 5)){
        printf("!!!!!!THUNK!!!!!!\n");
        memset(thunkid, 0, sizeof(thunkid));
        memcpy(thunkid, a1+1, len-1);
#ifndef ABSTRACT_MACHINE
        if((c = ccnl_nfn_resolve_thunk(ccnl, thunkid)) != NULL){ //thunk can be resolved
            char *h = a1+1;
            memcpy(res, c->content, c->contentlen);
            len = c->contentlen;
        }
        else{ //null as dummy if thunk could not be resolved
            DEBUGMSG(49, "Thunk could not be resolved, use 0 as debug dummy\n");
            a1[1] = '0';
            a1[2] = 0;
            len = 2;
            memcpy(res, a1+1, len-1);
        }
#endif
    }
    else{ //no thunk
         memcpy(res, a1+1, len-1);
    }
    if(cp)
        *tail = cp+1;
    else 
        *tail = 0;
    return len;
    
}

int pop1int(struct ccnl_relay_s * ccnl, char **tail, char *stackname)
{
    char res[100];
    memset(res, 0, sizeof(res));
    int len = pop(ccnl, tail, stackname, res);
    
    return atoi(res);    
}

int
pop2int(struct ccnl_relay_s * ccnl, char **tail, char *stackname, int *i1, int *i2)
{
    char res1[100], res2[100];
    memset(res1, 0, sizeof(res1));
    memset(res2, 0, sizeof(res2));
    int it1 = pop(ccnl, tail, stackname, res1);
    *tail = resstack2str(&stackname, *tail, "");
    int it2 = pop(ccnl, tail, stackname, res2);
    *tail = resstack2str(&stackname, *tail, "");
    
    *i1 = atoi(res1);
    *i2 = atoi(res2);
    return 2;
}

// ----------------------------------------------------------------------

void
log_ZAM(char *en, char *sn, struct term_s *t,
    char *cmd1, char *param1, char *cmd2, char *param2)
{
    printf("    log_ZAM     %s%s); %s%s\n", cmd1, param1, cmd2, param2);
}

// ----------------------------------------------------------------------

char*
ZAM_term(struct ccnl_relay_s *ccnl, char *cfg, int thunk_request, 
        int *num_of_required_thunks, struct ccnl_prefix_s *original_prefix) // written as forth approach
{
    char *en, *astack, *rstack = 0, *prog, *cp, *pending, *p;
    struct term_s *t;
    int len;

    if (strncmp(cfg, "CFG", 3)) {
	cfg = ccn_name2content(cfg); // resolve the hash
    }
    DEBUGMSG(1, "---ZAM_term exec \"%s\"\n", cfg);

    if (str2config(cfg, &en, &astack, &rstack, &prog))
	return NULL;

    if (!prog || strlen(prog) == 0) {
	cp = 0;
	if (rstack)
	    cp = ccn_name2content(rstack); // resolve the hash
	printf("nothing left to do, ");
	if (cp){
	    printf("result is '%s'\n", cp+4);
	    return cp;
	}
	else
	    printf("no result returned\n");
	return 0;
    }

    pending = strchr(prog, ';');
    p = strchr(prog, '(');
    
    if (!strncmp(prog, "ACCESS(", 7)) {
	char *env, *cname, *sname;
	//TODO: handle thunks here, change CFG and prog, remove ACCESS(thunk1) from terms
	//and add the thunk result to the (result) stack
	
	if (pending)
	    cp = pending-1;
	else
	    cp = prog + strlen(prog) - 1;
	len = cp - p;
	cp = malloc(len);
	memcpy(cp, p+1, len-1);
	cp[len-1] = '\0';
	DEBUGMSG(2, "---to do: access <%s>\n", cp);

	env = ccn_name2content(en); // should be split operation
//	printf("  >fox(env:%s) --> %s\n", en, env);

	cname = env_find(env, cp);
        //TODO check here for var in content store
        printf("ACCESS: %s\n", cp);
	if (!cname) {
	    env = ccn_name2content(global_dict);
	    cname = env_find(env, cp);
	    if (!cname) {
		printf("?? could not lookup var %s\n", cp);
		// NFN: do not publish stop, could be due to lack of ENV response
		return 0;
	    }
	}
	cp = argstack2str(&sname, cname, astack);
	if (cp) {
	    ccn_store(sname, cp);
	    free(cp);
	}
	if (pending)
	    cp = config2str(&cfg, en, sname, rstack, pending+1);
	else
	    printf("** not implemented %d\n", __LINE__);
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "CLOSURE(", 8)) {
	char *sname, *cname;

	if (pending)
	    cp = pending-1;
	else
	    cp = prog + strlen(prog) - 1;
	len = cp - p;
	cp = malloc(len);
	memcpy(cp, p+1, len-1);
	cp[len-1] = '\0';
	DEBUGMSG(2, "---to do: closure <%s>\n", cp);

/*
	sprintf(dummybuf, "CLO|%s|RESOLVE(%s)", en, cp);
	cname = mkHash(dummybuf);
*/
//	sprintf(dummybuf, "IRGENDETWAS(%s)", cp);
//	cp = closure2str2(&cname, en, dummybuf);
	if (!astack && !strncmp(cp, "RESOLVENAME(", 12)) { // detect tail recursion
	    char v[500], *c, *cl;
	    char *e = ccn_name2content(en); // should be split operation
	    int len;

//	    goto normal;
	    c = strchr(cp+12, ')'); if (!c) goto normal;
	    len = c - (cp+12);
	    memcpy(v, cp+12, len);
	    v[len] = '\0';
//	    printf("testing tail recursion %s\n", v);
	    cname = env_find(e, v); if (!cname) goto normal;
	    cl = ccn_name2content(cname);
	    c = strchr(cl+4, '|'); if (!c) goto normal;
//	    printf("testing tail recursion: %s %s\n", c+1, cp);
	    if (!strcmp(c+1, cp)) {
		DEBUGMSG(2, "** detected tail recursion case %s\n", cl);
	    } else
		goto normal;
	    cp = argstack2str(&sname, cname, astack);
	} else {
normal:
	    cp = closure2str2(&cname, en, cp);
	    if (cp)
		ccn_store(cname, cp);
	    cp = argstack2str(&sname, cname, astack);
//	ccn_push(sname, cp);
	    if (cp) {
		ccn_store(sname, cp);
		free(cp);
	    }
	}
	if (pending)
	    cp = config2str(&cfg, en, sname, rstack, pending+1);
	else
	    printf("** not implemented %d\n", __LINE__);

	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "GRAB(", 5)) {
	char *v, *cname, *tailname, *env, *en2;

	if (pending)
	    v = pending-1;
	else
	    v = prog + strlen(prog) - 1;
	len = v - p;
	v = malloc(len);
	memcpy(v, p+1, len-1);
	v[len-1] = '\0';
	DEBUGMSG(2, "---to do: grab <%s>\n", v);

	cp = ccn_name2content(astack); // should be split operation
//	printf("  >fox(stk:%s) --> %s\n", sn, cp);
	str2stack(cp, &cname, &tailname);
//	cp = ccn_name2content(cname);

	if (cname) {
	    env = en ? ccn_name2content(en) : 0;
	    cp = env2str(&en2, env, v, cname);
	    if (cp)
		ccn_store(en2, cp);
	} else
	    en2 = en;

	if (pending)
	    pending++;
	cp = config2str(&cfg, en2, tailname, rstack, pending);
//	ccn_push(cfg, cp);
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
	return cp;

    }

    if (!strncmp(prog, "TAILAPPLY", 9)) {
	char *cname, *tailname, *env, *code;

	DEBUGMSG(2, "---to do: tailapply\n");
	cp = ccn_name2content(astack); // should be split operation
//	printf("  >fox(stk:%s) --> %s\n", sn, cp);
//	printf(" content=%s\n", cp);
	str2stack(cp, &cname, &tailname);
	cp = ccn_name2content(cname);
//	printf(" content=%s\n", cp);
	str2closure(cp, &env, &code);
	if (pending)
//	    sprintf(dummybuf, "RESOLVEVAL(%s)%s", code, pending);
	    sprintf(dummybuf, "%s%s", code, pending);
	else
//	    sprintf(dummybuf, "RESOLVEVAL(%s)", code);
	    sprintf(dummybuf, "%s", code);
	cp = config2str(&cfg, env, tailname, rstack, dummybuf);
//	ccn_push(cfg, cp);
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
	return cp;
    }

    if (!strncmp(prog, "RESOLVENAME(", 12)) {

	if (pending)
	    cp = pending-1;
	else
	    cp = prog + strlen(prog) - 1;

	len = cp - p;
	cp = malloc(len);
	memcpy(cp, p+1, len-1);
	cp[len-1] = '\0';
	DEBUGMSG(2, "---to do: resolveNAME <%s>\n", cp);

	//check if term can be made available, if yes enter it as a var
	//try with searching in global env for an added term!
        t = parseKRIVINE(0, &cp);

//	if (term_is_var(t) && islower(t->v[0])) {
	if (term_is_var(t)) {
	    char *end = 0;
//	    int theInt;

	    cp = t->v;

	    //TODO if ccn name receive content, push to resultstack, similar to ints
	    //TODO if ccn name is name of a function //look for pattern

	    /*else*/if (isdigit(*cp)) {
//		theInt = strtol(cp, &end, 0);
		strtol(cp, &end, 0);
		if (end && *end)
		    end = 0;
	    }
	    else if(iscontent(cp)){
	   	// is content... 
		DEBUGMSG(99, "VAR IS CONTENT: %s\n", cp);
		end = cp;

	    }

	    if (end) {
//		printf("found a number! %d, %s\n", theInt, rstack);
		if (rstack) {
		    end = ccn_name2content(rstack); // should be split operation
//		    printf("resstack=%s\n", end);
		    if (end)
			end += 4;
		} else
		    end = 0;
		cp = resstack2str(&rstack, cp, end);
		if (cp)
		    ccn_store(rstack, cp);
//	    } 
		if (pending)
		    sprintf(dummybuf, "TAILAPPLY%s", pending);
		else
		    sprintf(dummybuf, "TAILAPPLY");
		cp = config2str(&cfg, en, astack, rstack, dummybuf);
		//		    printf("** closure/resstack/nopending not implemented %d\n", __LINE__);
	    } else {
		if (pending)
		    sprintf(dummybuf, "ACCESS(%s);TAILAPPLY%s", t->v, pending);
		else
		    sprintf(dummybuf, "ACCESS(%s);TAILAPPLY", t->v);
		cp = config2str(&cfg, en, astack, rstack, dummybuf);
	    }
//	ccn_push(cfg, cp);
	    ccn_listen_for(cfg);
	    ccn_announce(cfg, cp);
//	    DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	    return cp;
	}
	
	if (term_is_lambda(t)) {
	    char *var;
	    var = t->v;
	    printKRIVINE(dummybuf, t->m, 0);
	    cp = strdup(dummybuf);
	    if (pending)
		sprintf(dummybuf, "GRAB(%s);RESOLVENAME(%s)%s", var, cp, pending);
	    else
		sprintf(dummybuf, "GRAB(%s);RESOLVENAME(%s)", var, cp);
	    free(cp);
	    cp = config2str(&cfg, en, astack, rstack, dummybuf);
//	ccn_push(cfg, cp);
	    ccn_listen_for(cfg);
	    ccn_announce(cfg, cp);
//	    DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	    return cp;
	}

	if (term_is_app(t)) {
	    printKRIVINE(dummybuf, t->n, 0);
	    p = strdup(dummybuf);
	    printKRIVINE(dummybuf, t->m, 0);
	    cp = strdup(dummybuf);
/*
	    if (pending)
		sprintf(dummybuf, "CLOSURENAME(%s);RESOLVENAME(%s)%s", p, cp, pending);
	    else
		sprintf(dummybuf, "CLOSURENAME(%s);RESOLVENAME(%s)", p, cp);
*/
	    if (pending)
		sprintf(dummybuf, "CLOSURE(RESOLVENAME(%s));RESOLVENAME(%s)%s", p, cp, pending);
	    else
		sprintf(dummybuf, "CLOSURE(RESOLVENAME(%s));RESOLVENAME(%s)", p, cp);
	    cp = config2str(&cfg, en, astack, rstack, dummybuf);
//	ccn_push(cfg, cp);
	    ccn_listen_for(cfg);
	    ccn_announce(cfg, cp);
//	    DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	    return cp;
	}
    }

    if (!strncmp(prog, "OP_CMPEQ", 8)) {
	int i1, i2, acc;

	DEBUGMSG(2, "---to do: OP_CMPEQ <%s>/<%s>\n", cp, pending);
	pop2int(ccnl, &cp, rstack, &i2, &i1);
	acc = i1 == i2;

	cp = resstack2str(&rstack, cp, "");
	if (cp)
	    ccn_store(rstack, cp);

//	cp =  acc ? "true" : "false";
	cp =  acc ? "@x@y x" : "@x@y y";
	if (pending)
	    sprintf(dummybuf, "RESOLVENAME(%s)%s", cp, pending);
	else
	    sprintf(dummybuf, "RESOLVENAME(%s)", cp);
	cp = config2str(&cfg, en, astack, rstack, dummybuf);
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	printf("new config is %s\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_CMPLEQ", 9)) {
	int i1, i2, acc;

	DEBUGMSG(2, "---to do: OP_CMPLEQ <%s>/%s\n", cp, pending);
	pop2int(ccnl, &cp, rstack, &i2, &i1);
	acc = i1 <= i2;

	cp = resstack2str(&rstack, cp, "");
	if (rstack)
	    ccn_store(rstack, cp);

//	cp =  acc ? "true" : "false";
	cp =  acc ? "@x@y x" : "@x@y y";
	if (pending)
	    sprintf(dummybuf, "RESOLVENAME(%s)%s", cp, pending);
	else
	    sprintf(dummybuf, "RESOLVENAME(%s)", cp);
	cp = config2str(&cfg, en, astack, rstack, dummybuf);
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	printf("new config is %s\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_NIL", 6)) {
	cp = ccn_name2content(rstack); // should be split operation
	DEBUGMSG(2, "---to do: OP_NIL <%s>\n", cp);
	cp = resstack2str(&rstack, "NIL", cp ? cp+4 : 0);
	if (cp)
	    ccn_store(rstack, cp);

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_NOP", 6)) {
	DEBUGMSG(2, "---to do: OP_NOP <%s>\n", cp);
	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_POP", 6)) {
	cp = ccn_name2content(rstack); // should be split operation
	DEBUGMSG(2, "---to do: OP_POP <%s>\n", cp);
	if (cp) {
	    cp = strchr(cp,'|');
	    if (cp) {
//		printf("+%s\n", cp+1);
		cp = strchr(cp+1,'|');
//		printf("+%s\n", cp);
		cp = resstack2str(&rstack, cp ? cp+1 : 0, 0);
//		printf("+%s\n", cp);
		if (cp && rstack)
		    ccn_store(rstack, cp);
	    }
	}

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_SUB", 6)) {
	int i1, i2;

	DEBUGMSG(2, "---to do: OP_SUB <%s>\n", prog+8);
	pop2int(ccnl, &cp, rstack, &i2, &i1);
	sprintf(dummybuf, "%d", i1 - i2);
	cp = resstack2str(&rstack, dummybuf, cp);
	if (cp)
	    ccn_store(rstack, cp);

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_ADD", 6)) {
	int i1, i2;

	DEBUGMSG(2, "---to do: OP_ADD <%s>\n", prog+7);
	pop2int(ccnl, &cp, rstack, &i2, &i1);
	sprintf(dummybuf, "%d", i1 + i2);
	cp = resstack2str(&rstack, dummybuf, cp);
	if (cp)
	    ccn_store(rstack, cp);

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	printf("cp %s; <--> cfg %s\n", cp, cfg);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_MULT", 7)) {
	int i1, i2;

	DEBUGMSG(2, "---to do: OP_MULT <%s>\n", prog+8);
	pop2int(ccnl, &cp, rstack, &i2, &i1);
	sprintf(dummybuf, "%d", i1 * i2);
	cp = resstack2str(&rstack, dummybuf, cp);
	if (cp)
	    ccn_store(rstack, cp);

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if (!strncmp(prog, "OP_PRINTSTACK", 13)) {
	cp = ccn_name2content(rstack); // should be split operation
	DEBUGMSG(2, "---to do: OP_STACK\n");
	if (cp)
	    printf("%s\n", cp);

	if (pending)
	    cp = config2str(&cfg, en, astack, rstack, pending+1);
	else
	    cp = config2str(&cfg, en, astack, rstack, "");
	ccn_listen_for(cfg);
	ccn_announce(cfg, cp);
//	DEBUGMSG(2, "---new config is \"%s\"\n", cp);
	return cp;
    }

    if(!strncmp(prog, "OP_CALL", 5)){
        
        
	int num_params = pop1int(ccnl, &cp, rstack);
        int i, offset;
	char name[5];
//	printf("numparams: %d", num_params);
	cp = resstack2str(&rstack, cp, "");
	free(cp);
	cp = malloc(1000);
	sprintf(cp, "CLOSURE(OP_FOX);RESOLVENAME(@op(");///x(/y y x 2 op)));TAILAPPLY";
        offset = strlen(cp);	
	for(i = 0; i < num_params; ++i){
	    sprintf(name, "x%d", i);
	    offset += sprintf(cp+offset, "@%s(", name);
	}
	for(i =  num_params - 1; i >= 0; --i){
	    sprintf(name, "x%d", i);
	    offset += sprintf(cp+offset, " %s", name);
	}
        offset += sprintf(cp + offset, " %d", num_params);
	offset += sprintf(cp+offset, " op");
	for(i = 0; i < num_params+2; ++i){
            offset += sprintf(cp + offset, ")");
	}
        offset += sprintf(cp + offset, ";TAILAPPLY");
	cp = config2str(&cfg, en, astack, rstack, cp);
	

	ccn_listen_for(cp);
	return cp;
    }

    if(!strncmp(prog, "OP_FOX(", 4)){
    	
	int num_params = pop1int(ccnl, &cp, rstack);
        int i, j, complen;
        char *res = NULL;
	cp = config2str(&cfg, en, astack, rstack, cp);
	char **params = malloc(sizeof(char * ) * num_params); 
        char *out =  malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
        char *comp =  malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
        char *namecomp[CCNL_MAX_NAME_COMP];
        char *param;
        struct ccnl_interest_s * interest;
        struct ccnl_content_s *c;
        
	//read function name and  parameters from stack
	char *cp;
	cp = ccn_name2content(rstack); // should be split operation
        char *a1 = strchr(cp, '|');
        a1 = strchr(a1+1, '|');
        int routable = -1;
	for(i = 0; i < num_params; ++i){
		char *end = strchr(a1+1, '|');
		if(!end) end = a1+strlen(a1);
		int num = end - a1;
		params[i] = malloc (num * sizeof(char));
		memcpy(params[i], a1+1, num-1);
		params[i][num-1] = '\0';	
		printf("FUNCTION PARAMS: %s\n", params[i]);
                if(iscontent(params[i])){
                    routable = i;
                }
		a1 = end;
	}
        
        //as long as there is a routable parameter: try to find a result
        for(i = num_params - 1; i >= 0; --i){
            if(iscontent(params[i])){

                //make compute request
                memset(comp, 0, CCNL_MAX_PACKET_SIZE);
                memset(out, 0, CCNL_MAX_PACKET_SIZE);
                complen = sprintf(comp, "(@x call %d ", num_params);
                //build computation string
                for(j = 0; j < num_params; ++j){
                    if(i == j){
                        complen += sprintf(comp + complen, "x ");
                    }
                    else{
                        complen += sprintf(comp + complen, "%s ", params[j]);
                    }
                }
                complen += sprintf(comp + complen, ")");
#ifndef ABSTRACT_MACHINE
                //make interest
                
                param = strdup(params[i]);
                j= splitComponents(param, namecomp);
                
                if(isLocalAvailable(ccnl, namecomp)){ //if node has routable content local --> compute local
                    DEBUGMSG(49, "Routable content %s is local availabe --> start computation\n", params[i]);
                    complen = sprintf(comp, "call %d ", num_params);
                    for(i = 0; i < num_params; ++i){
                        complen += sprintf(comp+complen, "%s ", params[i]);
                    }
                    i = 0;
                    namecomp[i++] = "COMPUTE";
                    namecomp[i++] = strdup(comp);
                    if(thunk_request) namecomp[i++] = "THUNK";
                    namecomp[i++] = "NFN";
                    namecomp[i++] = NULL;
                    //if(thunk_request)  ask for other thunks to be available

                    len = mkInterest(namecomp, 0, out);
                    interest = ccnl_nfn_create_interest_object(ccnl, out, len, namecomp[0]);
                    if((c = ccnl_nfn_content_computation(ccnl, interest)) != NULL){
                        DEBUGMSG(49, "Content can be computed");
                        goto tail;
                    }else{
                        DEBUGMSG(49, "Got no thunk, continue with null result just for debugging\n");
                        goto tail;
                    }
                }
                
                DEBUGMSG(49, "Computation request: %s %s\n", comp, params[i]);
                int len = mkInterestCompute(namecomp, comp, complen, thunk_request, out);    
                free(param);
                
                interest = ccnl_nfn_create_interest_object(ccnl, out, len, namecomp[0]); //FIXME: NAMECOMP[0]???
                //search locally for content
                if((c = ccnl_nfn_local_content_search(ccnl, interest, CMP_MATCH)) != NULL){
                    DEBUGMSG(49, "Content locally found\n");
                    goto tail;
                }
                
                else if((c = ccnl_nfn_global_content_search(ccnl, interest)) != NULL){
                    DEBUGMSG(49, "Content found in the network\n");
                    goto tail;
                }
                //TODO Async stuff!?
#endif // ABSTRACT_MACHINE
            }//endif
            
        }//endfor
        
        //TODO Try to receive components and compute here???
             
tail:
        DEBUGMSG(49, "OP_FOX: PUSHING RESULT ON RESULTSTACK\n");
#ifndef ABSTRACT_MACHINE
        interest = ccnl_interest_remove(ccnl, interest);
        //extract result from content object 
        if(c == NULL){
            DEBUGMSG(2, "No content object found\n");
            res == NULL;
        }
        else if(thunk_request){ //TODO: no thunk for local search!
            --(*num_of_required_thunks);
            res = ccnl_nfn_add_thunk(ccnl, interest->prefix);
            if((*num_of_required_thunks) <= 0){
                ccnl_nfn_reply_thunk(ccnl, original_prefix);
            } 
        }
        else{
            res = c->content;
        }    
#endif
        if(res == NULL) {
            res = "0";
        }
        else if(!strncmp(res, "RST|",4)){
            res+=4;
        }
	char rst[1000];
	int len = sprintf(rst, "RST|%s", res);
	if(a1) sprintf(rst+len, "%s", a1);
        
	char *name =  mkHash(rst);
	ccn_store(name, rst);
	
	if (pending)
	    cp = config2str(&cfg, en, astack, name, pending+1);
	else
	    cp = config2str(&cfg, en, astack, name, "");
	printf("CP: %s \n" , cp);

	name = mkHash(cp);
	ccn_store(name, cp);
	ccn_listen_for(cp);
        free(out);
        free(comp);
	return cp;
	
	dump_hashes();
	ccn_dump();
	exit(-1);
    }

    //Handle if computation is finished
    if(!strncmp(prog, "halt", 4)){
        return ccn_name2content(rstack);
    }

    printf("unknown built-in command <%s>\n", prog);
    return 0;
}


char*
createDict(char *pairs[])
{
    int i = 0;
    char buf[9000], *cname, *ename;

    strcpy(buf, "ENV");

    while (pairs[i]) {
//	sprintf(dummybuf, "CLO|hash003|%s", pairs[i+1]);
	sprintf(dummybuf, "CLO||%s", pairs[i+1]);
	cname = mkHash(dummybuf);
	ccn_store(cname, dummybuf);
	sprintf(buf+strlen(buf), "|%s|%s", pairs[i], cname);
	i += 2;
    }
    ename = mkHash(buf);
    ccn_store(ename, buf);
    return ename;
}

char *Krivine_reduction(struct ccnl_relay_s *ccnl, char *expression, int thunk_request, 
        int *num_of_required_thunks, struct ccnl_prefix_s *original_prefix){

    char *prog, *cp, *config;
    char *setup_env[] = {
	"true", "RESOLVENAME(@x@y x)",
	"false", "RESOLVENAME(@x@y y)",

	"eq", "CLOSURE(OP_CMPEQ);RESOLVENAME(@op(@x(@y x y op)))",
	"leq", "CLOSURE(OP_CMPLEQ);RESOLVENAME(@op@x@y x y op)",
	"ifelse", "RESOLVENAME(@expr@yes@no(expr yes no))",
	"nop", "OP_NOP;TAILAPPLY",
	"nil", "OP_NIL;TAILAPPLY",
	"pop", "OP_POP;TAILAPPLY",
	"=", "OP_PRINTSTACK;TAILAPPLY",

	"add", "CLOSURE(OP_ADD);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY",
	"sub", "CLOSURE(OP_SUB);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY",
	"mult", "CLOSURE(OP_MULT);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY",
	"call", "CLOSURE(OP_CALL);RESOLVENAME(@op(@x x op));TAILAPPLY",

	"factprime", "RESOLVENAME(@f@n (ifelse (leq n 1) 1 (mult n ((f f)(sub n 1))) ))",
	"fact", "RESOLVENAME(factprime factprime)",
	"sha256", "RESOLVENAME(@c 42)",
	0
    };
    int len = strlen("CLOSURE(halt);RESOLVENAME()") + strlen(expression);
    if(strlen(expression) == 0) return 0;
    prog = malloc(len*sizeof(char));
    sprintf(prog, "CLOSURE(halt);RESOLVENAME(%s)", expression);
    //prog = "CLOSURE(halt);RESOLVENAME((/x ( add ((/x add x 1) 3) x)) 7)";

    config = strdup(prog);
    cp = global_dict = createDict(setup_env);
    sprintf(dummybuf, "CFG|%s|||%s", cp, config);
    free(config);
    config = strdup(dummybuf);

    cp = mkHash(config);

    ccn_listen_for(cp);
    ccn_store(cp, config);
   
    while ( cp && cs_trigger && steps < MAX_STEPS) {
	steps++;
	DEBUGMSG(1, "Step %d: %s\n", steps, cs_trigger);
	cp = cs_trigger;
	cs_trigger = 0;
	char *hash_name = cp;
	cp = ZAM_term(ccnl, cp, thunk_request, num_of_required_thunks, original_prefix);
    }
    ccn_store(expression, cp); //ersetze durch richtigen hash eintrag
    return cp;
}


//--------------------------------------------------------------------
//Catch segfault and print state
#define CATCH_SEGFAULT
#ifdef CATCH_SEGFAULT
#include <signal.h>
void segfault_handler(int signal, siginfo_t *si, void *arg)
{
    dump_hashes();
    ccn_dump();
    printf("Caught segfault at address %p\n", si->si_addr);
    printf("SEGMENTATION FAULT\n");
    exit(-9);
}
#endif

//-----------------------------------------------------------------------

#ifdef ABSTRACT_MACHINE
int
main(int argc, char **argv)
{
    int opt;
    char *prog, *res;
#ifdef CATCH_SEGFAULT
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
#endif

    while ((opt = getopt(argc, argv, "v:e:")) != -1) {
	switch (opt) {
	case 'v':
	    debug_level = atoi(optarg);
	    break;
	case 'e':
	    prog = optarg; 
	    break;
	default:
	    fprintf(stderr, "usage: %s [-v DEBUG_LEVEL]\n", argv[0]);
	    exit(-1);
	}
    }


    //printf("Demo: Call-by-name reduction of untyped lambda calculus over CCN\n");
    //printf("      christian.tschudin@unibas.ch, Jun 2013\n");
    
    res = Krivine_reduction(NULL, prog, 0, NULL, NULL);

    printf("\n res; %s\n\n", res);

    dump_hashes();
    ccn_dump();
    //printf("\nEnd of execution\n");
    return 0;
}
#endif /*ABSTRACT_MACHINE*/
//eof