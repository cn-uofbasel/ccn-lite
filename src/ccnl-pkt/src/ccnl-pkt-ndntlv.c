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

#ifdef USE_SUITE_NDNTLV

#ifndef CCNL_LINUXKERNEL
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-core.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#else
#include <linux/types.h>
#include "../include/ccnl-pkt-ndntlv.h"
#include "../../ccnl-core/include/ccnl-core.h"
# define UINT32_MAX		(4294967295U)
# define UINT16_MAX		(65535)
#endif




// ----------------------------------------------------------------------
// packet parsing

int8_t
ccnl_ndntlv_varlenint(uint8_t **buf, size_t *len, uint64_t *val)
{
    if (**buf < 253 && *len >= 1) {
        *val = **buf;
        *buf += 1;
        *len -= 1;
    } else if (**buf == 253 && *len >= 3) { // 2 bytes
        /* ORing bytes does not provoke alignment issues */
        *val = (((*buf)[1] << 8U) | ((*buf)[2] << 0U));
        *buf += 3;
        *len -= 3;
    } else if (**buf == 254 && *len >= 5) { // 4 bytes
        /* ORing bytes does not provoke alignment issues */
        *val = ((uint64_t)(*buf)[1] << 24U) | ((uint64_t)(*buf)[2] << 16U) |
               ((uint64_t)(*buf)[3] <<  8U) | ((uint64_t)(*buf)[4] <<  0U);
        *buf += 5;
        *len -= 5;
    } else {
        // not implemented yet (or too short)
        return -1;
    }
    return 0;
}

uint64_t
ccnl_ndntlv_nonNegInt(uint8_t *cp, size_t len)
{
    uint64_t val = 0;

    while (len-- > 0) {
        val = (val << 8U) | *cp;
        cp++;
    }
    return val;
}

int8_t
ccnl_ndntlv_dehead(uint8_t **buf, size_t *len,
                   uint64_t *typ, size_t *vallen)
{
    size_t maxlen = *len;
    uint64_t vallen_int = 0;
    if (ccnl_ndntlv_varlenint(buf, len, typ)) {
        return -1;
    }
    if (ccnl_ndntlv_varlenint(buf, len, &vallen_int)) {
        return -1;
    }
    if (vallen_int > SIZE_MAX) {
        return -1; // Return failure (-1) if length value in the tlv exceeds size_t bounds
    }
    *vallen = (size_t) vallen_int;
    if (*vallen > maxlen) {
        return -1; // Return failure (-1) if length value in the tlv is longer than the buffer
    }
    return 0;
}

// we use one extraction routine for each of interest, data and fragment pkts
struct ccnl_pkt_s*
ccnl_ndntlv_bytes2pkt(uint64_t pkttype, uint8_t *start,
                      uint8_t **data, size_t *datalen)
{
    struct ccnl_pkt_s *pkt;
    size_t oldpos, len, i;
    uint64_t typ;
    struct ccnl_prefix_s *prefix = 0;
#ifdef USE_HMAC256
    int validAlgoIsHmac256 = 0;
#endif


    DEBUGMSG(DEBUG, "ccnl_ndntlv_bytes2pkt len=%zu\n", *datalen);

    pkt = (struct ccnl_pkt_s*) ccnl_calloc(1, sizeof(struct ccnl_pkt_s));
    if (!pkt) {
        return NULL;
    }
    pkt->type = pkttype;

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
        DEBUGMSG(INFO, "  ndntlv: unknown packet type %llu\n", (unsigned long long)pkttype);
        goto Bail;
    }

    pkt->suite = CCNL_SUITE_NDNTLV;
    pkt->s.ndntlv.scope = 3;
    pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;

    /* set default lifetime, in case InterestLifetime guider is absent */
    pkt->s.ndntlv.interestlifetime = CCNL_INTEREST_TIMEOUT * 1000;    // ms

    oldpos = *data - start;
    while (ccnl_ndntlv_dehead(data, datalen, &typ, &len) == 0) {
        uint8_t *cp = *data;
        size_t len2 = len;

        switch (typ) {
        case NDN_TLV_Name:
            if (prefix) {
                DEBUGMSG(WARNING, " ndntlv: name already defined\n");
                goto Bail;
            }
            prefix = ccnl_prefix_new(CCNL_SUITE_NDNTLV, CCNL_MAX_NAME_COMP);
            if (!prefix) {
                goto Bail;
            }
            prefix->compcnt = 0;
            pkt->pfx = prefix;
            pkt->val.final_block_id = -1;

            prefix->nameptr = start + oldpos;
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i)) {
                    goto Bail;
                }
                if (typ == NDN_TLV_NameComponent &&
                            prefix->compcnt < CCNL_MAX_NAME_COMP) {
                    if(cp[0] == NDN_Marker_SegmentNumber) {
                        uint64_t chunknum;
                        prefix->chunknum = (uint32_t *) ccnl_malloc(sizeof(uint32_t));
                        // TODO: requires ccnl_ndntlv_includedNonNegInt which includes the length of the marker
                        // it is implemented for encode, the decode is not yet implemented
                        chunknum = ccnl_ndntlv_nonNegInt(cp + 1, i - 1);
                        if (chunknum > UINT32_MAX) {
                            goto Bail;
                        }
                        *prefix->chunknum = (uint32_t) chunknum;
                    }
                    prefix->comp[prefix->compcnt] = cp;
                    prefix->complen[prefix->compcnt] = i; //FIXME, what if the len value inside the TLV is wrong -> can this lead to overruns inside
                    prefix->compcnt++;
                }  // else unknown type: skip
                cp += i;
                len2 -= i;
            }
            prefix->namelen = *data - prefix->nameptr;
            DEBUGMSG(DEBUG, "  check interest type\n");
            break;
        case NDN_TLV_Selectors:
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i)) {
                    goto Bail;
                }
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
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i)) {
                    goto Bail;
                }
                if (typ == NDN_TLV_ContentType) {
                    // Not used
                    // = ccnl_ndntlv_nonNegInt(cp, i);
                    DEBUGMSG(WARNING, "'ContentType' field ignored\n");
                }
                if (typ == NDN_TLV_FreshnessPeriod) {
                    pkt->s.ndntlv.freshnessperiod = ccnl_ndntlv_nonNegInt(cp, i);
                }
                if (typ == NDN_TLV_FinalBlockId) {
                    if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i)) {
                        goto Bail;
                    }
                    if (typ == NDN_TLV_NameComponent) {
                        // TODO: again, includedNonNeg not yet implemented
                        pkt->val.final_block_id = ccnl_ndntlv_nonNegInt(cp + 1, i - 1);
                        if (pkt->val.final_block_id < 0) { // TODO: Is this check ok?
                            goto Bail;
                        }
                    }
                }
                cp += i;
                len2 -= i;
            }
            break;
        case NDN_TLV_InterestLifetime:
            pkt->s.ndntlv.interestlifetime = ccnl_ndntlv_nonNegInt(*data, len);
            break;
        case NDN_TLV_Frag_BeginEndFields:
            pkt->val.seqno = ccnl_ndntlv_nonNegInt(*data, len);
            DEBUGMSG(TRACE, "  frag: %04llux\n", (unsigned long long)pkt->val.seqno);
            if (pkt->val.seqno & 0x4000) {
                pkt->flags |= CCNL_PKT_FRAG_BEGIN;
            }
            if (pkt->val.seqno & 0x8000) {
                pkt->flags |= CCNL_PKT_FRAG_END;
            }
            pkt->val.seqno &= 0x3fff;
            break;
#ifdef USE_HMAC256
        case NDN_TLV_SignatureInfo:
            while (len2 > 0) {
                if (ccnl_ndntlv_dehead(&cp, &len2, &typ, &i)) {
                    goto Bail;
                }
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
    if (*datalen > 0) {
        goto Bail;
    }

    pkt->pfx = prefix;
    pkt->buf = ccnl_buf_new(start, *data - start);
    if (!pkt->buf) {
        goto Bail;
    }
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (pkt->content) {
        pkt->content = pkt->buf->data + (pkt->content - start);
    }
    if (prefix) {
        for (i = 0; i < prefix->compcnt; i++) {
            prefix->comp[i] = pkt->buf->data + (prefix->comp[i] - start);
        }
        if (prefix->nameptr) {
            prefix->nameptr = pkt->buf->data + (prefix->nameptr - start);
        }
    }

    return pkt;
Bail:
    ccnl_pkt_free(pkt);
    return NULL;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int8_t
ccnl_ndntlv_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
#ifndef CCNL_LINUXKERNEL
    assert(p);
    assert(p->suite == CCNL_SUITE_NDNTLV);
#endif

    if (ccnl_i_prefixof_c(p->pfx, p->s.ndntlv.minsuffix, p->s.ndntlv.maxsuffix, c) < 0) {
        return -1;
    }

    if (p->s.ndntlv.mbf && ((c->flags & CCNL_CONTENT_FLAGS_STALE) != 0)) {
        DEBUGMSG(DEBUG, "ignore stale content\n");
        return -1;
    }

    DEBUGMSG(DEBUG, "  matching content for interest, content %p\n",
                     (void *) c);
    return 0;
}

#endif

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

int8_t
ccnl_ndntlv_prependTLval(uint64_t val, size_t *offset, uint8_t *buf)
{
    uint8_t len, i, t;

    if (val < 253U) {
        len = 0U, t = (uint8_t) val;
    } else if (val <= 0xffff) {
        len = 2U, t = 253U;
    } else if (val <= 0xffffffffL) {
        len = 4U, t = 254U;
    } else {
        len = 8U, t = 255U;
    }
    if (*offset < (unsigned) (len+1)) {
        return -1;
    }

    for (i = 0; i < len; i++) {
        buf[--(*offset)] = (uint8_t) (val & 0xffU);
        val = val >> 8U;
    }
    buf[--(*offset)] = t;
    // return len + 1;
    return 0;
}

int8_t
ccnl_ndntlv_prependTL(uint64_t type, uint64_t len,
                      size_t *offset, uint8_t *buf)
{
//    size_t oldoffset = *offset;
    if (ccnl_ndntlv_prependTLval(len, offset, buf) < 0) {
        return -1;
    }
    if (ccnl_ndntlv_prependTLval(type, offset, buf) < 0) {
        return -1;
    }
    // return (oldoffset - *offset);
    return 0;
}

int8_t
ccnl_ndntlv_prependNonNegIntVal(uint64_t val,
                                size_t *offset, uint8_t *buf) {
    int len = 0;
    uint8_t i;
    // Number of padding bytes needed for a value of a certain length
    static uint8_t fill[] = {1, 0, 0, 1, 0, 3, 2, 1, 0};

    while (val) {
        if ((*offset)-- < 1) {
            return -1;
        }
        buf[*offset] = (uint8_t) (val & 0xffU);
        len++;
        val = val >> 8U;
    }
    for (i = 0; i < fill[len]; ++i) {
        if ((*offset)-- < 1) {
            return -1;
        }
        buf[*offset] = 0;
        len++;
    }
    // return len;
    return 0;
}

int8_t
ccnl_ndntlv_prependNonNegInt(uint64_t type,
                             uint64_t val,
                             size_t *offset, uint8_t *buf)
{
    size_t oldoffset = *offset;
    if (ccnl_ndntlv_prependNonNegIntVal(val, offset, buf) < 0) {
        return -1;
    }
    if (ccnl_ndntlv_prependTL(type, oldoffset - *offset, offset, buf) < 0) {
        return -1;
    }
    // return oldoffset - *offset;
    return 0;
}


int8_t
ccnl_ndntlv_prependIncludedNonNegInt(uint64_t type, uint64_t val,
                                     uint8_t marker,
                                     size_t *offset, uint8_t *buf)
{
    size_t oldoffset = *offset;
    if (ccnl_ndntlv_prependNonNegIntVal(val, offset, buf) < 0) {
        return -1;
    }

    if((*offset)-- < 1) {
        return -1;
    }
    buf[*offset] = marker;

    if (ccnl_ndntlv_prependTL(type, oldoffset - *offset, offset, buf) < 0) {
        return -1;
    }
    // return oldoffset - *offset;
    return 0;
}


int8_t
ccnl_ndntlv_prependBlob(uint64_t type, uint8_t *blob, size_t len,
                        size_t *offset, uint8_t *buf)
{

    if (*offset < len) {
        return -1;
    }
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    if (ccnl_ndntlv_prependTL(type, len, offset, buf) < 0) {
        return -1;
    }
    return 0;
}

int8_t
ccnl_ndntlv_prependName(struct ccnl_prefix_s *name,
                        size_t *offset, uint8_t *buf)
{
    size_t oldoffset = *offset;
    uint64_t cnt;

    if (name->chunknum) {
        if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                 *name->chunknum,
                                                 NDN_Marker_SegmentNumber,
                                                 offset, buf) < 0) {
            return -1;
        }
    }

    for (cnt = name->compcnt; cnt > 0; cnt--) {
        if (ccnl_ndntlv_prependBlob(NDN_TLV_NameComponent, name->comp[cnt-1],
                                    name->complen[cnt-1], offset, buf) < 0) {
            return -1;
        }
    }
    if (ccnl_ndntlv_prependTL(NDN_TLV_Name, oldoffset - *offset,
                              offset, buf) < 0) {
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------

int8_t
ccnl_ndntlv_prependInterest(struct ccnl_prefix_s *name, int scope, struct ccnl_ndntlv_interest_opts_s *opts,
                            size_t *offset, uint8_t *buf, size_t *reslen)
{
    size_t oldoffset = *offset;

    if (scope >= 0) {
        if (scope > 2) {
            return -1;
        }
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_Scope, (uint64_t) scope, offset, buf) < 0) {
            return -1;
        }
    }

    /* only include InterestLifetime TLV Guider, if life time > 0 milli seconds */
    if (opts->interestlifetime) {
        if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_InterestLifetime,
                                         opts->interestlifetime, offset, buf) < 0) {
            return -1;
        }
    }

    if (ccnl_ndntlv_prependBlob(NDN_TLV_Nonce, (uint8_t *) &opts->nonce, 4, offset, buf) < 0) {
        return -1;
    }

    /* MustBeFresh is the only supported Selector for now */
    if (opts->mustbefresh) {
        size_t sel_offset = *offset;
        if (ccnl_ndntlv_prependTL(NDN_TLV_MustBeFresh, 0U, offset, buf) < 0) {
            return -1;
        }

        if (ccnl_ndntlv_prependTL(NDN_TLV_Selectors, sel_offset - *offset, offset, buf) < 0) {
            return -1;
        }
    }

    if (ccnl_ndntlv_prependName(name, offset, buf)) {
        return -1;
    }

    if (ccnl_ndntlv_prependTL(NDN_TLV_Interest, oldoffset - *offset,
                              offset, buf) < 0) {
        return -1;
    }

    *reslen = oldoffset - *offset;
    return 0;
}

int8_t
ccnl_ndntlv_prependContent(struct ccnl_prefix_s *name,
                           uint8_t *payload, size_t paylen,
                           size_t *contentpos, struct ccnl_ndntlv_data_opts_s *opts,
                           size_t *offset, uint8_t *buf, size_t *reslen)
{
    size_t oldoffset = *offset, oldoffset2;
    uint8_t signatureType = NDN_VAL_SIGTYPE_DIGESTSHA256;

    if (contentpos) {
        *contentpos = *offset - paylen;
    }

    // fill in backwards

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureValue, 0, offset, buf) < 0) {
        return -1;
    }

    // to find length of SignatureInfo
    oldoffset2 = *offset;

    // KeyLocator is not required for DIGESTSHA256
    if (signatureType != NDN_VAL_SIGTYPE_DIGESTSHA256) {
        if (ccnl_ndntlv_prependTL(NDN_TLV_KeyLocator, 0, offset, buf) < 0) {
            return -1;
        }
    }

    // use NDN_SigTypeVal_SignatureSha256WithRsa because this is default in ndn client libs
    if (ccnl_ndntlv_prependBlob(NDN_TLV_SignatureType, &signatureType, 1,
                offset, buf) < 0) {
        return 1;
    }

    // Groups KeyLocator and Signature Type with stored len
    if (ccnl_ndntlv_prependTL(NDN_TLV_SignatureInfo, oldoffset2 - *offset, offset, buf) < 0) {
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependBlob(NDN_TLV_Content, payload, paylen,
                                offset, buf) < 0) {
        return -1;
    }

    // to find length of optional (?) MetaInfo fields
    oldoffset2 = *offset;
    if (opts) {
        if (opts->finalblockid != UINT32_MAX) {
            if (ccnl_ndntlv_prependIncludedNonNegInt(NDN_TLV_NameComponent,
                                                     opts->finalblockid,
                                                     NDN_Marker_SegmentNumber,
                                                     offset, buf) < 0) {
                return -1;
            }

            // optional
            if (ccnl_ndntlv_prependTL(NDN_TLV_FinalBlockId, oldoffset2 - *offset,
                                      offset, buf) < 0) {
                return -1;
            }
        }

        if (opts->freshnessperiod) {
            if (ccnl_ndntlv_prependNonNegInt(NDN_TLV_FreshnessPeriod,
                                             opts->freshnessperiod, offset, buf) < 0) {
                return -1;
            }
        }
    }

    // mandatory (empty for now)
    if (ccnl_ndntlv_prependTL(NDN_TLV_MetaInfo, oldoffset2 - *offset,
                              offset, buf) < 0) {
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependName(name, offset, buf)) {
        return -1;
    }

    // mandatory
    if (ccnl_ndntlv_prependTL(NDN_TLV_Data, oldoffset - *offset,
                              offset, buf) < 0) {
           return -1;
    }

    if (contentpos) {
        *contentpos -= *offset;
    }

    *reslen = oldoffset - *offset;
    return 0;
}

#ifdef USE_FRAG

// produces a full FRAG packet. It does not write, just read the fields in *fr
struct ccnl_buf_s*
ccnl_ndntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed)
{
    unsigned char test[20];
    size_t offset, hdrlen;
    long datalen;
    struct ccnl_buf_s *buf;
    uint16_t tmp = 0;

    DEBUGMSG(TRACE, "ccnl_ndntlv_mkFrag seqno=%d\n", fr->sendseq);

    // pre-compute overhead, first
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (datalen > fr->mtu) {
        datalen = fr->mtu;
    }
    offset = sizeof(test);
    hdrlen = ccnl_ndntlv_prependTL(NDN_TLV_NdnlpFragment, datalen,
                                   &offset, test);
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Frag_BeginEndFields, 2,
                                    &offset, test) + 2;
    hdrlen += ccnl_ndntlv_prependTL(NDN_TLV_Fragment, hdrlen + datalen,
                                    &offset, test);

    // with real values:
    datalen = fr->bigpkt->datalen - fr->sendoffs;
    if (datalen > (fr->mtu - hdrlen)) {
        datalen = fr->mtu - hdrlen;
    }

    buf = ccnl_buf_new(NULL, datalen + hdrlen);
    if (!buf) {
        return 0;
    }
    offset = buf->datalen - datalen;
    memcpy(buf->data + offset,
           fr->bigpkt->data + fr->sendoffs, datalen);
    ccnl_ndntlv_prependTL(NDN_TLV_NdnlpFragment, datalen,
                          &offset, buf->data);

    tmp = fr->sendseq & 0x03fff;
    if (datalen >= fr->bigpkt->datalen) {            // single
        tmp |= CCNL_DTAG_FRAG_FLAG_SINGLE << 14;
    } else if (fr->sendoffs == 0) {                  // start
        tmp |= CCNL_DTAG_FRAG_FLAG_FIRST << 14;
    } else if((unsigned) datalen >= (fr->bigpkt->datalen - fr->sendoffs)) { // end
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
