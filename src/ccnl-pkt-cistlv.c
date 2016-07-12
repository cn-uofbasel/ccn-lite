/*
 * @f pkt-cistlv.c
 * @b CCN lite - Cisco pkt parsing and composing (TLV pkt format Jan 6, 2015)
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-01-06 created
 */

#ifdef USE_SUITE_CISTLV

#include "ccnl-pkt-cistlv.h"

// ----------------------------------------------------------------------
// packet decomposition

// convert network order byte array into local unsigned integer value
int
ccnl_cistlv_extractNetworkVarInt(unsigned char *buf, int len,
                                 int *intval)
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
int
ccnl_cistlv_getHdrLen(unsigned char *data, int len)
{
    struct cisco_tlvhdr_201501_s *hp;

    hp = (struct cisco_tlvhdr_201501_s*) data;
    if (len >= hp->hlen)
        return hp->hlen;
    return -1;
}

// parse TL (returned in typ and vallen) and adjust buf and len
int
ccnl_cistlv_dehead(unsigned char **buf, int *len,
                   unsigned int *typ, unsigned int *vallen)
{
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
}

// we use one extraction routine for both interest and data pkts
struct ccnl_pkt_s*
ccnl_cistlv_bytes2pkt(unsigned char *start, unsigned char **data, int *datalen)
{
    struct ccnl_pkt_s *pkt;
    int i;
    unsigned int len, typ, oldpos;
    struct ccnl_prefix_s *p;

    DEBUGMSG(DEBUG, "extracting CISTLV packet (%d, %d bytes)\n",
             (int)(*data - start), *datalen);

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

    pkt->pfx = p = ccnl_prefix_new(CCNL_SUITE_CISTLV, CCNL_MAX_NAME_COMP);
    if (!p) {
        ccnl_free(pkt);
        return NULL;
    }
    p->compcnt = 0;

    // We ignore the TL types of the message for now
    // content and interests are filled in both cases (and only one exists)
    // validation is ignored
    if (ccnl_cistlv_dehead(data, datalen, &typ, &len))
        goto Bail;
    pkt->type = typ;
    pkt->suite = CCNL_SUITE_CISTLV;
    pkt->val.final_block_id = -1;

    oldpos = *data - start;
    while (ccnl_cistlv_dehead(data, datalen, &typ, &len) == 0) {
        unsigned char *cp = *data, *cp2;
        int len2 = len;
        unsigned int len3;

        switch (typ) {
        case CISCO_TLV_Name:
            p->nameptr = start + oldpos;
            while (len2 > 0) {
                cp2 = cp;
                if (ccnl_cistlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;

                if (typ == CISCO_TLV_NameSegment) {
                    // We extract the chunknum to the prefix but keep it in the name component for now
                    // In the future we possibly want to remove the chunk segment from the name components
                    // and rely on the chunknum field in the prefix.
                  p->chunknum = (int*) ccnl_malloc(sizeof(int));

                    if (ccnl_cistlv_extractNetworkVarInt(cp,
                                                         len2, p->chunknum) < 0) {
                        DEBUGMSG(WARNING, "Error in NetworkVarInt for chunk\n");
                        goto Bail;
                    }
                    if (p->compcnt < CCNL_MAX_NAME_COMP) {
                        p->comp[p->compcnt] = cp2;
                        p->complen[p->compcnt] = cp - cp2 + len3;
                        p->compcnt++;
                    } // else out of name component memory: skip
                } else if (typ == CISCO_TLV_NameComponent) {
                    if (p->compcnt < CCNL_MAX_NAME_COMP) {
                        p->comp[p->compcnt] = cp2;
                        p->complen[p->compcnt] = cp - cp2 + len3;
                        p->compcnt++;
                    } // else out of name component memory: skip
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
        case CISCO_TLV_FinalSegmentID:
            if (ccnl_cistlv_extractNetworkVarInt(cp, len2,
                               (int*) &pkt->val.final_block_id) < 0) {
                    DEBUGMSG(WARNING, "error when extracting CISCO_TLV_FinalSegmentID\n");
                    goto Bail;
            }
            break;
        case CISCO_TLV_ContentData:
            pkt->content = *data;
            pkt->contlen = len;
            break;
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

    return pkt;
Bail:
    free_packet(pkt);
    return NULL;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int
ccnl_cistlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
    assert(p);
    assert(p->suite == CCNL_SUITE_CISTLV);

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
ccnl_cistlv_prependTL(unsigned int type, unsigned short len,
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
ccnl_cistlv_prependBlob(unsigned short type, unsigned char *blob,
                        unsigned short len, int *offset, unsigned char *buf)
{
    if (*offset < (len + 4))
        return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_cistlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
}


// write (in network order and with the minimal number of bytes)
// the given integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int
ccnl_cistlv_prependNetworkVarInt(unsigned short type, unsigned int intval,
                                 int *offset, unsigned char *buf)
{
    int offs = *offset;
    int len = 0;

    while (offs > 0) {
        buf[--offs] = intval & 0xff;
        len++;
        if (intval < 128)
            break;
        intval = intval >> 8;
    }
    *offset = offs;

    if (ccnl_cistlv_prependTL(type, len, offset, buf) < 0)
        return -1;
    return len + 4;
}

// write (in network order and with the minimal number of bytes)
// the given integer val *before* position buf[offset], adjust offset
// and return number of bytes prepended. 0 is represented as %x00
int
ccnl_cistlv_prepend4BInt(unsigned short type, unsigned int intval,
                         int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (*offset < 8)
        return -1;

    *offset -= 4;
    *(uint32_t*)(buf + *offset) = htonl(intval);

    if (ccnl_cistlv_prependTL(type, 4, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write *before* position buf[offset] the Cisco fixed header,
// returns total packet len
int
ccnl_cistlv_prependFixedHdr(unsigned char ver,
                            unsigned char packettype,
                            unsigned short payloadlen,
                            unsigned char hoplimit,
                            int *offset, unsigned char *buf)
{
    // optional headers are not yet supported, only the fixed header
    unsigned char hdrlen = sizeof(struct cisco_tlvhdr_201501_s);
    struct cisco_tlvhdr_201501_s *hp;

    if (*offset < hdrlen)
        return -1;

    *offset -= sizeof(struct cisco_tlvhdr_201501_s);
    hp = (struct cisco_tlvhdr_201501_s *)(buf + *offset);
    hp->version = ver;
    hp->pkttype = packettype;
    hp->hoplim = hoplimit;
    hp->flags = 0;
    hp->resvd = 0;
    hp->hlen = hdrlen;
    hp->pktlen = htons(hdrlen + payloadlen);

    return hdrlen + payloadlen;
}

// write given prefix and chunknum *before* buf[offs], adjust offset
// and return bytes used
int
ccnl_cistlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf) {

    int oldoffset = *offset, cnt;

    if (name->chunknum &&
        ccnl_cistlv_prepend4BInt(CISCO_TLV_NameSegment,
                                 *name->chunknum, offset, buf) < 0)
            return -1;

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
        if (*offset < (name->complen[cnt]))
            return -1;
        *offset -= name->complen[cnt];
        memcpy(buf + *offset, name->comp[cnt], name->complen[cnt]);
    }
    if (ccnl_cistlv_prependTL(CISCO_TLV_Name, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return 0;
}

// write Interest payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_cistlv_prependInterest(struct ccnl_prefix_s *name,
                            int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (ccnl_cistlv_prependName(name, offset, buf))
        return -1;
    if (ccnl_cistlv_prependTL(CISCO_TLV_Interest,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_cistlv_prependChunkInterestWithHdr(struct ccnl_prefix_s *name,
                                        int *offset, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 64; // setting hoplimit to max valid value?

    oldoffset = *offset;
    len = ccnl_cistlv_prependInterest(name, offset, buf);
    if (len >= ((1 << 16)-8))
        return -1;
/*
    *offset -= sizeof(struct ccnx_tlvhdr_cisco201501nack_s);
    memset(buf + *offset, 0, sizeof(struct ccnx_tlvhdr_cisco201501nack_s));
*/
    if (ccnl_cistlv_prependFixedHdr(CISCO_TLV_V1, CISCO_TLV_Interest,
                                    len, hoplimit, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Interest packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_cistlv_prependInterestWithHdr(struct ccnl_prefix_s *name,
                                int *offset, unsigned char *buf)
{
    return ccnl_cistlv_prependChunkInterestWithHdr(name, offset, buf);
}

// write Content payload *before* buf[offs], adjust offs and return bytes used
int
ccnl_cistlv_prependContent(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           unsigned int *lastchunknum, int *offset,
                           int *contentpos, unsigned char *buf)
{
    int oldoffset = *offset;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_cistlv_prependBlob(CISCO_TLV_ContentData, payload, paylen,
                                                        offset, buf) < 0)
        return -1;

    if (lastchunknum) {
        if (ccnl_cistlv_prepend4BInt(CISCO_TLV_FinalSegmentID,
                                         *lastchunknum, offset, buf) < 0)
            return -1;
    }

    if (ccnl_cistlv_prependName(name, offset, buf))
        return -1;

    if (ccnl_cistlv_prependTL(CISCO_TLV_Content,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Content packet *before* buf[offs], adjust offs and return bytes used
int
ccnl_cistlv_prependContentWithHdr(struct ccnl_prefix_s *name,
                                  unsigned char *payload, int paylen,
                                  unsigned int *lastchunknum, int *offset,
                                  int *contentpos, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 64;

    oldoffset = *offset;

    len = ccnl_cistlv_prependContent(name, payload, paylen, lastchunknum,
                                     offset, contentpos, buf);

    ccnl_cistlv_prependFixedHdr(CISCO_TLV_V1, CISCO_TLV_Content,
                                len, hoplimit, offset, buf);
    return oldoffset - *offset;
}

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_SUITE_CISTLV

// eof
