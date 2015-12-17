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
    if (!buf || !*buf || !len)
        return 0;
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
                   unsigned int *typ, unsigned int *vallen)
{
  if (ccnl_ndntlv_varlenint(buf, len, (int*) typ))
        return -1;
  if (ccnl_ndntlv_varlenint(buf, len, (int*) vallen))
        return -1;
    return 0;
}

// turn the sequence of components into a ccnl_prefix_s data structure
struct ccnl_prefix_s*
ccnl_ndntlv_bytes2prefix(unsigned char **md, unsigned char **data, int *datalen)
{
    struct ccnl_prefix_s *p;

    DEBUGMSG(DEBUG, "ccnl_ndntlv_bytes2prefix len=%d\n", *datalen);

    p = ccnl_prefix_new(CCNL_SUITE_NDNTLV, CCNL_MAX_NAME_COMP);
    if (!p)
        return NULL;

    p->compcnt = 0;
    while (*datalen > 0) {
        unsigned int typ, len3;

        if (ccnl_ndntlv_dehead(data, datalen, &typ, &len3))
            goto Bail;
        if (typ == NDN_TLV_NameComponent && p->compcnt < CCNL_MAX_NAME_COMP) {
            if (data[0] == NDN_Marker_SegmentNumber) {
                p->chunknum = (int*) ccnl_malloc(sizeof(int));
                        // TODO: requires ccnl_ndntlv_includedNonNegInt which includes the length of the marker
                        // it is implemented for encode, the decode is not yet implemented
                *p->chunknum = ccnl_ndntlv_nonNegInt(*data + 1, len3 - 1);
            }
            p->comp[p->compcnt] = *data;
            p->complen[p->compcnt] = len3;
            p->compcnt++;
        } else if (md && typ == NDN_TLV_NameImplicitDigest &&
                                                      len3 == SHA256_DIGEST_LENGTH) {
            *md = *data;
        } // else unknown type: skip
        *data += len3;
        *datalen -= len3;
    }

#ifdef USE_NFN
    if (p->compcnt > 0 && p->complen[p->compcnt-1] == 3 &&
        !memcmp(p->comp[p->compcnt-1], "NFN", 3)) {
        p->nfnflags |= CCNL_PREFIX_NFN;
        p->compcnt--;
        if (p->compcnt > 0 && p->complen[p->compcnt-1] == 5 &&
            !memcmp(p->comp[p->compcnt-1], "THUNK", 5)) {
            p->nfnflags |= CCNL_PREFIX_THUNK;
            p->compcnt--;
        }
    }
#endif
    return p;
Bail:
    DEBUGMSG(DEBUG, "  ccnl_ndntlv_bytes2prefix bailed\n");
    free_prefix(p);
    return NULL;
}

// we use one extraction routine for each of interest, data and fragment pkts
struct ccnl_pkt_s*
ccnl_ndntlv_bytes2pkt(unsigned char *start,
                      unsigned char **data, int *datalen)
{
    unsigned char *msgstart, *digest = NULL;
    struct ccnl_pkt_s *pkt;
    int oldpos, i;
    unsigned int typ, len;
    struct ccnl_prefix_s *p = 0;
#ifdef USE_HMAC256
    int validAlgoIsHmac256 = 0;
#endif

    DEBUGMSG(DEBUG, "ccnl_ndntlv_bytes2pkt len=%d\n", *datalen);

    if (ccnl_ndntlv_dehead(data, datalen, &typ, &len) || (int) len > *datalen) {
        DEBUGMSG_CFWD(TRACE, "  invalid packet format\n");
        return NULL;
    }

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;
    pkt->packetType = typ;

    switch(pkt->packetType) {
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
    case NDN_TLV_NDNLP:
        pkt->flags |= CCNL_PKT_NDNLP;
        DEBUGMSG(INFO, "  ndnlp packet\n");
        goto Bail;
    default:
        DEBUGMSG(INFO, "  ndntlv: unknown packet type %d\n", pkt->packetType);
        goto Bail;
    }

#ifdef USE_HMAC256
    pkt->hmacStart = *data;
#endif
    msgstart = start;

    pkt->suite = CCNL_SUITE_NDNTLV;
    pkt->s.ndntlv.scope = 3;
    pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;

    oldpos = *data - start;
    while (ccnl_ndntlv_dehead(data, datalen, &typ, &len) == 0) {
        unsigned char *cp = *data;
        int len2 = len;

        if (len > (unsigned) *datalen)
            goto Bail;
        switch (typ) {
        case NDN_TLV_Name:
            if (p) {
                DEBUGMSG(WARNING, " ndntlv: name already defined\n");
                goto Bail;
            }
            p = ccnl_ndntlv_bytes2prefix(&digest, &cp, &len2);
            if (!p)
                goto Bail;
            if (digest) {
                pkt->s.ndntlv.dataHashRestr = ccnl_malloc(SHA256_DIGEST_LENGTH);
                memcpy(pkt->s.ndntlv.dataHashRestr, digest, SHA256_DIGEST_LENGTH);
            }
            digest = NULL;
            p->nameptr = start + oldpos;
            p->namelen = cp - p->nameptr;
            pkt->pfx = p;
            pkt->val.final_block_id = -1;
            break;
        case NDN_TLV_Selectors:
            while (len2 > 0) {
                unsigned int len3;
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;
                switch(typ) {
                case NDN_TLV_MinSuffixComponents:
                    pkt->s.ndntlv.minsuffix = ccnl_ndntlv_nonNegInt(cp, len3);
                    break;
                case NDN_TLV_MaxSuffixComponents:
                    pkt->s.ndntlv.maxsuffix = ccnl_ndntlv_nonNegInt(cp, len3);
                    break;
                case NDN_TLV_MustBeFresh:
                    pkt->s.ndntlv.mbf = 1;
                    break;
                case NDN_TLV_Exclude:
                    DEBUGMSG(DEBUG, "'Exclude' field ignored\n");
                    break;
                default:
                    break;
                }
                cp += len3;
                len2 -= len3;
            }
            break;
        case NDN_TLV_Nonce:
            pkt->s.ndntlv.nonce = ccnl_buf_new(*data, len);
            break;
        case NDN_TLV_Scope:
            pkt->s.ndntlv.scope = ccnl_ndntlv_nonNegInt(*data, len);
            break;
        case NDN_TLV_Content:
        case NDN_TLV_Fragment: // payload
            pkt->content = *data;
            pkt->contlen = len;
            break;
        case NDN_TLV_MetaInfo:
            while (len2 > 0) {
            unsigned int len3;
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;
                if (typ == NDN_TLV_ContentType) {
                    pkt->contentType = ccnl_ndntlv_nonNegInt(cp, len3);
                    DEBUGMSG(TRACE, "'ContentType' %d\n",
                             pkt->contentType);
                }
                if (typ == NDN_TLV_FreshnessPeriod) {
                    struct timeval tv;
                    ccnl_get_timeval(&tv);
                    pkt->expiresAt = tv.tv_sec +
                        (tv.tv_usec / 1000 + ccnl_ndntlv_nonNegInt(cp, len3)) / 1000;
                }
                if (typ == NDN_TLV_FinalBlockId) {
                    if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &len3))
                        goto Bail;
                    if (typ == NDN_TLV_NameComponent) {
                        // TODO: again, includedNonNeg not yet implemented
                        pkt->val.final_block_id = ccnl_ndntlv_nonNegInt(cp + 1, len3 - 1);
                    }
                }
                cp += len3;
                len2 -= len3;
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
                unsigned int len3;
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;
                if (typ == NDN_TLV_SignatureType && len3 == 1 &&
                                          *cp == NDN_VAL_SIGTYPE_HMAC256) {
                    validAlgoIsHmac256 = 1;
                    break;
                }
                cp += len3;
                len2 -= len3;
            }
            if (pkt->hmacStart)
                pkt->hmacLen = (*data + len) - pkt->hmacStart;
            break;
        case NDN_TLV_SignatureValue:
            if (pkt->hmacStart && validAlgoIsHmac256 && len == 32)
                pkt->hmacSignature = *data;
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

#ifdef USE_NAMELESS
    {
        SHA256_CTX_t ctx;

        ccnl_SHA256_Init(&ctx);
        ccnl_SHA256_Update(&ctx, msgstart, *data - msgstart);
        ccnl_SHA256_Final(pkt->md, &ctx);
    }
#else
    (void) msgstart;
#endif

//    pkt->pfx = p;
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

#ifdef USE_NAMELESS
    if (p->s.ndntlv.dataHashRestr) {
        /*
        unsigned char *cp = p->s.ndntlv.dataHashRestr;
        DEBUGMSG_PCNX(TRACE, "ccnl_ndntlv_cMatch DataHashRestr\n");
        DEBUGMSG_PCNX(TRACE, "  want %02x%02x%02x..., have %02x%02x%02x...\n",
                      cp[0], cp[1], cp[2], c->pkt->md[0], c->pkt->md[1], c->pkt->md[2]);
        */
        return memcmp(p->s.ndntlv.dataHashRestr, c->pkt->md, 32);
    }
#endif

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
ccnl_ndntlv_prependTL(unsigned short type, unsigned short len,
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
ccnl_ndntlv_prependNameComponents(struct ccnl_prefix_s *name,
                                  int *offset, unsigned char *buf)
{
    int oldoffset = *offset, cnt;

    if (!name)
        return 0;

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
    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf)
{
    int len, len2;

    if (!name)
        return 0;

    len = ccnl_ndntlv_prependNameComponents(name, offset, buf);
    if (len < 0)
        return -1;

    len2 = ccnl_ndntlv_prependTL(NDN_TLV_Name, len, offset, buf);
    if (len2 < 0)
        return -1;

    return len + len2;
}

// ----------------------------------------------------------------------

int
ccnl_ndntlv_prependInterest(struct ccnl_prefix_s *name, unsigned char *implicitDigest,
                            int scope, int *nonce, int *offset, unsigned char *buf)
{
    int oldoffset = *offset, nameoffset;
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

    nameoffset = *offset;
    if (implicitDigest) {
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_MaxSuffixComponents,
                                                                 0, offset, buf) < 0)
            return -1;
        if (ccnl_ndntlv_prependTL(NDN_TLV_Selectors, nameoffset - *offset,
                                                                    offset, buf) < 0)
            return -1;
        nameoffset = *offset;
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameImplicitDigest, implicitDigest,
                                              SHA256_DIGEST_LENGTH, offset, buf) < 0)
            return -1;
    }
    if (ccnl_ndntlv_prependNameComponents(name, offset, buf) < 0)
        return -1;
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, nameoffset - *offset, offset, buf) < 0)
        return -1;

    if (ccnl_ndntlv_prependTL(NDN_TLV_Interest, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependContentWithMeta(struct ccnl_prefix_s *name,
                                   unsigned char *payload, int paylen,
                                   int contType, int freshness,
                                   unsigned int *final_block_id,
                                   int *contentpos,
                                   int *offset, unsigned char *buf)
{
    int oldoffset = *offset, oldoffset2, shaoffset;
    unsigned char signatureType = NDN_VAL_SIGTYPE_DIGESTSHA256;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards

    // mandatory (empty for now)
    if (*offset < SHA256_DIGEST_LENGTH)
      return -1;
    *offset -= SHA256_DIGEST_LENGTH;
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, SHA256_DIGEST_LENGTH,
                              offset, buf) < 0)
        return -1;

    // to find length of SignatureInfo
    shaoffset = oldoffset2 = *offset;

    // optional (empty for now) because ndn client lib also puts it in by default
    //    if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 0, offset, buf) < 0)
    //    return -1;

    // use NDN_SigTypeVal_DigestSha256 because this is default in ndn client libs
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, &signatureType, 1,
                offset, buf) < 0)
        return 1;

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, oldoffset2 - *offset, offset, buf) < 0)
        return -1;

    // mandatory Content
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf) < 0)
        return -1;

    oldoffset2 = *offset; // keep mem ref where MetaInfo ends
    if (final_block_id) { // optional
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *final_block_id,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf) < 0)
            return -1;
        if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset,
                                  offset, buf) < 0)
            return -1;
    }
    if (freshness >= 0) { // optional FreshnessPeriod
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_FreshnessPeriod,
                                         freshness, offset, buf) < 0)
            return -1;
    }
    if (contType >= 0) { // optional ContentType
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_ContentType,
                                         contType, offset, buf) < 0)
            return -1;
    }
    // mandatory MetaInfo
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset,
                              offset, buf) < 0)
        return -1;

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf) < 0)
        return -1;

    // do the SHA256 here - the "Data" TL header is NOT included
    ccnl_SHA256(buf + *offset, shaoffset - *offset,
                buf + oldoffset - SHA256_DIGEST_LENGTH);

    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf) < 0)
           return -1;

    if (contentpos)
        *contentpos -= *offset;

    return oldoffset - *offset;
}

int
ccnl_ndntlv_prependContent(struct ccnl_prefix_s *name, unsigned char *payload,
                           int paylen, unsigned int *final_block_id,
                           int *contentpos, int *offset, unsigned char *buf)
{
    return ccnl_ndntlv_prependContentWithMeta(name, payload, paylen,
                                              -1, -1, final_block_id,
                                              contentpos, offset, buf);
}

#ifdef USE_FRAG

// produces a full FRAG packet. It does not write, just read the fields in *fr
struct ccnl_buf_s*
ccnl_ndntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed)
{
    unsigned char test[20];
    int offset, hdrlen;
    unsigned int datalen;
    struct ccnl_buf_s *buf;
    uint16_t tmp = 0;

    DEBUGMSG(TRACE, "ccnl_ndntlv_mkFrag seqno=%d\n", fr->sendseq);

    // pre-compute overhead, first
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (fr->mtu < 0) {
        DEBUGMSG(WARNING, "MTU value of fragment is negative: %d, setting fragment datalen to 0.\n", fr->mtu);
        datalen = 0;
    } else if (datalen > (unsigned int) fr->mtu)
        datalen = fr->mtu;

    offset = sizeof(test);
    /*
    hdrlen = ccnl_ndntlv_prependTL(NDN_TLV_NdnlpFragment, datalen,
                                   &offset, test);
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Frag_BeginEndFields, 2,
                                    &offset, test) + 2;
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Fragment, hdrlen + datalen,
                                    &offset, test);
    */
    hdrlen = ccnl_ndntlv_prependTL(NDN_TLV_Fragment, datalen,
                                   &offset, test);
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Frag_BeginEndFields, 2,
                                    &offset, test) + 2;
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_NDNLP, hdrlen + datalen,
                                    &offset, test);

    // with real values:
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (fr->mtu - hdrlen < 0) {
        DEBUGMSG(WARNING, "(MTU value - hdrlen) is negative: %d - %d = %d, setting fragment datalen to 0.\n",
                 fr->mtu, hdrlen, fr->mtu-hdrlen);
        datalen = 0;
    } else if (datalen > (unsigned int) (fr->mtu - hdrlen))
        datalen = fr->mtu - hdrlen;

    buf = ccnl_buf_new(NULL, datalen + hdrlen);
    if (!buf)
        return 0;
    offset = buf->datalen - datalen;
    memcpy(buf->data + offset,
           fr->bigpkt->data + fr->sendoffs, datalen);
    ccnl_ndntlv_prependTL(NDN_TLV_Fragment, datalen,
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
    tmp = ccnl_ndntlv_prependTL(NDN_TLV_NDNLP, buf->datalen - offset,
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
