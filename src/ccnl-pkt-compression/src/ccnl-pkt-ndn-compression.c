/**
 * @f ccnl-pkt-ndn-compression.h
 * @brief CCN lite (CCNL), compress and decompress NDN packet format
 * requires USE_SUITE_COMPRESSED to be defined 
 *
 * author Christopher Scherb <christopher.scherb@unibas.ch>
 * author Cenk Gündoğan <cenk.guendogan@haw-hamburg.de>
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
 
 #include "ccnl-pkt-ndn-compression.h"

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

struct ccnl_pkt_s *
ccnl_pkt_ndn_compress(struct ccnl_pkt_s *ndn_pkt){
    (void)ndn_pkt;
    return NULL;
}

struct ccnl_pkt_s *
ccnl_pkt_ndn_decompress(struct ccnl_pkt_s *compressed_pkt){
    (void)compressed_pkt;
    return NULL;
}

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED
