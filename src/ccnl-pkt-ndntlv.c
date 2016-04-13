/*
 * @f pkt-ndntlv.c
 * @b CCN lite - NDN pkt decoding and composition (TLV pkt format March 2014)
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
 * 2014-11-05 merged from pkt-ndntlv-enc.c pkt-ndntlv-dec.c
 */

#include "ccnl-pkt-ndntlv.h"

#ifdef USE_SUITE_NDNTLV

// ----------------------------------------------------------------------
// packet parsing

static int
ccnl_ndntlv_varlenint(unsigned char **buf, int *len, int *val)
{

    if (**buf < 253 && *len >= 1) {
        *val = **buf;
        *buf += 1;
        *len -= 1;
    } else if (**buf == 253 && *len >= 3) { // 2 bytes
        *val = ntohs(*(uint16_t*)(*buf + 1));
        *buf += 3;
        *len -= 3;
    } else if (**buf == 254 && *len >= 5) { // 4 bytes
        *val = ntohl(*(uint32_t*)(*buf + 1));
        *buf += 5;
        *len -= 5;
    } else {
        // not implemented yet (or too short)
        return -1;
    }
    return 0;
}

unsigned long int
ccnl_ndntlv_nonNegInt(unsigned char *cp, int len)
{
    unsigned long int val = 0;

    while (len-- > 0) {
        val = (val << 8) | *cp;
        cp++;
    }
    return val;
}

int
ccnl_ndntlv_dehead(unsigned char **buf, int *len,
                   int *typ, int *vallen)
{
  if (ccnl_ndntlv_varlenint(buf, len, (int*) typ))
        return -1;
  if (ccnl_ndntlv_varlenint(buf, len, (int*) vallen))
        return -1;
    return 0;
}

// we use one extraction routine for each of interest, data and fragment pkts
struct ccnl_pkt_s*
ccnl_ndntlv_bytes2pkt(unsigned int pkttype, unsigned char *start,
                      unsigned char **data, int *datalen)
{
    struct ccnl_pkt_s *pkt;
    int oldpos, len, i;
    unsigned int typ;
    struct ccnl_prefix_s *p = 0;
#ifdef USE_HMAC256
    int validAlgoIsHmac256 = 0;
#endif

    DEBUGMSG(DEBUG, "ccnl_ndntlv_bytes2pkt len=%d\n", *datalen);

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

#ifdef USE_HMAC256
    pkt->hmacStart = start;
#endif
    switch(pkttype) {
    case NDN_TLV_Interest:
        pkt->flags |= CCNL_PKT_REQUEST;
        break;
    case NDN_TLV_Data:
        pkt->flags |= CCNL_PKT_REPLY;
        break;
#ifdef USE_FRAG
    case NDN_TLV_Fragment:
        pkt->flags |= CCNL_PKT_FRAGMENT;
        break;
#endif
    default:
        DEBUGMSG(INFO, "  ndntlv: unknown packet type %d\n", pkttype);
        goto Bail;
    }

    pkt->suite = CCNL_SUITE_NDNTLV;
    pkt->s.ndntlv.scope = 3;
    pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;

    oldpos = *data - start;
    while (ccnl_ndntlv_dehead(data, datalen, (int*) &typ, &len) == 0) {
        unsigned char *cp = *data;
        int len2 = len;

        switch (typ) {
        case NDN_TLV_Name:
            if (p) {
                DEBUGMSG(WARNING, " ndntlv: name already defined\n");
                goto Bail;
            }
            p = ccnl_prefix_new(CCNL_SUITE_NDNTLV, CCNL_MAX_NAME_COMP);
            if (!p)
                goto Bail;
            p->compcnt = 0;
            pkt->pfx = p;
            pkt->val.final_block_id = -1;

            p->nameptr = start + oldpos;
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i))
                    goto Bail;
                if (typ == NDN_TLV_NameComponent &&
                            p->compcnt < CCNL_MAX_NAME_COMP) {
                    if(cp[0] == NDN_Marker_SegmentNumber) {
                      p->chunknum = (int*) ccnl_malloc(sizeof(int));
                        // TODO: requires ccnl_ndntlv_includedNonNegInt which includes the length of the marker
                        // it is implemented for encode, the decode is not yet implemented
                        *p->chunknum = ccnl_ndntlv_nonNegInt(cp + 1, i - 1);
                    }
                    p->comp[p->compcnt] = cp;
                    p->complen[p->compcnt] = i;
                    p->compcnt++;
                }  // else unknown type: skip
                cp += i;
                len2 -= i;
            }
            p->namelen = *data - p->nameptr;
    #ifdef USE_NFN
            if (p->compcnt > 0 && p->complen[p->compcnt-1] == 3 &&
                    !memcmp(p->comp[p->compcnt-1], "NFN", 3)) {
                p->nfnflags |= CCNL_PREFIX_NFN;
                p->compcnt--;
            }
    #endif
            break;
        case NDN_TLV_Selectors:
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i))
                    goto Bail;
                switch(typ) {
                case NDN_TLV_MinSuffixComponents:
                    pkt->s.ndntlv.minsuffix = ccnl_ndntlv_nonNegInt(cp, i);
                    break;
                case NDN_TLV_MaxSuffixComponents:
//                    fprintf(stderr, "setting to %d\n", (int)ccnl_ndntlv_nonNegInt(cp, i));
                    pkt->s.ndntlv.maxsuffix = ccnl_ndntlv_nonNegInt(cp, i);
                    break;
                case NDN_TLV_MustBeFresh:
                    pkt->s.ndntlv.mbf = 1;
                    break;
                case NDN_TLV_Exclude:
                    DEBUGMSG(WARNING, "'Exclude' field ignored\n");
                    break;
                default:
                    break;
                }
                cp += i;
                len2 -= i;
            }
            break;
        case NDN_TLV_Nonce:
            pkt->s.ndntlv.nonce = ccnl_buf_new(*data, len);
            break;
        case NDN_TLV_Scope:
            pkt->s.ndntlv.scope = ccnl_ndntlv_nonNegInt(*data, len);
            break;
        case NDN_TLV_Content:
        case NDN_TLV_NdnlpFragment: // payload
            pkt->content = *data;
            pkt->contlen = len;
            break;
        case NDN_TLV_MetaInfo:
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i))
                    goto Bail;
                if (typ == NDN_TLV_ContentType) {
                    // Not used
                    // = ccnl_ndntlv_nonNegInt(cp, i);
                    DEBUGMSG(WARNING, "'ContentType' field ignored\n");
                }
                if (typ == NDN_TLV_FreshnessPeriod) {
                    // Not used
                    // = ccnl_ndntlv_nonNegInt(cp, i);
                    DEBUGMSG(WARNING, "'FreshnessPeriod' field ignored\n");
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
            break;
        case NDN_TLV_Frag_BeginEndFields:
            pkt->val.seqno = ccnl_ndntlv_nonNegInt(*data, len);
            DEBUGMSG(TRACE, "  frag: %04x\n", pkt->val.seqno);
            if (pkt->val.seqno & 0x4000)
                pkt->flags |= CCNL_PKT_FRAG_BEGIN;
            if (pkt->val.seqno & 0x8000)
                pkt->flags |= CCNL_PKT_FRAG_END;
            pkt->val.seqno &= 0x3fff;
            break;
#ifdef USE_HMAC256
        case NDN_TLV_SignatureInfo:
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, (int*) &typ, &i))
                    goto Bail;
                if (typ == NDN_TLV_SignatureType && i == 1 &&
                                          *cp == NDN_VAL_SIGTYPE_HMAC256) {
                    validAlgoIsHmac256 = 1;
                    break;
                }
                cp += i;
                len2 -= i;
            }
            break;
        case NDN_TLV_SignatureValue:
            if (pkt->hmacStart && validAlgoIsHmac256 && len == 32) {
                pkt->hmacLen = oldpos;
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
    if (p) {
        for (i = 0; i < p->compcnt; i++)
            p->comp[i] = pkt->buf->data + (p->comp[i] - start);
        if (p->nameptr)
            p->nameptr = pkt->buf->data + (p->nameptr - start);
    }

    return pkt;
Bail:
    free_packet(pkt);
    return NULL;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int
ccnl_ndntlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
    assert(p);
    assert(p->suite == CCNL_SUITE_NDNTLV);

    if (!ccnl_i_prefixof_c(p->pfx, p->s.ndntlv.minsuffix, p->s.ndntlv.maxsuffix, c))
        return -1;
    // FIXME: should check freshness (mbf) here
    // if (mbf) // honor "answer-from-existing-content-store" flag
    DEBUGMSG(DEBUG, "  matching content for interest, content %p\n",
                     (void *) c);
    return 0;
}

#endif

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

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
ccnl_ndntlv_prependNonNegIntVal(unsigned int val,
                                int *offset, unsigned char *buf) {
    int len = 0, i;
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
    return len;
}

int
ccnl_ndntlv_prependNonNegInt(int type,
                             unsigned int val,
                             int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    if (ccnl_ndntlv_prependNonNegIntVal(val, offset, buf) < 0)
        return -1;
    if (ccnl_ndntlv_prependTL(type, oldoffset - *offset, offset, buf) < 0)
        return -1;
    return oldoffset - *offset;
}


int
ccnl_ndntlv_prependIncludedNonNegInt(int type, unsigned int val,
                                     char marker,
                                     int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    if (ccnl_ndntlv_prependNonNegIntVal(val, offset, buf) < 0)
        return -1;

    if((*offset)-- < 1)
        return -1;
    buf[*offset] = marker;

    if (ccnl_ndntlv_prependTL(type, oldoffset - *offset, offset, buf) < 0)
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

    if(name->chunknum) {
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *name->chunknum,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf) < 0)
            return -1;
    }

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN) {
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent,
                                (unsigned char*) "NFN", 3, offset, buf) < 0)
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
ccnl_ndntlv_prependInterest(struct ccnl_prefix_s *name, int scope, int *nonce,
                            int *offset, unsigned char *buf)
{
    int oldoffset = *offset;
    unsigned char lifetime[2] = { 0x0f, 0xa0 };
    //unsigned char mustbefresh[2] = { NDN_TLV_MustBeFresh, 0x00 };

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

    /*if (ccnl_ndntlv_prependBlob(NDN_TLV_Selectors, mustbefresh, 2,
                                offset, buf) < 0)
        return -1;*/

    if (ccnl_ndntlv_prependName(name, offset, buf))
        return -1;
    if (ccnl_ndntlv_prependTL(NDN_TLV_Interest, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependContent(struct ccnl_prefix_s *name,
                           unsigned char *payload, int paylen,
                           int *contentpos, unsigned int *final_block_id,
                           int *offset, unsigned char *buf)
{
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

    // optional (empty for now) because ndn client lib also puts it in by default
    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 0, offset, buf) < 0)
        return -1;

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
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf) < 0)
           return -1;

    if (contentpos)
        *contentpos -= *offset;

    return oldoffset - *offset;
}

#ifdef USE_FRAG

// produces a full FRAG packet. It does not write, just read the fields in *fr
struct ccnl_buf_s*
ccnl_ndntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed)
{
    unsigned char test[20];
    int offset, hdrlen;
    int datalen;
    struct ccnl_buf_s *buf;
    uint16_t tmp = 0;

    DEBUGMSG(TRACE, "ccnl_ndntlv_mkFrag seqno=%d\n", fr->sendseq);

    // pre-compute overhead, first
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (datalen > fr->mtu)
        datalen = fr->mtu;
    offset = sizeof(test);
    hdrlen = ccnl_ndntlv_prependTL(NDN_TLV_NdnlpFragment, datalen,
                                   &offset, test);
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Frag_BeginEndFields, 2,
                                    &offset, test) + 2;
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Fragment, hdrlen + datalen,
                                    &offset, test);

    // with real values:
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (datalen > (fr->mtu - hdrlen))
        datalen = fr->mtu - hdrlen;

    buf = ccnl_buf_new(NULL, datalen + hdrlen);
    if (!buf)
        return 0;
    offset = buf->datalen - datalen;
    memcpy(buf->data + offset,
           fr->bigpkt->data + fr->sendoffs, datalen);
    ccnl_ndntlv_prependTL(NDN_TLV_NdnlpFragment, datalen,
                          &offset, buf->data);

    tmp = fr->sendseq & 0x03fff;
    if (datalen >= fr->bigpkt->datalen) {            // single
        tmp |= CCNL_DTAG_FRAG_FLAG_SINGLE << 14;
    } else if (fr->sendoffs == 0)                    // start
        tmp |= CCNL_DTAG_FRAG_FLAG_FIRST << 14;
    else if((unsigned) datalen >= (fr->bigpkt->datalen - fr->sendoffs)) { // end
        tmp |= CCNL_DTAG_FRAG_FLAG_LAST << 14;
    } else
        tmp |= CCNL_DTAG_FRAG_FLAG_MID << 14;        // middle
    offset -= 2;
    *(uint16_t*) (buf->data + offset) = htons(tmp);
    tmp = ccnl_ndntlv_prependTL(NDN_TLV_Frag_BeginEndFields, 2,
                          &offset, buf->data);
    tmp = ccnl_ndntlv_prependTL(NDN_TLV_Fragment, buf->datalen - offset,
                          &offset, buf->data);

    if (offset > 0) {
        buf->datalen -= offset;
        memmove(buf->data, buf->data + offset, buf->datalen);
    }

    *consumed = datalen;
    return buf;
}
#endif // USE_FRAG

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_NDNTLV

// eof
