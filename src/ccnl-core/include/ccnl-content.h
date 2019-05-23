/**
 * @addtogroup CCNL-core
 * @{
 *
 * @file ccnl-content.h
 * @brief CCN lite, core CCNx content store definition and helper functions 
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

/**
 * @brief Defines if content added to the content store is
 * static or stale.
 */
typedef enum ccnl_content_flags_e {
    CCNL_CONTENT_FLAGS_NOT_STALE = 0x0, /**< content is not stale */
    CCNL_CONTENT_FLAGS_STATIC = 0x01,   /**< content is static */
    CCNL_CONTENT_FLAGS_STALE = 0x02,    /**< content is stale */
    CCNL_CONTENT_DO_NOT_USE = UINT8_MAX /**< for internal use only, sets the width of the enum to sizeof(uint8_t) */
} ccnl_content_flags;

/**
 * @brief Defines an entry in the content store.
 *
 * The content store is implemented as linked list and stores the
 * full byte representation (the packet) of an content object 
 * (and not just the content itself).
 */
typedef struct ccnl_content_s {
    struct ccnl_content_s *next;          /**< pointer to the next element in the content store */
    struct ccnl_content_s *prev;          /**< pointer to the previous element in the content store */
    struct ccnl_pkt_s *pkt;               /**< a byte representation of received content (the actual packet) */

    ccnl_content_flags flags;             /**< indicates if content is marked static or stale */

    // NON-CONFORM: "The [ContentSTore] MUST also implement the Staleness Bit."
    // >> CCNL: currently no stale bit, old content is fully removed <<

    uint32_t last_used;                   /**< indicates when the stored content was last used */
#ifdef CCNL_RIOT
    evtimer_msg_event_t evtmsg_cstimeout; /**< event timer message which is triggered when a timeout in the content store occurs */
#endif
    int served_cnt;                       /**< determines how often the content has been served */
} ccnl_cs_content_t;

/**
 * @brief Wraps a \p packet into a \ref ccnl_content data structure
 * 
 * The function does not add the packet to the content store! 
 *
 * @param[in] packet The packet to be wrapped into a \ref ccnl_content data structure
 * 
 * @return Upon success, the \p packet wrapped into a \ref ccnl_content data structure
 * @return NULL if the operation fails
 */
struct ccnl_content_s*
ccnl_content_new(struct ccnl_pkt_s **packet);

/**
 * @brief Frees a \p content object.

 * @param[in] content The content object which will be freed 
 */
int
ccnl_content_free(struct ccnl_content_s *content);

#endif // EOF
/** @} */
