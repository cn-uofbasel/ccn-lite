/*
 * @f test/unit/prefix-cmp.c
 * @b unit test for prefix comparisons
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 * 2013-07-25 created
 */

#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>


#define CCNL_UNIX

#define USE_CCNxDIGEST
#define USE_DEBUG
#define USE_DEBUG_MALLOC

#include "ccnl-includes.h"

#include "ccnx.h"
#include "ccnl.h"
#include "ccnl-core.h"

#include "ccnl-ext-debug.c"
#include "ccnl-ext.h"
#include "ccnl-platform.c"

#define ccnl_app_RX(x,y)		do{}while(0)
#define ccnl_print_stats(x,y)		do{}while(0)
#define ccnl_ll_TX(a,b,c,d)		do{a=a;}while(0)
#define ccnl_close_socket(a)		do{}while(0)

#include "ccnl-core.c"

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
unescape_component(unsigned char *comp) // inplace, returns len after shrinking
{
    unsigned char *in = comp, *out = comp;
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

struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(*pr));

    if (!pr)
        return NULL;
    pr->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    pr->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    pr->path = (unsigned char*) ccnl_malloc(strlen(path)+1);
    if (!pr->comp || !pr->complen || !pr->path) {
        ccnl_free(pr->comp);
        ccnl_free(pr->complen);
        ccnl_free(pr->path);
        ccnl_free(pr);
        return NULL;
    }

    strcpy((char*) pr->path, path);
    cp = (char*) pr->path;
    for (path = strtok(cp, "/");
                 path && pr->compcnt < CCNL_MAX_NAME_COMP;
                 path = strtok(NULL, "/")) {
        pr->comp[pr->compcnt] = (unsigned char*) path;
        pr->complen[pr->compcnt] = strlen(path);
        pr->compcnt++;
    }
    return pr;
}

void
intro(void)
{
    printf(
	">>> Unit tests for CCN-lite's prefix matching <<<\n\n"
	"Below, a fixed prefix is matched against a set of names\n"
	"which can be a conten's name, or another prefix in the PIT.\n\n"
	"The two columns with numbers indicate the expected and actual\n"
	"return value of the ccnl_prefix_cmp() function for the name\n"
	"that follows. In case of a mismatch, a * is printed at the start\n"
	"of the line.\n\n"
	"A second set of tests relates to the ccnl_i_prefixof_c() fct,\n"
	"which handles the minsuffix, maxsuffix tasks.\n\n"
	"A third set tests the matchiong with respect to CCNx' implicit\n"
	"name component. The given prefix carries a hash value as its third\n"
	"component which was computed to match the digest of the (supposed)\n"
	"content packet whose name we try to match. This matching is\n"
	"handled by ccnl_i_prefixof_c(), too.\n\n\n"
	);
}

int
main(int argc, char *argv[])
{
    struct ccnl_prefix_s *thePrefix, *nm;
    struct ccnl_content_s c;
    int min, max, rc, i, len;
    struct ccnl_buf_s b;
    unsigned char *md;
    char cb[200];

    intro();

    thePrefix = ccnl_path_to_prefix("/a/b/c");

// ---
#define is_match(S, M, R1, R2, R3, R4, R5) \
    printf("\nPrefix <%s> has #" S " matching components\n", \
	   ccnl_prefix_to_path(thePrefix));			     \
    nm = ccnl_path_to_prefix("/a/b"); \
    rc = ccnl_prefix_cmp(nm, NULL, thePrefix, M);	  \
    printf("%s%2d  %2d %s\n", rc == R1 ? " " : "*", R1, rc, \
           ccnl_prefix_to_path(nm)); \
    free_prefix(nm); \
    nm = ccnl_path_to_prefix("/a/b/c"); \
    rc = ccnl_prefix_cmp(nm, NULL, thePrefix, M);	  \
    printf("%s%2d  %2d %s\n", rc == R2 ? " " : "*", R2, rc, \
           ccnl_prefix_to_path(nm)); \
    free_prefix(nm); \
    nm = ccnl_path_to_prefix("/a/b/c/d"); \
    rc = ccnl_prefix_cmp(nm, NULL, thePrefix, M);	  \
    printf("%s%2d  %2d %s\n", rc == R3 ? " " : "*", R3, rc, \
           ccnl_prefix_to_path(nm)); \
    free_prefix(nm); \
    nm = ccnl_path_to_prefix("/a/b/x/d"); \
    rc = ccnl_prefix_cmp(nm, NULL, thePrefix, M);	  \
    printf("%s%2d  %2d %s\n", rc == R4 ? " " : "*", R4, rc, \
           ccnl_prefix_to_path(nm)); \
    free_prefix(nm); \
    nm = ccnl_path_to_prefix("/a/b/c/x"); \
    rc = ccnl_prefix_cmp(nm, NULL, thePrefix, M);	  \
    printf("%s%2d  %2d %s\n", rc == R5 ? " " : "*", R5, rc, \
           ccnl_prefix_to_path(nm)); \
    free_prefix(nm)

    is_match("", CMP_MATCH,           2, 3, 3, 2, 3);
    is_match(" exact", CMP_EXACT,    -1, 0, -1, -1, -1);
    is_match(" longest", CMP_LONGEST, 2, 3, 3, 2, 3);

    printf("\n\n");


    b.datalen = 0;
    c.pkt = &b;

    thePrefix = ccnl_path_to_prefix("/a/b/c");

#define is_prefix(MI, MA, R1, R2, R3, R4) \
    min = MI, max = MA;   \
    printf("\n%s is matching prefix of name (mins=%d, maxs=%d)\n", \
	   ccnl_prefix_to_path(thePrefix), min, max); \
    c.name = ccnl_path_to_prefix("/a/b"); \
    rc = ccnl_i_prefixof_c(thePrefix, NULL, min, max, &c); \
    printf("%s%2d  %2d %s\n", rc == R1 ? " ":"*", R1, rc, \
    	   ccnl_prefix_to_path(c.name));		\
    free_prefix(c.name); \
    c.name = ccnl_path_to_prefix("/a/b/c"); \
    rc = ccnl_i_prefixof_c(thePrefix, NULL, min, max, &c); \
    printf("%s%2d  %2d %s\n", rc == R2 ? " ":"*", R2, rc, \
    	   ccnl_prefix_to_path(c.name));		\
    free_prefix(c.name); \
    c.name = ccnl_path_to_prefix("/a/b/c/d"); \
    rc = ccnl_i_prefixof_c(thePrefix, NULL, min, max, &c); \
    printf("%s%2d  %2d %s\n", rc == R3 ? " ":"*", R3, rc, \
    	   ccnl_prefix_to_path(c.name));		\
    free_prefix(c.name); \
    c.name = ccnl_path_to_prefix("/a/b/c/d/e"); \
    rc = ccnl_i_prefixof_c(thePrefix, NULL, min, max, &c); \
    printf("%s%2d  %2d %s\n", rc == R4 ? " ":"*", R4, rc, \
    	   ccnl_prefix_to_path(c.name));		\
    free_prefix(c.name)

    is_prefix(0, 0,    0, 0, 0, 0);
    is_prefix(0, 1,    0, 1, 0, 0);
    is_prefix(0, 2,    0, 1, 1, 0);
    is_prefix(0, 64,   0, 1, 1, 1);
    is_prefix(1, 0,    0, 0, 0, 0);
    is_prefix(1, 1,    0, 1, 0, 0);
    is_prefix(1, 2,    0, 1, 1, 0);
    is_prefix(1, 64,   0, 1, 1, 1);

    printf("\n\n");

    md = compute_ccnx_digest(c.pkt);
    strcpy(cb, "/a/b/");
    len = strlen(cb);
    for (i = 0; i < 32; i++)
	len += sprintf(cb + len, "%%%02x", md[i]);
    thePrefix = ccnl_path_to_prefix(cb);
    thePrefix->complen[2] = unescape_component(thePrefix->comp[2]);

//    debug_level = 99;

    is_prefix(0, 0,    1, 0, 0, 0);
    is_prefix(0, 1,    1, 0, 0, 0);
    is_prefix(0, 2,    1, 0, 0, 0);
    is_prefix(0, 64,   1, 0, 0, 0);
    is_prefix(1, 0,    0, 0, 0, 0);
    is_prefix(1, 1,    0, 0, 0, 0);
    is_prefix(1, 2,    0, 0, 0, 0);
    is_prefix(1, 64,   0, 0, 0, 0);


    printf("\n\n# end of test\n");

    return 0;
}

// eof
