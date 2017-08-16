/**
 * @addtogroup CCNL-pkt-compression
 * @{
 * @file ccnl-pkt-namecompression.h
 * @brief CCN lite (CCNL), functions for compressing and decompressing ICN names
 * @note requires USE_SUITE_COMPRESSED to be defined 
 *
 * @author Christopher Scherb <christopher.scherb@unibas.ch>
 * @author Cenk Gündoğan <cenk.guendogan@haw-hamburg.de>
 * @author Balazs Faludi <balazs.faludi@unibas.ch>
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

#ifndef CCNL_PKT_NAMECOMPRESSION_H
#define CCNL_PKT_NAMECOMPRESSION_H

#include "ccnl-core.h"

#ifdef USE_SUITE_COMPRESSED

/**
 * @brief   Compress a ICN Name String, use / as separator     
 * 
 * @param[in] str          input string containing the ICN name
 * @param[in] charlen      number of bits per char which should be used for compression 
 * @param[out] out         encoded string, must be allocated  
 * @param[int] outlen      maximal length of the compressed string @p out (allocated bytes)  
 *
 * @return     length of the compressed string
 */
int
ccnl_pkt_compression_str2bytes(unsigned char *str, int charlen, 
                              unsigned char *out, int outlen);

/**
 * @brief   DcCompress a ICN Name String, use / as separator     
 * 
 * @param[in] in          compressed input string containing the ICN name
 * @param[in] in          length of the compressed input string
 * @param[in] charlen     number of bits per char of the compressed string 
 * @param[out] out        decoded string, must be allocated  
 * @param[in] outlen     maximal length of the decoded string @p out (allocated bytes)  
 *
 * @return     length of the compressed string
 */
int
ccnl_pkt_compression_bytes2str(unsigned char *in, int charlen, int outlen,
                              unsigned char *out);


/**
 * @brief   Compress the name components of a prefix     
 * 
 * @param[in] pfx         prefix of which the namecomponents should be compressed
 *
 * @return     prefix with compressed name components
 */
struct ccnl_prefix_s *
ccnl_pkt_prefix_compress(struct ccnl_prefix_s *pfx);


/**
 * @brief   Deompress the name components of a prefix     
 * 
 * @param[in] pfx         prefix of which the namecomponents should be decompressed
 *
 * @return     prefix with decompressed name components
 */
struct ccnl_prefix_s *
ccnl_pkt_prefix_decompress(struct ccnl_prefix_s *pfx);

#endif //USE_SUITE_COMPRESSED
#endif //CCNL_PKT_NAMECOMPRESSION_H

/** @} */
