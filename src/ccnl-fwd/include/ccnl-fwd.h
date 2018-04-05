/**
 * @addtogroup CCNL-fwd 
 * @brief CCN-lite Forwarding Library for different packet formats
 * @{
 * @file ccnl-fwd.h
 * @brief CCN lite (CCNL), packet forwarding logic
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

#ifndef CCNL_FWD_H
#define CCNL_FWD_H

#include "ccnl-core.h"

/**
 * @brief       Functionpointer to a CCN-lite Forwarder Function
 */
typedef int (*dispatchFct)(struct ccnl_relay_s*, struct ccnl_face_s*, 
                           unsigned char**, int*);

/**
 * @brief       Functionpointer to a CCN-lite CS-Matching Function
 */
typedef int (*cMatchFct)(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

/**
 * @brief       Defines for every Packet format the Forwarding and CS-Matching function
 *
 */
struct ccnl_suite_s {
    dispatchFct RX; /**< Forwarder Function for a specific packet format */
    cMatchFct cMatch; /**< CS-Matching Function for a speific packet format */
};

#ifdef USE_SUITE_CCNB
/**
 * @brief       Helper to process one CCNB packet
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 * @param[in] typ       packet type: interest or content object
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_ccnb_fwd(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
              unsigned char **data, int *datalen, int typ);

/**
 * @brief       process one CCNB packet (CCNB forwarding pipeline)
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_ccnb_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    unsigned char **data, int *datalen);
#endif // USE_SUITE_CCNB

#ifdef USE_SUITE_CCNTLV
/**
 * @brief       process one CCNTLV packet (CCNTLV forwarding pipeline)
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_CCNTLV

#ifdef USE_SUITE_CISTLV
/**
 * @brief       process one CISTLV packet (CISTLV forwarding pipeline)
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_cistlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_CISTLV

#ifdef USE_SUITE_NDNTLV
/**
 * @brief       process one NDNTLV packet (NDN forwarding pipeline)
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_ndntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
#endif // USE_SUITE_NDNTLV

/**
 * @brief Handle and incomming Interest Message
 *
 * @param[in] relay   pointer to current ccnl relay
 * @param[in] from    face on which the interest was received
 * @param[in] pkt     packet which was received   
 * @param[in] cMatch  matching strategy for the Content Store 
 *
 * @return   0 on success
 * @return   < 0 on failure
*/
int
ccnl_fwd_handleInterest(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                        struct ccnl_pkt_s **pkt, cMatchFct cMatch);


/**
 * @brief Handle and incomming Content Message
 *
 * @param[in] relay   pointer to current ccnl relay
 * @param[in] from    face on which the interest was received
 * @param[in] pkt     packet which was received   
 *
 * @return   0 on success
 * @return   < 0 on failure
*/
int
ccnl_fwd_handleContent(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                       struct ccnl_pkt_s **pkt);

#endif

/** @} */
