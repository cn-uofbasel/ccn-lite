/*
 * @f pkt-ccntlv.c
 * @b CCN lite - CCNx pkt parsing and composing (TLV pkt format Sep 22, 2014)
 *
 * Copyright (C) 2014-15, Christian Tschudin, University of Basel
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
 * 2014-11-05 merged from pkt-ccntlv-enc.c pkt-ccntlv-dec.c
 */

#ifdef USE_SUITE_CCNTLV

#ifndef CCNL_LINUXKERNEL
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <assert.h>
#else
#include "../include/ccnl-pkt-ccntlv.h"
#include "../../ccnl-core/include/ccnl-core.h"
#endif



#ifndef DEBUGMSG_PCNX
# define DEBUGMSG_PCNX(...) DEBUGMSG(__VA_ARGS__)
#endif

/* ----------------------------------------------------------------------
Note:
  For a CCNTLV prefix, we store the name components INCLUDING
  the TL. This is different for CCNB and NDNTLV where we only keep
  a component's value bytes, as there is only one "type" of name
  component.
  This might change in the future (on the side of CCNB and NDNTLV prefixes).
*/

// ----------------------------------------------------------------------
// packet decomposition

// convert network order byte array into local unsigned integer value
int8_t
ccnl_ccnltv_extractNetworkVarInt(uint8_t *buf, size_t len,
                                 uint32_t *intval)
{
    uint32_t val = 0;

    if (len > sizeof(uint32_t)) {
        return -1;
    }

    while (len-- > 0) {
        val = (val << 8U) | *buf;
        buf++;
    }

    *intval = val;
    return 0;
}

// returns hdr length to skip before the payload starts.
// this value depends on the type (interest/data/nack) and opt headers
// but should be available in the fixed header
int8_t
ccnl_ccntlv_getHdrLen(uint8_t *data, size_t datalen, size_t *hdrlen)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx2015_s*) data;
    if (datalen >= hp->hdrlen) {
        *hdrlen = hp->hdrlen;
        return 0;
    }
    return -1;
}

int8_t
ccnl_ccntlv_dehead(uint8_t **buf, size_t *len,
                   uint16_t *typ, size_t *vallen)
{
    size_t maxlen = *len;

    if (*len < 4) { //ensure that len is not negative!
        return -1;
    }
    *typ = ((*buf)[0] << 8U) | (*buf)[1];
    *vallen = ((*buf)[2] << 8U) | (*buf)[3];
    *len -= 4;
    *buf += 4;
    if (*vallen > maxlen) {
        return -1; //Return failure (-1) if length value in the tlv is longer than the buffer
    }
    return 0;
}

// We use one extraction procedure for both interest and data pkts.
// This proc assumes that the packet header was already processed and consumed
struct ccnl_pkt_s*
ccnl_ccntlv_bytes2pkt(uint8_t *start, uint8_t **data, size_t *datalen)
{
    struct ccnl_pkt_s *pkt;
    struct ccnl_prefix_s *p;
    uint32_t i;
    size_t len;
    size_t oldpos;
    uint16_t typ;
#ifdef USE_HMAC256
    uint8_t validAlgoIsHmac256 = 0;
#endif

    DEBUGMSG_PCNX(TRACE, "ccnl_ccntlv_bytes2pkt len=%zu\n", *datalen);

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt) {
        return NULL;
    }

    pkt->pfx = p = ccnl_prefix_new(CCNL_SUITE_CCNTLV, CCNL_MAX_NAME_COMP);
    if (!p) {
        ccnl_free(pkt);
        return NULL;
    }
    p->compcnt = 0;

#ifdef USE_HMAC256
    pkt->hmacStart = *data;
#endif
    // We ignore the TL types of the message for now:
    // content and interests are filled in both cases (and only one exists).
    // Validation info is now collected
    if (ccnl_ccntlv_dehead(data, datalen, &typ, &len) || len > *datalen) {
        goto Bail;
    }

    pkt->type = typ;
    pkt->suite = CCNL_SUITE_CCNTLV;
    pkt->val.final_block_id = -1;

    // XXX this parsing is not safe for all input data - needs more bound
    // checks, as some packets with wrong L values can bring this to crash
    oldpos = *data - start;
    while (ccnl_ccntlv_dehead(data, datalen, &typ, &len) == 0) {
        uint8_t *cp = *data, *cp2;
        size_t len2 = len;
        size_t len3;

        if (len > *datalen) {
            goto Bail;
        }
        switch (typ) {
        case CCNX_TLV_M_Name:
            p->nameptr = start + oldpos;
            while (len2 > 0) {
                cp2 = cp;
                if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3) || len>*datalen) {
                    goto Bail;
                }

                switch (typ) {
                case CCNX_TLV_N_Chunk:
                    // We extract the chunknum to the prefix but keep it
                    // in the name component for now. In the future we
                    // possibly want to remove the chunk segment from the
                    // name components and rely on the chunknum field in
                    // the prefix.
                  p->chunknum = (uint32_t *) ccnl_malloc(sizeof(uint32_t));

                    if (ccnl_ccnltv_extractNetworkVarInt(cp, len3, p->chunknum) < 0) {
                        DEBUGMSG_PCNX(WARNING, "Error in NetworkVarInt for chunk\n");
                        goto Bail;
                    }
                    if (p->compcnt < CCNL_MAX_NAME_COMP) {
                        p->comp[p->compcnt] = cp2;
                        p->complen[p->compcnt] = cp - cp2 + len3;
                        p->compcnt++;
                    } // else out of name component memory: skip
                    break;
                case CCNX_TLV_N_NameSegment:
                    if (p->compcnt < CCNL_MAX_NAME_COMP) {
                        p->comp[p->compcnt] = cp2;
                        p->complen[p->compcnt] = cp - cp2 + len3;
                        p->compcnt++;
                    } // else out of name component memory: skip
                    break;
                case CCNX_TLV_N_Meta:
                    if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3) || len > *datalen) {
                        DEBUGMSG_PCNX(WARNING, "error when extracting CCNX_TLV_M_MetaData\n");
                        goto Bail;
                    }
                    break;
                default:
                    break;
                }
                cp += len3;
                len2 -= len3;
            }
            p->namelen = *data - p->nameptr;
            break;
        case CCNX_TLV_M_ENDChunk: {
            uint32_t final_block_id;
            if (ccnl_ccnltv_extractNetworkVarInt(cp, len, &final_block_id) < 0) {
                DEBUGMSG_PCNX(WARNING, "error when extracting CCNX_TLV_M_ENDChunk\n");
                goto Bail;
            }
            pkt->val.final_block_id = final_block_id;
            break;
        }
        case CCNX_TLV_M_Payload:
            pkt->content = *data;
            pkt->contlen = len;
            break;
#ifdef USE_HMAC256
        case CCNX_TLV_TL_ValidationAlgo:
            cp = *data;
            len2 = len;
            if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3) || len > *datalen) {
                goto Bail;
            }
            if (typ == CCNX_VALIDALGO_HMAC_SHA256) {
                // ignore keyId and other algo dependent data ... && len3 == 0)
                validAlgoIsHmac256 = 1;
            }
            break;
        case CCNX_TLV_TL_ValidationPayload:
            if (pkt->hmacStart && validAlgoIsHmac256 && len == 32) {
                pkt->hmacLen = *data - pkt->hmacStart - 4;
                pkt->hmacSignature = *data;
            }
            break;
#endif
        default:
            break;
        }
        *data += len;
        *datalen -= len;
        oldpos = *data - start;
    }
    if (*datalen > 0) {
        goto Bail;
    }

    pkt->pfx = p;
    pkt->buf = ccnl_buf_new(start, *data - start);
    if (!pkt->buf) {
        goto Bail;
    }
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (pkt->content) {
        pkt->content = pkt->buf->data + (pkt->content - start);
    }
    for (i = 0; i < p->compcnt; i++) {
        p->comp[i] = pkt->buf->data + (p->comp[i] - start);
    }
    if (p->nameptr) {
        p->nameptr = pkt->buf->data + (p->nameptr - start);
    }
#ifdef USE_HMAC256
    pkt->hmacStart = pkt->buf->data + (pkt->hmacStart - start);
    pkt->hmacSignature = pkt->buf->data + (pkt->hmacSignature - start);
#endif

    return pkt;
Bail:
    ccnl_pkt_free(pkt);
    return NULL;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int8_t
ccnl_ccntlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
#ifndef CCNL_LINUXKERNEL
    assert(p);
    assert(p->suite == CCNL_SUITE_CCNTLV);
#endif
    if (ccnl_prefix_cmp(c->pkt->pfx, NULL, p->pfx, CMP_EXACT)) {
        return -1;
    }
    // TODO: check keyid
    // TODO: check freshness, kind-of-reply
    return 0;
}

#endif

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

// write given TL *before* position buf+offset, adjust offset and return len
int8_t
ccnl_ccntlv_prependTL(uint16_t type, size_t len,
                      size_t *offset, uint8_t *buf)
{
    if (*offset < 4 || len > UINT16_MAX) {
        return -1;
    }

    buf += *offset;
    *offset -= 4;

    *(buf-1) = (uint8_t) (len & 0xffU);
    *(buf-2) = (uint8_t) (len & 0xff00U) >> 8U;
    *(buf-3) = (uint8_t) (type & 0xffU);
    *(buf-4) = (uint8_t) (type & 0xff00U) >> 8U;

    return 0;

}

// write len bytes *before* position buf[offset], adjust offset
int8_t
ccnl_ccntlv_prependBlob(uint16_t type, uint8_t *blob,
                        size_t len, size_t *offset, uint8_t *buf)
{
    if (*offset < (len + 4)) {
        return -1;
    }
    *offset -= len;
    memcpy(buf + *offset, blob, len);

    if (ccnl_ccntlv_prependTL(type, len, offset, buf)) {
        return -1;
    }
    return 0;
}

// write (in network order and with the minimal number of bytes)
// the given unsigned integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int8_t
ccnl_ccntlv_prependNetworkVarUInt(uint16_t type, uint32_t intval,
                                  size_t *offset, uint8_t *buf)
{
    size_t offs = *offset;
    size_t len = 0;

    while (offs > 0) {
        buf[--offs] = (uint8_t) (intval & 0xffU);
        len++;
        intval = intval >> 8U;
        if (!intval) {
            break;
        }
    }
    *offset = offs;

    if (ccnl_ccntlv_prependTL(type, len, offset, buf)) {
        return -1;
    }
    return 0;
}

#ifdef XXX
// write (in network order and with the minimal number of bytes)
// the given signed integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int
ccnl_ccntlv_prependNetworkVarSInt(unsigned short type, int intval,
                                  int *offset, unsigned char *buf)
{
 ...
}
#endif

// write *before* position buf[offset] the CCNx1.0 fixed header,
// returns 0 on success, non-zero on failure.
int8_t
ccnl_ccntlv_prependFixedHdr(uint8_t ver,
                            uint8_t packettype,
                            size_t payloadlen,
                            uint8_t hoplimit,
                            size_t *offset, uint8_t *buf)
{
    // optional headers are not yet supported, only the fixed header
    uint8_t hdrlen = sizeof(struct ccnx_tlvhdr_ccnx2015_s);
    size_t pktlen = hdrlen + payloadlen;
    if (pktlen > UINT16_MAX) {
        return -1;
    }
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    if (*offset < hdrlen) {
        return -1;
    }

    *offset -= sizeof(struct ccnx_tlvhdr_ccnx2015_s);
    hp = (struct ccnx_tlvhdr_ccnx2015_s *)(buf + *offset);
    hp->version = ver;
    hp->pkttype = packettype;
    hp->hoplimit = hoplimit;
    memset(hp->fill, 0, 2);

    hp->hdrlen = hdrlen;
    hp->pktlen = htons((uint16_t) pktlen);

    return 0;
}

// write given prefix and chunknum *before* buf[offs], adjust offset.
// Returns 0 on success, non-zero on failure.
int8_t
ccnl_ccntlv_prependName(struct ccnl_prefix_s *name,
                        size_t *offset, uint8_t *buf,
                        const uint32_t *lastchunknum) {

    size_t cnt;
    size_t nameend;

    if (lastchunknum) {
        if (ccnl_ccntlv_prependNetworkVarUInt(CCNX_TLV_M_ENDChunk,
                                              *lastchunknum, offset, buf)) {
            return -1;
        }
    }

    nameend = *offset;

    if (name->chunknum &&
        ccnl_ccntlv_prependNetworkVarUInt(CCNX_TLV_N_Chunk,
                                          *name->chunknum, offset, buf)) {
        return -1;
    }

    // optional: (not used)
    // CCNX_TLV_N_Meta

    for (cnt = name->compcnt; cnt > 0; cnt--) {
        if (*offset < name->complen[cnt-1]) {
            return -1;
        }
        *offset -= name->complen[cnt-1];
        memcpy(buf + *offset, name->comp[cnt-1], name->complen[cnt-1]);
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_M_Name, nameend - *offset,
                              offset, buf)) {
        return -1;
    }

    return 0;
}

// write Interest payload *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependInterest(struct ccnl_prefix_s *name,
                            size_t *offset, uint8_t *buf, size_t *len)
{
    size_t oldoffset = *offset;

    if (ccnl_ccntlv_prependName(name, offset, buf, NULL)) {
        return -1;
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Interest,
                                        oldoffset - *offset, offset, buf)) {
        return -1;
    }

    *len = oldoffset - *offset;
    return 0;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependChunkInterestWithHdr(struct ccnl_prefix_s *name,
                                        size_t *offset, uint8_t *buf, size_t *reslen)
{
    size_t len, oldoffset;
    uint8_t hoplimit = 64; // setting hoplimit to max valid value?

    oldoffset = *offset;
    if (ccnl_ccntlv_prependInterest(name, offset, buf, &len)) {
        return -1;
    }
    if (len > (UINT16_MAX - sizeof(struct ccnx_tlvhdr_ccnx2015_s))) {
        return -1;
    }

    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Interest,
                                    len, hoplimit, offset, buf)) {
        return -1;
    }

    *reslen = oldoffset - *offset;
    return 0;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependInterestWithHdr(struct ccnl_prefix_s *name,
                                   size_t *offset, uint8_t *buf, size_t *len)
{
    return ccnl_ccntlv_prependChunkInterestWithHdr(name, offset, buf, len);
}

// write Content payload *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependContent(struct ccnl_prefix_s *name,
                           uint8_t *payload, size_t paylen,
                           uint32_t *lastchunknum, size_t *contentpos,
                           size_t *offset, uint8_t *buf, size_t *reslen)
{
    size_t tloffset = *offset;

    if (contentpos) {
        *contentpos = *offset - paylen;
    }

    // fill in backwards
    if (ccnl_ccntlv_prependBlob(CCNX_TLV_M_Payload, payload, paylen, offset, buf)) {
        return -1;
    }

    if (ccnl_ccntlv_prependName(name, offset, buf, lastchunknum)) {
        return -1;
    }

    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Object, tloffset - *offset, offset, buf)) {
        return -1;
    }

    *reslen = tloffset - *offset;
    return 0;
}

// write Content packet *before* buf[offs], adjust offs and return bytes used
int8_t
ccnl_ccntlv_prependContentWithHdr(struct ccnl_prefix_s *name,
                                  uint8_t *payload, size_t paylen,
                                  uint32_t *lastchunknum, size_t *contentpos,
                                  size_t *offset, uint8_t *buf, size_t *reslen)
{
    size_t len, oldoffset;
    uint8_t hoplimit = 255; // setting to max (content has no hoplimit)

    oldoffset = *offset;

    if (ccnl_ccntlv_prependContent(name, payload, paylen, lastchunknum,
                                   contentpos, offset, buf, &len)) {
        return -1;
    }

    if (len > (UINT16_MAX - sizeof(struct ccnx_tlvhdr_ccnx2015_s))) {
        return -1;
    }

    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                    len, hoplimit, offset, buf)) {
        return -1;
    }

    *reslen = oldoffset - *offset;
    return 0;
}

#ifdef USE_FRAG

// produces a full FRAG packet. It does not write, just read the fields in *fr
struct ccnl_buf_s*
ccnl_ccntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed)
{
    size_t datalen;
    struct ccnl_buf_s *buf;
    struct ccnx_tlvhdr_ccnx2015_s *fp;
    uint16_t tmp;

    DEBUGMSG_PCNX(TRACE, "ccnl_ccntlv_mkFrag seqno=%u\n", fr->sendseq);

    datalen = fr->mtu - sizeof(*fp) - 4;
    if (datalen > (fr->bigpkt->datalen - fr->sendoffs)) {
        datalen = fr->bigpkt->datalen - fr->sendoffs;
    }
    if (datalen > UINT16_MAX) {
        return NULL;
    }

    buf = ccnl_buf_new(NULL, sizeof(*fp) + 4 + datalen);
    if (!buf) {
        return NULL;
    }
    fp = (struct ccnx_tlvhdr_ccnx2015_s*) buf->data;
    memset(fp, 0, sizeof(*fp));
    fp->version = CCNX_TLV_V1;
    fp->pkttype = CCNX_PT_Fragment;
    fp->hdrlen = sizeof(*fp);
    fp->pktlen = htons(sizeof(*fp) + 4 + datalen);

    tmp = htons(CCNX_TLV_TL_Fragment);
    memcpy(fp+1, &tmp, 2);
    tmp = htons((uint16_t) datalen);
    memcpy((char*)(fp+1) + 2, &tmp, 2);
    memcpy((char*)(fp+1) + 4, fr->bigpkt->data + fr->sendoffs, datalen);

    tmp = fr->sendseq & 0x03fff;
    if (datalen >= fr->bigpkt->datalen) {   // single
        tmp |= CCNL_BEFRAG_FLAG_SINGLE << 14;
    } else if (fr->sendoffs == 0) {          // start
        tmp |= CCNL_BEFRAG_FLAG_FIRST  << 14;
    } else if(datalen >= (fr->bigpkt->datalen - fr->sendoffs)) { // end
        tmp |= CCNL_BEFRAG_FLAG_LAST   << 14;
    } else {                                  // middle
        tmp |= CCNL_BEFRAG_FLAG_MID    << 14;
    }
    tmp = htons(tmp);
    memcpy(fp->fill, &tmp, 2);

    *consumed = datalen;
    return buf;
}
#endif

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_SUITE_CCNTLV

/* suppress empty translation unit error */
typedef int unused_typedef;

// eof
