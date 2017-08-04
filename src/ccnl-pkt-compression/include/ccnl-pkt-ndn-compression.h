/**
 * @addtogroup CCNL-pkt-compression
 * @{
 * @file ccnl-pkt-ndn-compression.h
 * @brief CCN lite (CCNL), functions for compressing and decompressing NDN packets
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

#ifndef CCNL_PKT_NDN_COMPRESSION_H
#define CCNL_PKT_NDN_COMPRESSION_H

#include "ccnl-core.h"

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

/**
 * @brief   Compress a NDN Packet      
 * 
 * @param[in] ndn_pkt              the NDN packet to be compressed
 *
 * @return      pointer to compressed packet, NULL if failed
 */
struct ccnl_pkt_s *
ccnl_pkt_ndn_compress(struct ccnl_pkt_s *ndn_pkt);

/**
 * @brief   Decompress a NDN Packet      
 * 
 * @param[in] compressed_pkt              the NDN packet to be compressed
 *
 * @return      pointer to decompressed packet, NULL if failed
 */
struct ccnl_pkt_s * 
ccnl_pkt_ndn_decompress(struct ccnl_pkt_s *compressed_pkt);

int
ccnl_ndntlv_prependInterestCompressed(struct ccnl_prefix_s *name, int *nonce,
                            int *offset, unsigned char *buf);

int
ccnl_ndntlv_prependContentCompressed(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           int *contentpos, unsigned int *final_block_id,
                           int *offset, unsigned char *buf);


struct ccnl_pkt_s*
ccnl_ndntlvCompressed_bytes2pkt(unsigned char **data, int *datalen);

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED

#endif //CCNL_PKT_NDN_COMPRESSION_H

/** @} */
