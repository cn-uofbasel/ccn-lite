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

#include "ccnl-pkt-namecompression.h"
#include "ccnl-pkt-builder.h"
#include "ccnl-pkt-ndntlv.h"

#include <assert.h>

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

struct ccnl_pkt_s *
ccnl_pkt_ndn_compress(struct ccnl_pkt_s *ndn_pkt)
{
    
    struct ccnl_prefix_s *prefix = ccnl_pkt_prefix_compress(ndn_pkt->pfx);
    prefix->suite = CCNL_SUITE_NDNTLV;
    struct ccnl_buf_s *buf = NULL; //TODO Really required to rewrite the buffer every time (move this to separate function, only for creating compressed packet types)
    if(ndn_pkt->type == NDN_TLV_Interest){
        buf = ccnl_mkSimpleInterest(prefix, 0); //FIXME: set nonce //Replace function with function without tlvs.
    }
    else if(ndn_pkt->type == NDN_TLV_Data){
        //DEBUGMSG(DEBUG, "PACKET TYPE: %d\n", ndn_pkt->type);
        buf = ccnl_mkSimpleContent(prefix, ndn_pkt->content, ndn_pkt->contlen, 0);
    }
    assert(buf != NULL);
    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(ndn_pkt);
    ccnl_free(pkt->buf);
    ccnl_prefix_free(pkt->pfx);
    pkt->buf = buf;
    pkt->pfx = prefix;
    pkt->pfx->suite = CCNL_SUITE_NDNTLV;
    return pkt;
}

struct ccnl_pkt_s *
ccnl_pkt_ndn_decompress(struct ccnl_pkt_s *compressed_pkt)
{
    struct ccnl_prefix_s *prefix = ccnl_pkt_prefix_decompress(compressed_pkt->pfx);
    prefix->suite = CCNL_SUITE_NDNTLV;
    struct ccnl_buf_s *buf = NULL; //TODO Really required to rewrite the buffer every time(s.o.)
    if(compressed_pkt->type == NDN_TLV_Interest){
        buf = ccnl_mkSimpleInterest(prefix, 0); //FIXME: set nonce //Replace function with function without tlvs.
    }
    else if(compressed_pkt->type == NDN_TLV_Data){
        buf = ccnl_mkSimpleContent(prefix, compressed_pkt->content, compressed_pkt->contlen, 0);
    }
    assert(buf != NULL);
    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(compressed_pkt);
    ccnl_free(pkt->buf);
    ccnl_prefix_free(pkt->pfx);
    pkt->buf = buf;
    pkt->pfx = prefix;
    pkt->type = CCNL_SUITE_NDNTLV;
    pkt->pfx->suite = CCNL_SUITE_NDNTLV;
    return pkt;
}

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED
