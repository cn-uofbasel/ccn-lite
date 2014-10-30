/*
 * @f pkt-ndntlv-enc.c
 * @b CCN lite - NDN pkt composing (TLV pkt format March 2014)
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2014-03-05 created
 */

#ifndef PKT_NDNTLV_ENC_C
#define PKT_NDNTLV_ENC_C

#include "pkt-ndntlv.h"

int
ccnl_ndntlv_prependTLval(unsigned long val, int *offset, unsigned char *buf)
{
    int len, i, t;

    if (val < 253)
        len = 0, t = val;
    else if (val <= 0xffff)
        len = 2, t = 253;
    else if (val <= 0xffffffffL)
        len = 4, t = 254;
    else
        len = 8, t = 255;
    if (*offset < (len+1))
        return -1;

    for (i = 0; i < len; i++) {
        buf[--(*offset)] = val & 0xff;
        val = val >> 8;
    }
    buf[--(*offset)] = t;
    return len + 1;
}

int
ccnl_ndntlv_prependTL(int type, unsigned int len,
                      int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    if (ccnl_ndntlv_prependTLval(len, offset, buf) < 0)
        return -1;
    if (ccnl_ndntlv_prependTLval(type, offset, buf) < 0)
        return -1;
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependNonNegInt(int type, unsigned int val,
                             int *offset, unsigned char *buf)
{
    int oldoffset = *offset, len = 0, i;
    static char fill[] = {1, 0, 0, 1, 0, 3, 2, 1, 0};

    while (val) {
        if ((*offset)-- < 1)
            return -1;
        buf[*offset] = (unsigned char) (val & 0xff);
        len++;
        val = val >> 8;
    }
    for (i = fill[len]; i > 0; i--) {
        if ((*offset)-- < 1)
            return -1;
        buf[*offset] = 0;
        len++;
    }
    if (ccnl_ndntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependBlob(int type, unsigned char *blob, int len,
                        int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (*offset < len)
        return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_ndntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf)
{
    int oldoffset = *offset, cnt;

    if(name->chunknum && *name->chunknum >=0) {
        char *chunk_name_component = ccnl_malloc(12 * sizeof(char));
        sprintf(chunk_name_component, "c%i", *name->chunknum);
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
                                    (unsigned char*)chunk_name_component, strlen(chunk_name_component), 
                                    offset, buf) < 0)
            return -1;
    }

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN) {
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
                                (unsigned char*) "NFN", 3, offset, buf) < 0)
            return -1;
        if (name->nfnflags & CCNL_PREFIX_THUNK)
            if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
                                (unsigned char*) "THUNK", 5, offset, buf) < 0)
                return -1;
    }
#endif
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent, name->comp[cnt],
                                    name->complen[cnt], offset, buf) < 0)
            return -1;
    }
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return 0;
}

// ----------------------------------------------------------------------

int
ccnl_ndntlv_fillInterest(struct ccnl_prefix_s *name, int scope, int *nonce,
                         int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    unsigned char lifetime[2] = { 0x0f, 0xa0 };
     unsigned char mustbefresh[2] = { NDN_TLV_MustBeFresh, 0x00 };

    if (scope >= 0) {
        if (scope > 2)
            return -1;
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_Scope, scope, offset, buf) < 0)
            return -1;
    }

    if (ccnl_ndntlv_prependBlob(NDN_TLV_InterestLifetime, lifetime, 2,
                                offset, buf) < 0)
        return -1;

    if (nonce && ccnl_ndntlv_prependBlob(NDN_TLV_Nonce, (unsigned char*) nonce, 4,
                                offset, buf) < 0)
        return -1;

    if (ccnl_ndntlv_prependBlob(NDN_TLV_Selectors, mustbefresh, 2,
                                offset, buf) < 0)
        return -1;

    if (ccnl_ndntlv_prependName(name, offset, buf))
        return -1;
    if (ccnl_ndntlv_prependTL(NDN_TLV_Interest, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

int
ccnl_ndntlv_fillContent(struct ccnl_prefix_s *name, 
                        unsigned char *payload, int paylen, 
                        int *offset, int *contentpos,
                        unsigned char *final_block_id, int final_block_id_len, 
                        unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2;
    unsigned char signatureType[1] = { NDN_SigTypeVal_SignatureSha256WithRsa };

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, 0, offset, buf) < 0)
        return -1;

    // to find length of SignatureInfo
    oldoffset2 = *offset;

    // optional (empty for now) because ndn client lib also puts it in by default
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 0, offset, buf) < 0)
        return -1;

    // use NDN_SigTypeVal_SignatureSha256WithRsa because this is default in ndn client libs
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, signatureType, 1,
                offset, buf)< 0)
        return 1;

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, oldoffset2 - *offset, offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf) < 0)
        return -1;

    // to find length of optional MetaInfo fields
    oldoffset2 = *offset;
    if(final_block_id) {

        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent, final_block_id,
                                    final_block_id_len, offset, buf) < 0)
            return -1;

        // optional
        if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset, offset, buf) < 0)
            return -1;
    }

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset, offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf))
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf) < 0)
           return -1;

    if (contentpos)
        *contentpos -= *offset;

    return oldoffset - *offset;
}

#endif
// eof
