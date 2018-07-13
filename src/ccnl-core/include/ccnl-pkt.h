/** 
 * @addtogroup CCNL-core
 * @{
 *
 * @file ccnl-pkt.h
 * @brief CCN lite, core CCN PKT data structure
 *
 * @author Christopher Scherb <christopher.scherb@unibas.ch>
 * @author Balazs Faludi <balazs.faludi@unibas.ch>
 *
 * @copyright (C) 2011-18 University of Basel
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

#ifndef CCNL_PKT_H
#define CCNL_PKT_H

#include <stddef.h>
#include <stdint.h>

#include "ccnl-buf.h"
#include "ccnl-prefix.h"

#ifdef USE_SUITE_NDNTLV
#include "ccnl-pkt-ndntlv.h"
#endif

// packet flags:  0000ebtt
#define CCNL_PKT_REQUEST    0x01 // "Interest"
#define CCNL_PKT_REPLY      0x02 // "Object", "Data"
#define CCNL_PKT_FRAGMENT   0x03 // "Fragment"
#define CCNL_PKT_FRAG_BEGIN 0x04 // see also CCNL_DATA_FRAG_FLAG_FIRST etc
#define CCNL_PKT_FRAG_END   0x08

/**
 * @brief Options for Interest messages of all TLV formats
 */
typedef union {
#ifdef USE_SUITE_NDNTLV
    struct ccnl_ndntlv_interest_opts_s ndntlv;      /**< options for NDN Interest messages */
#endif
} ccnl_interest_opts_u;

/**
 * @brief Options for Data messages of all TLV formats
 */
typedef union {
#ifdef USE_SUITE_NDNTLV
    struct ccnl_ndntlv_data_opts_s ndntlv;      /**< options for NDN Data messages */
#endif
} ccnl_data_opts_u;

struct ccnl_pktdetail_ccnb_s {
    int minsuffix, maxsuffix, aok, scope;
    struct ccnl_buf_s *nonce;
    struct ccnl_buf_s *ppkd;        /**< publisher public key digest */
};

struct ccnl_pktdetail_ccntlv_s {
    struct ccnl_buf_s *keyid;       /**< publisher keyID */
};

/**
 * @brief Packet details for the NDN TLV format
 */
struct ccnl_pktdetail_ndntlv_s {
    /* Interest */
    int minsuffix, maxsuffix, mbf, scope;
    struct ccnl_buf_s *nonce;      /**< nonce */
    struct ccnl_buf_s *ppkl;       /**< publisher public key locator */
    uint32_t interestlifetime;     /**< interest lifetime */
    /* Data */
    uint32_t freshnessperiod;      /**< content freshness period */
};

struct ccnl_pkt_s {
    struct ccnl_buf_s *buf;        /**< the packet's bytes */
    struct ccnl_prefix_s *pfx;     /**< prefix/name */
    unsigned char *content;        /**< pointer into the data buffer */
    int contlen;
    unsigned int type;   /**< suite-specific value (outermost type) */
    union {
        int final_block_id;
        unsigned int seqno;
    } val;
    union {
        struct ccnl_pktdetail_ccnb_s   ccnb;
        struct ccnl_pktdetail_ccntlv_s ccntlv;
        struct ccnl_pktdetail_ndntlv_s ndntlv;
    } s;                           /**< suite specific packet details */
#ifdef USE_HMAC256
    unsigned char *hmacStart;
    int hmacLen;
    unsigned char *hmacSignature;
#endif
    unsigned int flags;
    char suite;
};

/**
 * @brief Free a pkt data structure
 *
 * @param[in] pkt       pkt datastructure to be freed
*/
void
ccnl_pkt_free(struct ccnl_pkt_s *pkt);

/**
 * @brief Duplicates a pkt data structure
 *
 * @param[in] pkt       pkt data structure to be duplicated
 *
 * @return  returns a copy of @p pkt, NULL if failed
*/
struct ccnl_pkt_s *
ccnl_pkt_dup(struct ccnl_pkt_s *pkt);

/**
 * @brief Create a component for a pkt data structure (CCNTLV and CISTLV need special component start)
 *
 * @param[in] suite     suite for which the component should be created
 * @param[out] dst      target, where the created component should be stored
 * @param[in] src       content that should be written into the component
 * @param[in] srclen    length of the content @p s
 *
 * @return              length of the created component
*/
int
ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen);

/**
 * @brief prepend a component to buf
 *
 * @param[in] suite     suite for which the component should be created
 * @param[in] src       content that should be written into the component
 * @param[in] offset    offset for prepending the component
 * @param[out] buf      target, where the component was prepended
 *
 * @return              length of the entire component
*/
int
ccnl_pkt_prependComponent(int suite, char *src, int *offset, unsigned char *buf);

#endif // EOF
/** @} */
