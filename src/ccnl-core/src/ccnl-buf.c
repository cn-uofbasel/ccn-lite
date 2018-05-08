/*
 * @f ccnl-buf.c
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
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

#ifndef CCNL_LINUXKERNEL
#include <stdio.h>
#include "ccnl-os-time.h"
#include "ccnl-buf.h"
#include "ccnl-logging.h"
#include "ccnl-relay.h"
#include "ccnl-forward.h"
#include "ccnl-prefix.h"
#include "ccnl-malloc.h"
#else
#include <ccnl-os-time.h>
#include <ccnl-buf.h>
#include <ccnl-logging.h>
#include <ccnl-relay.h>
#include <ccnl-forward.h>
#include <ccnl-prefix.h>
#include <ccnl-malloc.h>
#endif

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = (struct ccnl_buf_s*) ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

struct ccnl_buf_s *bufCleanUpList;

void
ccnl_core_addToCleanup(struct ccnl_buf_s *buf)
{
    buf->next = bufCleanUpList;
    bufCleanUpList = buf;
}

void
ccnl_core_cleanup(struct ccnl_relay_s *ccnl)
{
    int k;

    DEBUGMSG_CORE(TRACE, "ccnl_core_cleanup %p\n", (void *) ccnl);

    while (ccnl->pit)
        ccnl_interest_remove(ccnl, ccnl->pit);
    while (ccnl->faces)
        ccnl_face_remove(ccnl, ccnl->faces); // removes allmost all FWD entries
    while (ccnl->fib) {
        struct ccnl_forward_s *fwd = ccnl->fib->next;
        ccnl_prefix_free(ccnl->fib->prefix);
        ccnl_free(ccnl->fib);
        ccnl->fib = fwd;
    }
    while (ccnl->contents)
        ccnl_content_remove(ccnl, ccnl->contents);
    while (ccnl->nonces) {
        struct ccnl_buf_s *tmp = ccnl->nonces->next;
        ccnl_free(ccnl->nonces);
        ccnl->nonces = tmp;
    }
    for (k = 0; k < ccnl->ifcount; k++)
        ccnl_interface_cleanup(ccnl->ifs + k);

    while (bufCleanUpList) {
        struct ccnl_buf_s *tmp = bufCleanUpList->next;
        ccnl_free(bufCleanUpList);
        bufCleanUpList = tmp;
    }
}
