/*
 * @f ccnl-interest.h
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
 * Copyright (C) 2018 HAW Hamburg
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

#ifndef CCNL_INTEREST_H
#define CCNL_INTEREST_H

#include "ccnl-pkt.h"
#include "ccnl-face.h"

#ifdef CCNL_RIOT
#include "evtimer_msg.h"
#endif

struct ccnl_pendint_s { // pending interest
    struct ccnl_pendint_s *next; // , *prev;
    struct ccnl_face_s *face;
    uint32_t last_used;
};

struct ccnl_interest_s {
    struct ccnl_interest_s *next, *prev;
    struct ccnl_pkt_s *pkt;
    struct ccnl_face_s *from;
    struct ccnl_pendint_s *pending; // linked list of faces wanting that content
    unsigned short flags;
    uint32_t lifetime;
#define CCNL_PIT_COREPROPAGATES    0x01
#define CCNL_PIT_TRACED            0x02
    uint32_t last_used;
    int retries;
#ifdef CCNL_RIOT
    evtimer_msg_event_t evtmsg_retrans;
    evtimer_msg_event_t evtmsg_timeout;
#endif
};

//struct ccnl_interest_s*
//ccnl_interest_new(struct ccnl_face_s *from, struct ccnl_pkt_s **pkt);

int
ccnl_interest_isSame(struct ccnl_interest_s *i, struct ccnl_pkt_s *pkt);

int
ccnl_interest_append_pending(struct ccnl_interest_s *i, struct ccnl_face_s *from);

int
ccnl_interest_remove_pending(struct ccnl_interest_s *i, struct ccnl_face_s *face);

#endif //CCNL_INTEREST_H
