/*
 * @f pkt-ccntlv-enc.c
 * @b CCN lite - CCNx pkt composing routines (TLV pkt format Sep 22, 2014)
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

#ifndef PKT_CCNTLV_ENC_C
#define PKT_CCNTLV_ENC_C

#include "pkt-ccntlv.h"

// write given TL *before* position buf+offset, adjust offset and return len
int
ccnl_ccntlv_prependTL(unsigned int type, unsigned short len,
                      int *offset, unsigned char *buf)
{
    unsigned short *ip;

    if (*offset < 4)
        return -1;
    ip = (unsigned short*) (buf + *offset - 2);
    *ip-- = htons(len);
    *ip = htons(type);
    *offset -= 4;
    return 4;
}

// write len bytes *before* position buf[offset], adjust offset
int
ccnl_ccntlv_prependBlob(unsigned short type, unsigned char *blob,
                        unsigned short len, int *offset, unsigned char *buf)
{
    if (*offset < (len + 4))
        return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_ccntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
}


// write (in network order and with the minimal number of bytes)
// the given integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int
ccnl_ccntlv_prependNetworkVarInt(unsigned short type, int intval,
                                 int *offset, unsigned char *buf)
{
    int offs = *offset;
    int len = 0;

    if (intval >= 0) {
        while (offs > 0) {
            buf[--offs] = intval & 0xff;
            len++;
            if (intval < 128)
                break;
            intval = intval >> 8;
        }
        *offset = offs;
    } // else TODO: negative values

    if (ccnl_ccntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
}

// write *before* position buf[offset] the CCNx1.0 fixed header,
// returns total packet len
int
ccnl_ccntlv_prependFixedHdr(unsigned char ver, 
                            unsigned char packettype, 
                            unsigned short payloadlen, 
                            unsigned char hoplimit, 
                            int *offset, unsigned char *buf)
{
    // Currently there are no optional headers, only fixed header of size 8
    unsigned char hdrlen = 8;
    struct ccnx_tlvhdr_ccnx201409_s *hp;

    if (*offset < 8 || payloadlen < 0)
        return -1;
    *offset -= 8;
    hp = (struct ccnx_tlvhdr_ccnx201409_s *)(buf + *offset);
    hp->version = ver;
    hp->packettype = packettype;
    hp->payloadlen = htons(payloadlen);
    hp->hoplimit = hoplimit;
    hp->hdrlen = htons(hdrlen);
    hp->reserved = 0;

    return hdrlen + payloadlen;
}

// write given prefix and chunknum *before* buf[offs], adjust offset
// and return bytes used
int
ccnl_ccntlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf) {

    int oldoffset = *offset, cnt;

    if (name->chunknum >= 0 &&
        ccnl_ccntlv_prependNetworkVarInt(CCNX_TLV_N_Chunk, name->chunknum, offset, buf) < 0)
            return -1;

    // optional: (not used)
    // CCNX_TLV_N_MetaData

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN) {
        if (ccnl_ccntlv_prependBlob(CCNX_TLV_N_NameSegment,
                                (unsigned char*) "NFN", 3, offset, buf) < 0)
            return -1;
        if (name->nfnflags & CCNL_PREFIX_THUNK)
            if (ccnl_ccntlv_prependBlob(CCNX_TLV_N_NameSegment,
                                (unsigned char*) "THUNK", 5, offset, buf) < 0)
                return -1;
    }
#endif
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
        if (ccnl_ccntlv_prependBlob(CCNX_TLV_N_NameSegment, name->comp[cnt],
                                    name->complen[cnt], offset, buf) < 0)
            return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_M_Name, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return 0;
}

// write Interest payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_fillInterest(struct ccnl_prefix_s *name,
                         int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (ccnl_ccntlv_prependName(name, offset, buf))
        return -1;
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Interest,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_fillChunkInterestWithHdr(struct ccnl_prefix_s *name, 
                                     int *offset, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 255; // setting hoplimit to max valid value

    oldoffset = *offset;
    len = ccnl_ccntlv_fillInterest(name, offset, buf);
    if (len >= ((1 << 16)-8))
        return -1;
    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V0, CCNX_TLV_TL_Interest, 
                                    len, hoplimit, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_fillInterestWithHdr(struct ccnl_prefix_s *name, 
                                int *offset, unsigned char *buf)
{
    return ccnl_ccntlv_fillChunkInterestWithHdr(name, offset, buf);
}

// write Content payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_fillContent(struct ccnl_prefix_s *name, 
                        unsigned char *payload, int paylen, 
                        int *lastchunknum,
                        int *offset, int *contentpos,
                        unsigned char *buf)
{
    int tloffset = *offset;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_ccntlv_prependBlob(CCNX_TLV_M_Payload, payload, paylen,
                                                        offset, buf) < 0)
        return -1;

    if (lastchunknum) {
        int oldoffset = *offset;
        if (ccnl_ccntlv_prependNetworkVarInt(CCNX_TLV_M_ENDChunk,
                                             *lastchunknum, offset, buf) < 0)
            return -1;
        if (ccnl_ccntlv_prependTL(CCNX_TLV_M_MetaData,
                                  oldoffset - *offset, offset, buf) < 0)
            return -1;
    }

    if (ccnl_ccntlv_prependName(name, offset, buf))
        return -1;

    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Object,
                                        tloffset - *offset, offset, buf) < 0)
        return -1;

    if (contentpos)
        *contentpos -= *offset;

    return tloffset - *offset;
}

// write Content packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_fillContentWithHdr(struct ccnl_prefix_s *name,
                               unsigned char *payload, int paylen,
                               int *lastchunknum,
                               int *offset, int *contentpos, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 255; // setting to max (conten obj has no hoplimit)

    oldoffset = *offset;

    len = ccnl_ccntlv_fillContent(name, payload, paylen, lastchunknum, offset,
                                  contentpos, buf);

    if (len >= ((1 << 16) - 4))
        return -1;

    ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V0, CCNX_TLV_TL_Object,
                                len, hoplimit, offset, buf);
    return oldoffset - *offset;
}

#endif
// eof
