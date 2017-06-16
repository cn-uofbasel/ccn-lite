/*
 * @f ccnl-content.c
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

#include "ccnl-content.h"
#include "ccnl-malloc.h"
#include "ccnl-prefix.h"

// TODO: remove unused ccnl parameter
struct ccnl_content_s*
ccnl_content_new(struct ccnl_pkt_s **pkt)
{
    struct ccnl_content_s *c;

    char *s = NULL;
    DEBUGMSG_CORE(TRACE, "ccnl_content_new %p <%s [%d]>\n",
             (void*) *pkt, (s = ccnl_prefix_to_path((*pkt)->pfx)), ((*pkt)->pfx->chunknum)? *((*pkt)->pfx->chunknum) : -1);
    ccnl_free(s);

    c = (struct ccnl_content_s *) ccnl_calloc(1, sizeof(struct ccnl_content_s));
    if (!c)
        return NULL;
    c->pkt = *pkt;
    *pkt = NULL;
    c->last_used = CCNL_NOW();

    return c;
}

void
ccnl_content_free(struct ccnl_content_s *content) 
{
    ccnl_pkt_free(content->pkt);
    ccnl_free(content);
}