/*
 * @f util/ccnl-common.c
 * @b common functions for the CCN-lite utilities
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
 * 2013-07-22 created
 * 2013-10-17 extended <christopher.scherb@unibas.ch>
 */

#ifndef CCNL_COMMON_C
#define CCNL_COMMON_C

#define CCNL_UNIX
#define _BSD_SOURCE
#define _SVID_SOURCE
// ----------------------------------------------------------------------

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>


#include "base64.c"

#include "../ccnl.h"
#include "../ccnl-core.h"

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)
#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)
#define free_prefix(p)  do { if (p) { free(p->comp); free(p->complen); free(p->bytes); free(p); }} while(0)

#define DEBUGMSG(LVL, ...) do {       \
        if ((LVL)>debug_level) break;   \
        fprintf(stderr, __VA_ARGS__);   \
    } while (0)
#define DEBUGSTMT(LVL, ...)             do {} while(0)

int debug_level = 99;

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

#include "../pkt-ccnb-dec.c"
#include "../pkt-ccntlv-dec.c"
#include "../pkt-ndntlv-dec.c"
#include "../pkt-localrpc.h"

#define ccnl_core_addToCleanup(b)       do{}while(0)

#include "../ccnl-util.c"

// ----------------------------------------------------------------------

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; int vallen; \
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, &valptr, &vallen) < 0) \
                goto Bail; \
        s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
        VAR = (unsigned char*) s; \
        continue; \
    } do {} while(0)

#define extractStr2(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; int vallen; \
        if (ccnl_ccnb_consume(typ, num, buf, buflen, &valptr, &vallen) < 0) \
                goto Bail; \
        s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
    VAR = (unsigned char*) s; \
        continue; \
    } do {} while(0)


#endif //CCNL_COMMON_C
// eof
