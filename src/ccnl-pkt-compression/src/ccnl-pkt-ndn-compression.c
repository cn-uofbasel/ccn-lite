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
    if(!prefix){
        return NULL;
    }
    prefix->suite = CCNL_SUITE_NDNTLV;
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs, contentpos = 0;
    unsigned int contentlen = 0;
    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    if(!tmp){
        ccnl_prefix_free(prefix);
        return NULL;
    }
    offs = CCNL_MAX_PACKET_SIZE;

    if(ndn_pkt->type == NDN_TLV_Interest){
        int32_t nonce = 0;
        if (ndn_pkt != NULL && ndn_pkt->s.ndntlv.nonce != NULL) {
            if (ndn_pkt->s.ndntlv.nonce->datalen == 4) {
                memcpy(&nonce, ndn_pkt->s.ndntlv.nonce->data, 4);
            }
        }
        if(nonce == 0){
            nonce = rand();
        }
        len = ccnl_ndntlv_prependInterestCompressed(prefix, &nonce, &offs, tmp);
    }
    else if(ndn_pkt->type == NDN_TLV_Data){
        //DEBUGMSG(DEBUG, "PACKET TYPE: %d\n", ndn_pkt->type);
        len = ccnl_ndntlv_prependContentCompressed(prefix, ndn_pkt->content, ndn_pkt->contlen,
                                         &contentpos, &contentlen, &offs, tmp);

        offs +=4;
        len -=4;

    }
    if (len > 0){
        buf = ccnl_buf_new(tmp + offs, len);
    }
    ccnl_free(tmp);
    assert(buf != NULL);
    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(ndn_pkt);
    if(!pkt){
        ccnl_prefix_free(prefix);
        return NULL;
    }
    if(pkt->buf){
        ccnl_free(pkt->buf);
    }
    ccnl_prefix_free(pkt->pfx);
    pkt->buf = buf;
    pkt->pfx = prefix;
    pkt->pfx->suite = CCNL_SUITE_NDNTLV;
    pkt->type = ndn_pkt->type;
    if(pkt->type == NDN_TLV_Data){
        pkt->content = pkt->buf->data + contentpos;
        pkt->contlen = ndn_pkt->contlen;
    }
    return pkt;
}

struct ccnl_pkt_s *
ccnl_pkt_ndn_decompress(struct ccnl_pkt_s *compressed_pkt)
{
    //TODO: parse nonce?
    struct ccnl_prefix_s *prefix = ccnl_pkt_prefix_decompress(compressed_pkt->pfx);
    prefix->suite = CCNL_SUITE_NDNTLV;

    //use created buf to create packet
    struct ccnl_pkt_s *pkt = ccnl_pkt_dup(compressed_pkt);
    if(!pkt){
        return NULL;
    }
    ccnl_prefix_free(pkt->pfx);
    pkt->pfx = prefix;
    pkt->type = compressed_pkt->type;
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

    if(!name){
        return -1;
    }

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
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Name, name->comp[0],
                                name->complen[0], offset, buf) < 0){
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
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Name, name->comp[0],
                                name->complen[0], offset, buf) < 0){
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependTL(header, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    if (contentpos)
        *contentpos -= (*offset);

    return oldoffset - *offset;
}

struct ccnl_pkt_s*
ccnl_ndntlvCompressed_bytes2pkt(unsigned char **data, int *datalen){

    char header =  *data[0];
    char type = (header & 0x80) >> 7;
    char minSuffixComponets = (header & 0x40) >> 6;
    char maxSuffixComponets = (header & 0x20) >>  5;
    char publisherPublicKey = (header & 0x10) >> 4;
    char exclude = (header & 0x08) >> 3;
    char childSelector = (header & 0x04) >> 2;
    char mustBeFresh = (header & 0x02) >> 1;
    char interestLifetime = header & 0x01;

    (void)minSuffixComponets;
    (void)maxSuffixComponets;
    (void)publisherPublicKey;
    (void)exclude;
    (void)childSelector;
    (void)mustBeFresh;
    (void)interestLifetime;


    struct ccnl_pkt_s *pkt = NULL;
    unsigned char *start = *data;
    (void) start;
    (*data) += 2;
    (*datalen) -= 2;

    int oldpos, len, i;
    (void)oldpos;
    (void)i;
    unsigned int typ;
    struct ccnl_prefix_s *p = 0;

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    if (!pkt){
        return NULL;
    }
    pkt->suite = CCNL_SUITE_NDNTLV;
    if(type == 0){ //interest
        pkt->flags |= CCNL_PKT_REQUEST;
        pkt->type = NDN_TLV_Interest;
    }
    else if(type == 1){ //content
        pkt->type = NDN_TLV_Data;
    }

    pkt->s.ndntlv.scope = 3;
    pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;

    while (ccnl_ndntlv_dehead(data, datalen, (int*) &typ, &len) == 0) {
        unsigned char *cp = *data;
        int len2 = len;
        if(typ == NDN_TLV_Name){
            if(p){
                DEBUGMSG(DEBUG, "Prefix was already set\n");
                continue;
            }
            p = ccnl_prefix_new(CCNL_SUITE_NDNTLV, 1);
            if(!p){
                return NULL;
            }
            p->compcnt = 1;
            p->comp[0] = *data;
            //p->complen = ccnl_malloc(sizeof(int));
            p->complen[0] = len;

            pkt->pfx = p;
        }
        if(typ == NDN_TLV_Nonce){
            pkt->s.ndntlv.nonce = ccnl_buf_new(*data, len);
        }
        if(interestLifetime && typ == NDN_TLV_InterestLifetime){
            //TODO
        }
#ifdef USE_HMAC256
        if(typ == NDN_TLV_SignatureType){
            //TODO
        }
        if(typ == NDN_TLV_SignatureInfo) {
            //TODO
        }
#endif
        if(typ == NDN_TLV_Content || typ == NDN_TLV_NdnlpFragment){
            pkt->content = *data;
            pkt->contlen = len;
        }
        if(typ == NDN_TLV_Selectors) {
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int *) &typ, &i)) {
                    goto Bail;
                }
                if (minSuffixComponets || typ == NDN_TLV_MinSuffixComponents) {
                    pkt->s.ndntlv.minsuffix = ccnl_ndntlv_nonNegInt(cp, i);
                }
                if (maxSuffixComponets || typ == NDN_TLV_MaxSuffixComponents) {
                    pkt->s.ndntlv.maxsuffix = ccnl_ndntlv_nonNegInt(cp, i);
                }
                cp += i;
                len2 -= i;
            }
        }
        if(typ == NDN_TLV_Selectors) {
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i)){
                    goto Bail;
                }
                if (typ == NDN_TLV_ContentType) {
                    // Not used
                    // = ccnl_ndntlv_nonNegInt(cp, i);
                    DEBUGMSG(WARNING, "'ContentType' field ignored\n");
                }
                if (typ == NDN_TLV_FinalBlockId) {
                    if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i))
                        goto Bail;
                    if (typ == NDN_TLV_NameComponent) {
                        // TODO: again, includedNonNeg not yet implemented
                        pkt->val.final_block_id = ccnl_ndntlv_nonNegInt(cp + 1, i - 1);
                    }
                }
                cp += i;
                len2 -= i;
            }
        }
        *data += len;
        *datalen -= len;
    }
    return pkt; //error
Bail:
    ccnl_pkt_free(pkt);
    return NULL;
}

#endif //USE_SUITE_NDNTLV
#endif //USE_SUITE_COMPRESSED
