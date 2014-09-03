/*
 * @f fwd-localrpc.c
 * @b CCN lite, local RPC processing
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
 * 2014-05-11 created
 */

#include "pkt-ndntlv.h"
#include "pkt-localrpc.h"

// this is a place holder file

int ccnl_RX_localrpc(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		    unsigned char **data, int *datalen)
{
    DEBUGMSG(6, "ccnl_RX_localrpc: %d bytes from face=%p (id=%d.%d) -- ignored\n",
	     *datalen, (void*)from, relay->id, from ? from->faceid : -1);

    return -1;
}

// eof
