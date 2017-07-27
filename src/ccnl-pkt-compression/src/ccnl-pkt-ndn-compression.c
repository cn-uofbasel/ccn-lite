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
#include "ccnl-pkt-simple.h"
#include "ccnl-pkt-ndntlv.h"

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

struct ccnl_pkt_s *
ccnl_pkt_ndn_compress(struct ccnl_pkt_s *ndn_pkt)
{
    //compress name
    char* name = ccnl_prefix_to_path(ndn_pkt->pfx);
    int name_len = (int)strlen((char* )name);
    unsigned char *compressed_name = ccnl_malloc(name_len);
    int compressed_len = ccnl_pkt_compression_str2bytes((unsigned char*)name, 6, compressed_name, name_len);
    compressed_name = ccnl_realloc(compressed_name, compressed_len);
    //create compressed prefix
    struct ccnl_prefix_s *prefix = ccnl_prefix_new(CCNL_SUITE_NDNTLV, 1);
    prefix->comp[0] = compressed_name;
    prefix->complen[0] = compressed_len;

    struct ccnl_buf_s *buf = NULL; //TODO Really required to rewrite the buffer every time (move this to separate function, only for creating compressed packet types)
    if(ndn_pkt->type == NDN_TLV_Interest){
        buf = ccnl_mkSimpleInterest(prefix, 0); //FIXME: set nonce //Replace function with function without tlvs.
    }
    else if(ndn_pkt->type == NDN_TLV_Data){
        buf = ccnl_mkSimpleContent(prefix, ndn_pkt->content, ndn_pkt->contlen, 0);
    }

    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(ndn_pkt);
    ccnl_free(pkt->buf);
    ccnl_prefix_free(pkt->pfx);
    pkt->buf = buf;
    pkt->pfx = prefix;
    return pkt;
}

struct ccnl_pkt_s *
ccnl_pkt_ndn_decompress(struct ccnl_pkt_s *compressed_pkt)
{
    unsigned char* name = compressed_pkt->pfx->comp[0];
    int name_len = compressed_pkt->pfx->complen[0];
    int out_len = name_len * 8/6+1;
    unsigned char *decompressed_name = ccnl_malloc(out_len);
    out_len = ccnl_pkt_compression_bytes2str(name, name_len, 6, 
                              decompressed_name, out_len);
    decompressed_name = ccnl_realloc(decompressed_name, out_len);

    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix((char *)decompressed_name, CCNL_SUITE_NDNTLV, NULL, NULL);
    
    
    struct ccnl_buf_s *buf = NULL; //TODO Really required to rewrite the buffer every time(s.o.)
    if(compressed_pkt->type == NDN_TLV_Interest){
        buf = ccnl_mkSimpleInterest(prefix, 0); //FIXME: set nonce //Replace function with function without tlvs.
    }
    else if(compressed_pkt->type == NDN_TLV_Data){
        buf = ccnl_mkSimpleContent(prefix, compressed_pkt->content, compressed_pkt->contlen, 0);
    }

    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(compressed_pkt);
    ccnl_free(pkt->buf);
    ccnl_prefix_free(pkt->pfx);
    pkt->buf = buf;
    pkt->pfx = prefix;
    return pkt;
}

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED
