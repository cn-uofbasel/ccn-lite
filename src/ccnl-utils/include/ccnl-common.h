/**
 * @addtogroup CCNL-utils
 * @{
 *
 * @file ccnl-common.h
 * @brief Common functions for the CCN-lite utilities
 *
 * Copyright (C) 2013-18 Christian Tschudin, University of Basel
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
 */
#ifndef CCNL_COMMON_H
#define CCNL_COMMON_H

#ifndef CCNL_UAPI_H_    // if CCNL_UAPI_H_ is defined then the following config is taken care elsewhere in the code composite


#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _SVID_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "base64.h"

#include "ccnl-os-includes.h"
#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "ccnl-pkt-builder.h"
#include "ccnl-malloc.h"
#include "ccnl-os-time.h"
#include "ccnl-logging.h"
#include "ccnl-pkt-builder.h"

#ifndef USE_DEBUG_MALLOC
#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)
#endif //USE_DEBUG_MALLOC
#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)

struct ccnl_prefix_s* ccnl_prefix_new(char suite, uint32_t cnt);
int ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#include "ccnl-core.h"
#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-localrpc.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-switch.h"

#include "ccnl-socket.h"


// include only the utils, not the core routines:
#ifdef USE_FRAG
#include "../ccnl-frag.h"
#endif

#else // CCNL_UAPI_H_ is defined

#include "base64.c"
#ifdef RIOT_VERSION
#include "ccnl-defs.h"
#include "net/packet.h"
#include <unistd.h>
#include "sys/socket.h"
#include "ccn-lite-riot.h"
#include "ccnl-headers.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-ccnb.h"


extern int ccnl_suite2defaultPort(int suite);
#endif

#endif // CCNL_UAPI_H_


// ----------------------------------------------------------------------

const char* ccnl_enc2str(int enc);

// ----------------------------------------------------------------------

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; size_t vallen; \
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
        char *s; unsigned char *valptr; size_t vallen; \
        if (ccnl_ccnb_consume(typ, num, buf, buflen, &valptr, &vallen) < 0) \
                goto Bail; \
        s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
    VAR = (unsigned char*) s; \
        continue; \
    } do {} while(0)

// ----------------------------------------------------------------------

struct key_s {
    struct key_s *next;
    unsigned char* key;
    int keylen;
};

struct key_s* load_keys_from_file(char *path);

// ----------------------------------------------------------------------

int
ccnl_parseUdp(char *udp, int suite, char **addr, int *port);

#endif 
/** @} */
