/**
 * @addtogroup CCNL-pkt-compression
 * @brief Packet Compression LIB for CCN-lite
 * @{
 * @file ccnl-fwd-decompress.h
 * @brief CCN lite (CCNL), extract compressed packet formats first (for IoT)
 * @note requires USE_SUITE_COMPRESSED to be defined 
 *
 * @author Christopher Scherb <christopher.scherb@unibas.ch>
 * @author Cenk Gündoğan <cenk.guendogan@haw-hamburg.de>
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
 
#ifndef CCNL_FWD_DECOMPRESS_H
#define CCNL_FWD_DECOMPRESS_H


#include "ccnl-core.h"

#ifdef USE_SUITE_COMPRESSED

#ifdef USE_SUITE_NDNTLV
/**
 * @brief       process compressed NDNTLV packet. Decompress and call NFN Forwarder
 * 
 * @param[in] relay     pointer to current ccnl relay
 * @param[in] from      face on which the message was received
 * @param[in] data      data which were received
 * @param[in] datalen   length of the received data
 *
 * @return      < 0 if no bytes consumed or error
 */
int
ccnl_ndntlv_forwarder_decompress(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen);
                      
#endif //USE_SUITE_NDNTLV

#endif //USE_SUITE_COMPRESSED
#endif //CCNL_FWD_DECOMPRESS_H

 /** @} */

