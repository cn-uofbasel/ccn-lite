/*
 * @f ccnl-content.h
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

#ifndef CCNL_CONTENT_H
#define CCNL_CONTENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef CCNL_RIOT
#include "evtimer_msg.h"
#endif

struct ccnl_pkt_s;
struct ccnl_prefix_s;

typedef enum ccnl_content_flags_e {
    CCNL_CONTENT_FLAGS_STATIC = 0x01,
    CCNL_CONTENT_FLAGS_STALE = 0x02
} ccnl_content_flags;

/**
 * @brief Defines 
 *
 */
typedef struct ccnl_content_s {
    struct ccnl_content_s *next, *prev;  /**< pointers to the previous and the next element in the content store respectively */
    struct ccnl_pkt_s *pkt;              /**< a byte representation of received content (the actual packet) */
    ccnl_content_flags flags;            /**< indicates if */

    // NON-CONFORM: "The [ContentSTore] MUST also implement the Staleness Bit."
    // >> CCNL: currently no stale bit, old content is fully removed <<
    uint32_t last_used;

#ifdef USE_SUITE_NDNTLV
    uint32_t freshnessperiod; /**< defines how long a node has to wait (after the arrival of this data before) marking it “non-fresh”*/
    bool stale;               /**< indicates if a packet was marked 'non-fresh' -> staled */
#endif

    int served_cnt;
#ifdef CCNL_RIOT
    evtimer_msg_event_t evtmsg_cstimeout;
#endif
};
} ccnl_content;

struct ccnl_content_s*
ccnl_content_new(struct ccnl_pkt_s **pkt);

void
ccnl_content_free(struct ccnl_content_s *content);



#endif // EOF
