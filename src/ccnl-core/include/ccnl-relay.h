/**
 * @ingroup CCNL-core
 * @{
 * @file ccnl-relay.h
 * @brief CCN lite (CCNL) data structure ccnl-relay. contains all important datastructures for CCN-lite forwarding
 *
 * @author Christopher Scherb <christopher.scherb@unibas.ch>
 * @author Christian Tschudin <christian.tschudin@unibas.ch>
 *
 * @copyright (C) 2011-17, University of Basel
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CCNL_RELAY_H
#define CCNL_RELAY_H

#include "ccnl-defs.h"
#include "ccnl-face.h"
#include "ccnl-if.h"
#include "ccnl-pkt.h"
#include "ccnl-sched.h"
#include "ccnl-cs.h"


struct ccnl_relay_s {
    void (*ccnl_ll_TX_ptr)(struct ccnl_relay_s*, struct ccnl_if_s*,
        sockunion*, struct ccnl_buf_s*);
#ifndef CCNL_ARDUINO
    time_t startup_time;
#endif
    int id;
    struct ccnl_face_s *faces;  /**< The existing forwarding faces */
    struct ccnl_forward_s *fib; /**< The Forwarding Information Base (FIB) */

    struct ccnl_interest_s *pit; /**< The Pending Interest Table (PIT) */

    ccnl_cs_content_t *contents;
    /** TODO: come up with a better place */
    ccnl_cs_ops_t *content_options;

    struct ccnl_buf_s *nonces;  /**< The nonces that are currently in use */
    int contentcnt;             /**< number of cached items */
    int max_cache_entries;      /**< max number of cached items -1: unlimited */
    int pitcnt;                 /**< Number of entries in the PIT */
    int max_pit_entries;        /**< max number of pit entries; -1: unlimited */ 
    struct ccnl_if_s ifs[CCNL_MAX_INTERFACES];
    int ifcount;               /**< number of active interfaces */
    char halt_flag;            /**< Flag to interrupt the IO_Loop and to exit the relay */
    struct ccnl_sched_s* (*defaultFaceScheduler)(struct ccnl_relay_s*,
                                                 void(*cts_done)(void*,void*)); /**< FuncPoint to the scheduler for faces*/
    struct ccnl_sched_s* (*defaultInterfaceScheduler)(struct ccnl_relay_s*,
                                                 void(*cts_done)(void*,void*)); /**< FuncPoint to the scheduler for interfaces*/
#ifdef USE_HTTP_STATUS
    struct ccnl_http_s *http;  /**< http server for status information*/
#endif
    void *aux;
  /*
    struct ccnl_face_s *crypto_face;
    struct ccnl_pendcrypt_s *pendcrypt;
    char *crypto_path;
  */
};

/**
 * @brief Broadcast an interest message to all available interfaces
 *
 * @param[in] ccnl          The CCN-lite relay used to send the interest
 * @param[in] interest      The interest which should be sent
 */
void ccnl_interest_broadcast(struct ccnl_relay_s *ccnl,
                             struct ccnl_interest_s *interest);

void ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);

struct ccnl_face_s*
ccnl_get_face_or_create(struct ccnl_relay_s *ccnl, int ifndx,
                        struct sockaddr *sa, size_t addrlen);

struct ccnl_face_s*
ccnl_face_remove(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);

void
ccnl_interface_enqueue(void (tx_done)(void*, int, int), struct ccnl_face_s *f,
                       struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
                       struct ccnl_buf_s *buf, sockunion *dest);

struct ccnl_buf_s*
ccnl_face_dequeue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);

void
ccnl_face_CTS_done(void *ptr, int cnt, int len);

/**
 * @brief Send a packet to the face @p to
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] to    face to send to
 * @param[in] pkt   packet to be sent
 *
 * @return   0 on success
 * @return   < 0 on failure
*/
int
ccnl_send_pkt(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
                struct ccnl_pkt_s *pkt);

/**
 * @brief Send a buffer to the face @p to 
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] to    face to send to
 * @param[in] buf   buffer to be sent
 *
 * @return   0 on success
 * @return   < 0 on failure
*/
int
ccnl_face_enqueue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to,
                 struct ccnl_buf_s *buf);


struct ccnl_interest_s*
ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

/**
 * @brief Forwards interest message according to FIB rules 
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] i     interest message to be forwarded
*/
void
ccnl_interest_propagate(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

/**
 * Removes content from the content store
 * 
 * @param[in] relay The global \ref ccnl_relay_s struct
 * @param[in] content The content to be removed
 */
void
//ccnl_content_remove(struct ccnl_relay_s *relay, ccnl_cs_content_t *content);
ccnl_content_remove(struct ccnl_relay_s *relay, struct ccnl_content_s *content);

/**
 * @brief add content @p c to the content store
 *
 * @note adding content with this function bypasses pending interests
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] c     content to be added to the content store
 *
 * @return   reference to the content @p c
 * @return   NULL, if @p c cannot be added
*/
struct ccnl_content_s*
ccnl_content_add2cache(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

/**
 * @brief deliver new content @p c to all clients with (loosely) matching interest 
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] c     content to be sent
 *
 * @return   number of faces to which the content was sent to
*/
int
ccnl_content_serve_pending(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

void
ccnl_do_ageing(void *ptr, void *dummy);

int
ccnl_nonce_find_or_append(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *nonce);

int
ccnl_nonce_isDup(struct ccnl_relay_s *relay, struct ccnl_pkt_s *pkt);

void
ccnl_core_cleanup(struct ccnl_relay_s *ccnl);

#ifdef NEEDS_PREFIX_MATCHING
/**
 * @brief Add entry to the FIB
 *
 * @par[in] relay   Local relay struct
 * @par[in] pfx     Prefix of the FIB entry
 * @par[in] face    Face for the FIB entry
 *
 * @return 0    on success
 * @return -1   on error
 */
int
ccnl_fib_add_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face);

/**
 * @brief Remove entry from the FIB
 *
 * @par[in] relay   Local relay struct
 * @par[in] pfx     Prefix of the FIB entry, may be NULL
 * @par[in] face    Face for the FIB entry, may be NULL
 *
 * @return 0    on success
 * @return -1   on error
 */
int
ccnl_fib_rem_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx,
                   struct ccnl_face_s *face);
#endif //NEEDS_PREFIX_MATCHING

/**
 * @brief Prints the current FIB
 *
 * @par[in] relay   Local relay struct
 */
void
ccnl_fib_show(struct ccnl_relay_s *relay);

/**
 * @brief Prints the content of the content store
 *
 * @par[in] ccnl Local relay struct
 */
void
ccnl_cs_dump(struct ccnl_relay_s *ccnl);

void
ccnl_interface_CTS(void *aux1, void *aux2);

#define DBL_LINKED_LIST_ADD(l,e) \
  do { if ((l)) (l)->prev = (e); \
       (e)->next = (l); \
       (l) = (e); \
  } while(0)

#define DBL_LINKED_LIST_REMOVE(l,e) \
  do { if ((l) == (e)) (l) = (e)->next; \
       if ((e)->prev) (e)->prev->next = (e)->next; \
       if ((e)->next) (e)->next->prev = (e)->prev; \
  } while(0)

#ifdef CCNL_APP_RX
int ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);
#endif

/**
 * @brief Add content @p c to the Content Store and serve pending Interests
 *
 * @param[in] ccnl  pointer to current ccnl relay
 * @param[in] c     content to add to the content store
 *
 * @return   0,  if @p c was added to the content store
 * @return   -1, otherwise
*/
int
ccnl_cs_add2(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

/**
 * @brief Remove content with @p prefix from the Content Store
 *
 * @param[in] ccnl      pointer to current ccnl relay
 * @param[in] prefix    prefix of the content to remove from the Content Store
 *
 * @return    0, if content with @p prefix was removed
 * @return   -1, if @p ccnl or @p prefix are NULL
 * @return   -2, if no memory could be allocated
 * @return   -3, if no content with @p prefix was found to be removed
*/
int
ccnl_relay_remove(struct ccnl_relay_s *ccnl, char *prefix);

/**
 * @brief Lookup content from the Content Store with prefix @p prefix
 *
 * @param[in] ccnl      pointer to current ccnl relay
 * @param[in] prefix    prefix of the content to lookup from the Content Store
 *
 * @return              pointer to the content, if found
 * @return              NULL, if @p ccnl or @p prefix are NULL
 * @return              NULL, on memory allocation failure
 * @return              NULL, if not found
*/
struct ccnl_content_s *
ccnl_relay_lookup(struct ccnl_relay_s *ccnl, char *prefix);

#endif //CCNL_RELAY_H
/** @} */
