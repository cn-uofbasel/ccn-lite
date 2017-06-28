/*
 * @f ccnl-prefix.h
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

#ifndef CCNL_PREFIX_H
#define CCNL_PREFIX_H

#include <stddef.h>
#include <unistd.h>

struct ccnl_content_s;

struct ccnl_prefix_s {
    unsigned char **comp;
    int *complen;
    int compcnt;
    char suite;
    unsigned char *nameptr; // binary name (for fast comparison)
    ssize_t namelen; // valid length of name memory
    unsigned char *bytes;   // memory for name component copies
    int *chunknum; // -1 to disable
#ifdef USE_NFN
    unsigned int nfnflags;
#  define CCNL_PREFIX_NFN   0x01

#  define CCNL_PREFIX_COMPU 0x04
// #ifdef USE_TIMEOUT_KEEPALIVE 
// FIXME: these values need to be compiled conditionally
#  define CCNL_PREFIX_KEEPALIVE 0x08
#  define CCNL_PREFIX_INTERMEDIATE 0x10
    int internum;
// #endif
    unsigned char *nfnexpr;
#endif
};

struct ccnl_prefix_s* 
ccnl_prefix_new(int suite, int cnt);

void 
ccnl_prefix_free(struct ccnl_prefix_s *prefix);

struct ccnl_prefix_s* 
ccnl_prefix_dup(struct ccnl_prefix_s *prefix);

int 
ccnl_prefix_appendCmp(struct ccnl_prefix_s *prefix, unsigned char *cmp, int cmplen);

int
ccnl_prefix_addChunkNum(struct ccnl_prefix_s *prefix, unsigned int chunknum);

int
ccnl_prefix_cmp(struct ccnl_prefix_s *pfx, unsigned char *md,
                struct ccnl_prefix_s *nam, int mode);

int
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
                  int minsuffix, int maxsuffix, struct ccnl_content_s *c);

//int
//ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri);

struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, char *nfnexpr, unsigned int *chunknum);

int
ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri);

#ifndef CCNL_LINUXKERNEL
   char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip, int escape_components, int call_slash);
#  define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)
#else
   char* ccnl_prefix_to_path(struct ccnl_prefix_s *pr);
#endif


#endif //EOF
