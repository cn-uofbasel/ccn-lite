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

#include "ccnl-pkt-ccntlv.h"

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
int
ccnl_ccnltv_extractNetworkVarInt(unsigned char *buf, int len,
                                 unsigned int *intval)
{
    int val = 0;

    while (len-- > 0) {
        val = (val << 8) | *buf;
        buf++;
    }
    *intval = val;

    return 0;
}

// returns hdr length to skip before the payload starts.
// this value depends on the type (interest/data/nack) and opt headers
// but should be available in the fixed header
int
ccnl_ccntlv_getHdrLen(unsigned char *data, int len)
{
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx2015_s*) data;
    if (len >= hp->hdrlen)
        return hp->hdrlen;
    return -1;
}

// parse TL (returned in typ and vallen) and adjust buf and len
int
ccnl_ccntlv_dehead(unsigned char **buf, int *len,
                   unsigned int *typ, unsigned int *vallen)
{
// Avoiding casting pointers to uint16t -- issue with the RFduino compiler?
// Workaround:
    uint16_t tmp;

    if (*len < 4)
        return -1;
    memcpy(&tmp, *buf, 2);
    *typ = ntohs(tmp);
    memcpy(&tmp, *buf + 2, 2);
    *vallen = ntohs(tmp);
    *len -= 4;
    *buf += 4;
    return 0;
/*
    unsigned short *ip;

    if (*len < 4)
        return -1;
    ip = (unsigned short*) *buf;
    *typ = ntohs(*ip);
    ip++;
    *vallen = ntohs(*ip);
    *len -= 4;
    *buf += 4;
    return 0;
*/
}

// We use one extraction procedure for both interest and data pkts.
// This proc assumes that the packet header was already processed and consumed
struct ccnl_pkt_s*
ccnl_ccntlv_bytes2pkt(unsigned char *start, unsigned char **data, int *datalen)
{
    struct ccnl_pkt_s *pkt;
    int i, len;
    unsigned int typ, oldpos;
    struct ccnl_prefix_s *p;
#ifdef USE_HMAC256
    int validAlgoIsHmac256 = 0;
#endif

    DEBUGMSG_PCNX(TRACE, "ccnl_ccntlv_bytes2pkt len=%d\n", *datalen);

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

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
    if (ccnl_ccntlv_dehead(data, datalen, &typ, (unsigned int*) &len) || (int) len > *datalen)
        goto Bail;

    pkt->type = typ;
    pkt->suite = CCNL_SUITE_CCNTLV;
    pkt->val.final_block_id = -1;

    // XXX this parsing is not safe for all input data - needs more bound
    // checks, as some packets with wrong L values can bring this to crash
    oldpos = *data - start;
    while (ccnl_ccntlv_dehead(data, datalen, &typ, (unsigned int*) &len) == 0) {
        unsigned char *cp = *data, *cp2;
        int len2 = len;
        int len3;

        if ( (int)len > *datalen)
            goto Bail;
        switch (typ) {
        case CCNX_TLV_M_Name:
            p->nameptr = start + oldpos;
            while (len2 > 0) {
                cp2 = cp;
                if (ccnl_ccntlv_dehead(&cp, &len2, &typ, (unsigned int*) &len3) || (int)len>*datalen)
                    goto Bail;

                switch (typ) {
                case CCNX_TLV_N_Chunk:
                    // We extract the chunknum to the prefix but keep it
                    // in the name component for now. In the future we
                    // possibly want to remove the chunk segment from the
                    // name components and rely on the chunknum field in
                    // the prefix.
                  p->chunknum = (int*) ccnl_malloc(sizeof(int));

                    if (ccnl_ccnltv_extractNetworkVarInt(cp, len3,
                                                         (unsigned int*) p->chunknum) < 0) {
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
                    if (ccnl_ccntlv_dehead(&cp, &len2, &typ, (unsigned int*) &len3) ||
                        (int)len > *datalen) {
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
#ifdef USE_NFN
            if (p->compcnt > 0 && p->complen[p->compcnt-1] == 7 &&
                    !memcmp(p->comp[p->compcnt-1], "\x00\x01\x00\x03NFN", 7)) {
                p->nfnflags |= CCNL_PREFIX_NFN;
                p->compcnt--;
            }
#ifdef USE_TIMEOUT_KEEPALIVE
            if (p->compcnt > 0 && p->complen[p->compcnt-1] == 9 &&
                    !memcmp(p->comp[p->compcnt-1], "\x00\x01\x00\x05ALIVE", 9)) {
                p->nfnflags |= CCNL_PREFIX_KEEPALIVE;
                p->compcnt--;
            }
#endif
#endif
            break;
        case CCNX_TLV_M_ENDChunk:
            if (ccnl_ccnltv_extractNetworkVarInt(cp, len,
                              (unsigned int*) &(pkt->val.final_block_id)) < 0) {
                DEBUGMSG_PCNX(WARNING, "error when extracting CCNX_TLV_M_ENDChunk\n");
                goto Bail;
            }
            break;
        case CCNX_TLV_M_Payload:
            pkt->content = *data;
            pkt->contlen = len;
            break;
#ifdef USE_HMAC256
        case CCNX_TLV_TL_ValidationAlgo:
            cp = *data;
            len2 = len;
            if (ccnl_ccntlv_dehead(&cp, &len2, &typ, (unsigned*) &len3) || len>*datalen)
                goto Bail;
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
    if (*datalen > 0)
        goto Bail;

    pkt->pfx = p;
    pkt->buf = ccnl_buf_new(start, *data - start);
    if (!pkt->buf)
        goto Bail;
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (pkt->content)
        pkt->content = pkt->buf->data + (pkt->content - start);
    for (i = 0; i < p->compcnt; i++)
        p->comp[i] = pkt->buf->data + (p->comp[i] - start);
    if (p->nameptr)
        p->nameptr = pkt->buf->data + (p->nameptr - start);
#ifdef USE_HMAC256
    pkt->hmacStart = pkt->buf->data + (pkt->hmacStart - start);
    pkt->hmacSignature = pkt->buf->data + (pkt->hmacSignature - start);
#endif

    return pkt;
Bail:
    free_packet(pkt);
    return NULL;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int
ccnl_ccntlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
    assert(p);
    assert(p->suite == CCNL_SUITE_CCNTLV);

    if (ccnl_prefix_cmp(c->pkt->pfx, NULL, p->pfx, CMP_EXACT))
        return -1;
    // TODO: check keyid
    // TODO: check freshness, kind-of-reply
    return 0;
}

#endif

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

// write given TL *before* position buf+offset, adjust offset and return len
int
ccnl_ccntlv_prependTL(unsigned short type, unsigned short len,
                      int *offset, unsigned char *buf)
{
// RFduino compiler has problems with casting pointers to unsigned short?
// Workaround:
    uint16_t tmp;
    buf += *offset;
    tmp = htons(len);
    memcpy(buf-2, &tmp, 2);
    tmp = htons(type);
    memcpy(buf-4, &tmp, 2);
    *offset -= 4;
    return 4;
/*
    unsigned short *ip;

    if (*offset < 4)
        return -1;
    ip = (unsigned short*) (buf + *offset - 2);
    *ip-- = htons(len);
    *ip = htons(type);
    *offset -= 4;
    return 4;
*/
}

// write len bytes *before* position buf[offset], adjust offset
int
ccnl_ccntlv_prependBlob(unsigned short type, unsigned char *blob,
                        unsigned short len, int *offset, unsigned char *buf)
{
    if (*offset < (len + 4))
        return -1;
    *offset -= len;
    memcpy(buf + *offset, blob, len);

    if (ccnl_ccntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
}

// write (in network order and with the minimal number of bytes)
// the given unsigned integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int
ccnl_ccntlv_prependNetworkVarUInt(unsigned short type, unsigned int intval,
                                  int *offset, unsigned char *buf)
{
    int offs = *offset;
    int len = 0;

    while (offs > 0) {
        buf[--offs] = intval & 0xff;
        len++;
        intval = intval >> 8;
        if (!intval)
            break;
    }
    *offset = offs;

    if (ccnl_ccntlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
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
// returns total packet len
int
ccnl_ccntlv_prependFixedHdr(unsigned char ver,
                            unsigned char packettype,
                            unsigned short payloadlen,
                            unsigned char hoplimit,
                            int *offset, unsigned char *buf)
{
    // optional headers are not yet supported, only the fixed header
    unsigned char hdrlen = sizeof(struct ccnx_tlvhdr_ccnx2015_s);
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    if (*offset < hdrlen)
        return -1;

    *offset -= sizeof(struct ccnx_tlvhdr_ccnx2015_s);
    hp = (struct ccnx_tlvhdr_ccnx2015_s *)(buf + *offset);
    hp->version = ver;
    hp->pkttype = packettype;
    hp->hoplimit = hoplimit;
    memset(hp->fill, 0, 2);

    hp->hdrlen = hdrlen; // htons(hdrlen);
    hp->pktlen = htons(hdrlen + payloadlen);

    return hdrlen + payloadlen;
}

// write given prefix and chunknum *before* buf[offs], adjust offset
// and return bytes used
int
ccnl_ccntlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf,
                        unsigned int *lastchunknum) {

    int nameend, cnt;

    if (lastchunknum) {
        if (ccnl_ccntlv_prependNetworkVarUInt(CCNX_TLV_M_ENDChunk,
                                              *lastchunknum, offset, buf) < 0)
            return -1;
    }

    nameend = *offset;

    if (name->chunknum &&
        ccnl_ccntlv_prependNetworkVarUInt(CCNX_TLV_N_Chunk,
                                          *name->chunknum, offset, buf) < 0)
            return -1;

    // optional: (not used)
    // CCNX_TLV_N_Meta

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN)
        if (ccnl_pkt_prependComponent(name->suite, "NFN", offset, buf) < 0)
            return -1;
#ifdef USE_TIMEOUT_KEEPALIVE
    if (name->nfnflags & CCNL_PREFIX_KEEPALIVE)
        if (ccnl_pkt_prependComponent(name->suite, "ALIVE", offset, buf) < 0)
            return -1;
#endif
#endif
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
        if (*offset < name->complen[cnt])
            return -1;
        *offset -= name->complen[cnt];
        memcpy(buf + *offset, name->comp[cnt], name->complen[cnt]);
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_M_Name, nameend - *offset,
                              offset, buf) < 0)
        return -1;

    return 0;
}

// write Interest payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependInterest(struct ccnl_prefix_s *name,
                            int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (ccnl_ccntlv_prependName(name, offset, buf, NULL))
        return -1;
    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Interest,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependChunkInterestWithHdr(struct ccnl_prefix_s *name,
                                        int *offset, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 64; // setting hoplimit to max valid value?

    oldoffset = *offset;
    len = ccnl_ccntlv_prependInterest(name, offset, buf);
    if ( (unsigned int)len >= ((1 << 16) - sizeof(struct ccnx_tlvhdr_ccnx2015_s)))
        return -1;

    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Interest,
                                    len, hoplimit, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependInterestWithHdr(struct ccnl_prefix_s *name,
                                int *offset, unsigned char *buf)
{
    return ccnl_ccntlv_prependChunkInterestWithHdr(name, offset, buf);
}

// write Content payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependContent(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           unsigned int *lastchunknum, int *contentpos,
                           int *offset, unsigned char *buf)
{
    int tloffset = *offset;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_ccntlv_prependBlob(CCNX_TLV_M_Payload, payload, paylen,
                                                        offset, buf) < 0)
        return -1;

    if (ccnl_ccntlv_prependName(name, offset, buf, lastchunknum))
        return -1;

    if (ccnl_ccntlv_prependTL(CCNX_TLV_TL_Object,
                                        tloffset - *offset, offset, buf) < 0)
        return -1;

    return tloffset - *offset;
}

// write Content packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_ccntlv_prependContentWithHdr(struct ccnl_prefix_s *name,
                                  unsigned char *payload, int paylen,
                                  unsigned int *lastchunknum, int *contentpos,
                                  int *offset, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 255; // setting to max (content has no hoplimit)

    oldoffset = *offset;

    len = ccnl_ccntlv_prependContent(name, payload, paylen, lastchunknum,
                                     contentpos, offset, buf);
    if (len < 0)
        return -1;

    if ((unsigned int)len >= ((uint32_t)1 << 16) - sizeof(struct ccnx_tlvhdr_ccnx2015_s))
        return -1;

    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V1, CCNX_PT_Data,
                                    len, hoplimit, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

#ifdef USE_FRAG

// produces a full FRAG packet. It does not write, just read the fields in *fr
struct ccnl_buf_s*
ccnl_ccntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed)
{
    unsigned int datalen;
    struct ccnl_buf_s *buf;
    struct ccnx_tlvhdr_ccnx2015_s *fp;
    uint16_t tmp;

    DEBUGMSG_PCNX(TRACE, "ccnl_ccntlv_mkFrag seqno=%d\n", fr->sendseq);

    datalen = fr->mtu - sizeof(*fp) - 4;
    if (datalen > (fr->bigpkt->datalen - fr->sendoffs))
        datalen = fr->bigpkt->datalen - fr->sendoffs;

    buf = ccnl_buf_new(NULL, sizeof(*fp) + 4 + datalen);
    if (!buf)
        return 0;
    fp = (struct ccnx_tlvhdr_ccnx2015_s*) buf->data;
    memset(fp, 0, sizeof(*fp));
    fp->version = CCNX_TLV_V1;
    fp->pkttype = CCNX_PT_Fragment;
    fp->hdrlen = sizeof(*fp);
    fp->pktlen = htons(sizeof(*fp) + 4 + datalen);

    tmp = htons(CCNX_TLV_TL_Fragment);
    memcpy(fp+1, &tmp, 2);
    tmp = htons(datalen);
    memcpy((char*)(fp+1) + 2, &tmp, 2);
    memcpy((char*)(fp+1) + 4, fr->bigpkt->data + fr->sendoffs, datalen);

    tmp = fr->sendseq & 0x03fff;
    if ((int) datalen >= fr->bigpkt->datalen)   // single
        tmp |= CCNL_BEFRAG_FLAG_SINGLE << 14;
    else if (fr->sendoffs == 0)           // start
        tmp |= CCNL_BEFRAG_FLAG_FIRST  << 14;
    else if(datalen >= (fr->bigpkt->datalen - fr->sendoffs))  // end
        tmp |= CCNL_BEFRAG_FLAG_LAST   << 14;
    else                                  // middle
        tmp |= CCNL_BEFRAG_FLAG_MID    << 14;
    tmp = htons(tmp);
    memcpy(fp->fill, &tmp, 2);

    *consumed = datalen;
    return buf;
}
#endif

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_SUITE_CCNTLV

// eof
