/*
 * @f ccnl-ext-nfncommon.h
 * @b CCN-lite, execution/state management of running computations
 *
 * Copyright (C) 2017, Balazs Faludi, University of Basel
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
 * 2017-07-19 created
 */

#ifndef CCNL_NFN_REQUESTS_H
#define CCNL_NFN_REQUESTS_H

//typedef int (*cMatchFct)(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

#include "ccnl-fwd.h"

#ifdef USE_NFN_REQUESTS

enum nfn_request_type {
    NFN_REQUEST_TYPE_UNKNOWN = 0,
    NFN_REQUEST_TYPE_START,
    NFN_REQUEST_TYPE_PAUSE,
    NFN_REQUEST_TYPE_RESUME,
    NFN_REQUEST_TYPE_CANCEL,
    NFN_REQUEST_TYPE_STATUS,
    NFN_REQUEST_TYPE_KEEPALIVE,
    NFN_REQUEST_TYPE_COUNT_INTERMEDIATES,
    NFN_REQUEST_TYPE_GET_INTERMEDIATE,
    NFN_REQUEST_TYPE_MAX = NFN_REQUEST_TYPE_GET_INTERMEDIATE,
};

struct nfn_request_s {
    unsigned char *comp;
    int complen;
    enum nfn_request_type type;
    char *arg;
};

struct nfn_request_s*
nfn_request_new(unsigned char *comp, int complen);

struct nfn_request_s*
nfn_request_copy(struct nfn_request_s *request);

void
nfn_request_free(struct nfn_request_s *request);

struct ccnl_pkt_s*
nfn_request_interest_pkt_new(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pfx);

int
nfn_request_handle_content(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct ccnl_pkt_s **pkt);

int
nfn_request_handle_interest(struct ccnl_relay_s *relay, struct ccnl_face_s *from,  struct ccnl_pkt_s **pkt, cMatchFct cMatch);

void
nfn_request_content_set_prefix(struct ccnl_content_s *c, struct ccnl_prefix_s *pfx);

int
nfn_request_get_arg_int(struct nfn_request_s* request);

char *
nfn_request_description_new(struct nfn_request_s* request);

#endif // USE_NFN_REQUESTS

#endif //CCNL_NFN_REQUESTS_H
