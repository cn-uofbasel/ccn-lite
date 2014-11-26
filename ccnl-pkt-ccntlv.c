/*
 * @f pkt-ccntlv.c
 * @b CCN lite - CCNx pkt parsing and composing (TLV pkt format Sep 22, 2014)
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
 * 2014-11-05 merged from pkt-ccntlv-enc.c pkt-ccntlv-dec.c
 */

#ifndef PKT_CCNTLV_C
#define PKT_CCNTLV_C

#include "ccnl-pkt-ccntlv.h"

/* ----------------------------------------------------------------------
Note:
  For a CCNTLV prefix, we store the name components INCLUDING
  the TL. This is different for CCNB and NDNTLV where we only keep
  a components value bytes, as there is only one "type" of name
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

// parse TL (returned in typ and vallen) and adjust buf and len
int
ccnl_ccntlv_dehead(unsigned char **buf, int *len,
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
struct ccnl_buf_s*
ccnl_ccntlv_extract(int hdrlen,
                    unsigned char **data, int *datalen,
                    struct ccnl_prefix_s **prefix,
                    unsigned char **keyid, int *keyidlen,
                    unsigned int *lastchunknum,
                    unsigned char **content, int *contlen)
{
    unsigned char *start = *data - hdrlen;
    int i;
    unsigned int len, typ, oldpos;
    struct ccnl_prefix_s *p;
    struct ccnl_buf_s *buf;

    DEBUGMSG(99, "ccnl_ccntlv_extract len=%d hdrlen=%d\n", *datalen, hdrlen);

    if (content)
        *content = NULL;
    if (keyid)
        *keyid = NULL;

    p = ccnl_prefix_new(CCNL_SUITE_CCNTLV, CCNL_MAX_NAME_COMP);
    if (!p)
        return NULL;

    // We ignore the TL types of the message for now
    // content and interests are filled in both cases (and only one exists)
    // validation is ignored
    if(ccnl_ccntlv_dehead(data, datalen, &typ, &len))
        goto Bail;

    oldpos = *data - start;
    while (ccnl_ccntlv_dehead(data, datalen, &typ, &len) == 0) {
        unsigned char *cp = *data, *cp2;
        int len2 = len;
        unsigned int len3;

        switch (typ) {
        case CCNX_TLV_M_Name:
            p->nameptr = start + oldpos;
            while (len2 > 0) {
                cp2 = cp;
                if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;

                if (typ == CCNX_TLV_N_Chunk) {
                    // We extract the chunknum to the prefix but keep it in the name component for now
                    // In the future we possibly want to remove the chunk segment from the name components 
                    // and rely on the chunknum field in the prefix.
                    p->chunknum = ccnl_malloc(sizeof(int));

                    if (ccnl_ccnltv_extractNetworkVarInt(cp,
                                                         len2, p->chunknum) < 0) {
                        DEBUGMSG(99, "Error in NetworkVarInt for chunk\n");
                        goto Bail;
                    }
                    if (p->compcnt < CCNL_MAX_NAME_COMP) {
                        p->comp[p->compcnt] = cp2;
                        p->complen[p->compcnt] = cp - cp2 + len3;
                        p->compcnt++;
                    } // else out of name component memory: skip
                } else if (typ == CCNX_TLV_N_NameSegment) {
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
                if (p->compcnt > 0 && p->complen[p->compcnt-1] == 9 &&
                   !memcmp(p->comp[p->compcnt-1], "\x00\x01\x00\x05THUNK", 9)) {
                    p->nfnflags |= CCNL_PREFIX_THUNK;
                    p->compcnt--;
                }
            }
#endif
            break;
        case CCNX_TLV_M_MetaData:
            if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3)) {
                DEBUGMSG(99, "error when extracting CCNX_TLV_M_MetaData\n");
                goto Bail;
            }
            if (lastchunknum && typ == CCNX_TLV_M_ENDChunk &&
                ccnl_ccnltv_extractNetworkVarInt(cp, len2, lastchunknum) < 0) {
                DEBUGMSG(99, "error when extracting CCNX_TLV_M_ENDChunk\n");
                goto Bail;
            }  
            cp += len3;
            len2 -= len3;
            break;
        case CCNX_TLV_M_Payload:
            if (content) {
                *content = *data;
                *contlen = len;
            }
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

    if (prefix)    *prefix = p;    else free_prefix(p);

    buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content && *content)
        *content = buf->data + (*content - start);
    for (i = 0; i < p->compcnt; i++)
            p->comp[i] = buf->data + (p->comp[i] - start);
    if (p->nameptr)
        p->nameptr = buf->data + (p->nameptr - start);

    return buf;
Bail:
    free_prefix(p);
    return NULL;
}

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

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
ccnl_ccntlv_prependNetworkVarInt(unsigned short type, unsigned int intval,
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
    struct ccnx_tlvhdr_ccnx201411_s *hp;

    if (*offset < 8 || payloadlen < 0)
        return -1;
    *offset -= 8;
    hp = (struct ccnx_tlvhdr_ccnx201411_s *)(buf + *offset);
    hp->version = ver;
    hp->packettype = packettype;
    hp->payloadlen = htons(payloadlen);
    hp->hoplimit = hoplimit;
    hp->reserved = 0;
    hp->hdrlen = hdrlen; // htons(hdrlen);

    return hdrlen + payloadlen;
}

// write given prefix and chunknum *before* buf[offs], adjust offset
// and return bytes used
int
ccnl_ccntlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf) {

    int oldoffset = *offset, cnt;

    if (name->chunknum &&
        ccnl_ccntlv_prependNetworkVarInt(CCNX_TLV_N_Chunk,
                                         *name->chunknum, offset, buf) < 0)
            return -1;

    // optional: (not used)
    // CCNX_TLV_N_MetaData

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN)
        if (ccnl_pkt_prependComponent(name->suite, "NFN", offset, buf) < 0)
            return -1;
    if (name->nfnflags & CCNL_PREFIX_THUNK)
        if (ccnl_pkt_prependComponent(name->suite, "THUNK", offset, buf) < 0)
            return -1;
#endif
    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
        if (*offset < name->complen[cnt])
            return -1;
        *offset -= name->complen[cnt];
        memcpy(buf + *offset, name->comp[cnt], name->complen[cnt]);
    }
    if (ccnl_ccntlv_prependTL(CCNX_TLV_M_Name, oldoffset - *offset,
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

    if (ccnl_ccntlv_prependName(name, offset, buf))
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
    unsigned char hoplimit = 255; // setting hoplimit to max valid value

    oldoffset = *offset;
    len = ccnl_ccntlv_prependInterest(name, offset, buf);
    if (len >= ((1 << 16)-8))
        return -1;
    if (ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V0, CCNX_TLV_TL_Interest, 
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
                           unsigned int *lastchunknum, int *offset, int *contentpos,
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
ccnl_ccntlv_prependContentWithHdr(struct ccnl_prefix_s *name,
                                  unsigned char *payload, int paylen,
                                  unsigned int *lastchunknum, int *offset,
                                  int *contentpos, unsigned char *buf)
{
    int len, oldoffset;
    unsigned char hoplimit = 255; // setting to max (conten obj has no hoplimit)

    oldoffset = *offset;

    len = ccnl_ccntlv_prependContent(name, payload, paylen, lastchunknum,
                                     offset, contentpos, buf);

    if (len >= ((1 << 16) - 4))
        return -1;

    ccnl_ccntlv_prependFixedHdr(CCNX_TLV_V0, CCNX_TLV_TL_Object,
                                len, hoplimit, offset, buf);
    return oldoffset - *offset;
}

#endif // NEEDS_PACKET_CRAFTING

#endif // PKT_CCNTLV_C

// eof
