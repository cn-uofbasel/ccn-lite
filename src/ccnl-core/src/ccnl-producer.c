/*
 * @f ccnl-producer.c
 * @b Producer functions
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
 * Copyright (C) 2015, 2016, 2018, Oliver Hahm, INRIA
 * Copyright (C) 2018, Michael Frey, MSA Safety
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
 * 2018-01-24 created (based on ccn-lite-riot.c)
 */

#include "ccnl-producer.h"

/**
 * local producer function defined by the application
 */
static ccnl_producer_func _prod_func = NULL;

void
ccnl_set_local_producer(ccnl_producer_func func)
{
    _prod_func = func;
}

int
local_producer(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    if (_prod_func) {
        return _prod_func(relay, from, pkt);
    }

    return 0;
}
