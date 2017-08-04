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
 


#include "ccnl-pkt-builder.h"
#include "ccnl-pkt-ndntlv.h"

#include <assert.h>

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

#include "ccnl-pkt-ndn-compression.h"
#include "ccnl-pkt-namecompression.h"

struct ccnl_pkt_s *
ccnl_pkt_ndn_compress(struct ccnl_pkt_s *ndn_pkt)
{
    
    struct ccnl_prefix_s *prefix = ccnl_pkt_prefix_compress(ndn_pkt->pfx);
    prefix->suite = CCNL_SUITE_NDNTLV;
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs, contentpos;
    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    if(ndn_pkt->type == NDN_TLV_Interest){
        int nonce = rand();
        len = ccnl_ndntlv_prependInterestCompressed(prefix, &nonce, &offs, tmp);
    }
    else if(ndn_pkt->type == NDN_TLV_Data){
        //DEBUGMSG(DEBUG, "PACKET TYPE: %d\n", ndn_pkt->type);
        len = ccnl_ndntlv_prependContent(prefix, ndn_pkt->content, ndn_pkt->contlen,
                                         &contentpos, NULL, &offs, tmp);
    }
    if (len > 0){
        buf = ccnl_buf_new(tmp + offs, len);
    }
    ccnl_free(tmp);
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
    //TODO: parse nonce?
    struct ccnl_prefix_s *prefix = ccnl_pkt_prefix_decompress(compressed_pkt->pfx);
    prefix->suite = CCNL_SUITE_NDNTLV;
    struct ccnl_buf_s *buf = NULL; //TODO Really required to rewrite the buffer every time(s.o.)
    if(compressed_pkt->type == NDN_TLV_Interest){
        buf = ccnl_mkSimpleInterest(prefix, 0);
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

int
ccnl_ndntlv_prependInterestCompressed(struct ccnl_prefix_s *name, int *nonce,
                            int *offset, unsigned char *buf){

    int oldoffset = *offset;
    // header
    char header = 0;
    char type = 0; // interest
    char minSuffixComponets = 0;
    char maxSuffixComponets = 0;
    char publisherPublicKey = 0;
    char exclude = 0;
    char childSelector = 0;
    char mustBeFresh = 0;
    char interestLifetime = 1;

    // data fields
    unsigned char lifetime[2] = { 0x0f, 0xa0 };

    header = type << 7 | minSuffixComponets << 6 | maxSuffixComponets << 5
             | publisherPublicKey << 4 | exclude << 3 | childSelector << 2
             | mustBeFresh << 1 | interestLifetime;

    if(interestLifetime){
        if (ccnl_ndntlv_prependBlob(NDN_TLV_InterestLifetime, lifetime, 2,
                                    offset, buf) < 0) {
            return -1;
        }
    }
    if (nonce && ccnl_ndntlv_prependBlob(NDN_TLV_Nonce, (unsigned char*) nonce, 4,
                                         offset, buf) < 0){
        return -1;
    }
    if (ccnl_ndntlv_prependName(name, offset, buf)){
        return -1;
    }
    if (ccnl_ndntlv_prependTL(header, oldoffset - *offset,
                              offset, buf) < 0) {
        return -1;
    }
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependContentCompressed(struct ccnl_prefix_s *name,
                                     unsigned char *payload, int paylen,
                                     int *contentpos, unsigned int *final_block_id,
                                     int *offset, unsigned char *buf){

    // header
    char header = 0;
    char type = 1; // content
    char minSuffixComponets = 0;
    char maxSuffixComponets = 0;
    char publisherPublicKey = 0;
    char exclude = 0;
    char childSelector = 0;
    char mustBeFresh = 0;
    char interestLifetime = 1;

    header = type << 7 | minSuffixComponets << 6 | maxSuffixComponets << 5
             | publisherPublicKey << 4 | exclude << 3 | childSelector << 2
             | mustBeFresh << 1 | interestLifetime;

    int oldoffset = *offset, oldoffset2;
    unsigned char signatureType = NDN_VAL_SIGTYPE_DIGESTSHA256;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, 0, offset, buf) < 0)
        return -1;

    // to find length of SignatureInfo
    oldoffset2 = *offset;

    // use NDN_SigTypeVal_SignatureSha256WithRsa because this is default in ndn client libs
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, &signatureType, 1,
                                offset, buf) < 0)
        return 1;

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, oldoffset2 - *offset, offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf) < 0)
        return -1;

    // to find length of optional (?) MetaInfo fields
    oldoffset2 = *offset;
    if(final_block_id) {
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *final_block_id,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf) < 0)
            return -1;

        // optional
        if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset,
                                  offset, buf) < 0)
            return -1;
    }

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset,
                              offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf))
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependTL(header, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    if (contentpos)
        *contentpos -= *offset;

    return oldoffset - *offset;
}

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED
