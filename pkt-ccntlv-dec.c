/*
 * @f pkt-ccntlv-dec.c
 * @b CCN lite - CCNx pkt decoding routines (TLV pkt format Nov 2013)
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

#ifndef PKT_CCNTLV_DEC_C
#define PKT_CCNTLV_DEC_C

#include "pkt-ccntlv.h"


int
ccnl_ccnltv_extractNetworkVarInt(unsigned char *buf, int len, int *intval) {

    int nintval = 0;

    memcpy((unsigned char *)&nintval + 4 - len, buf, len);
    *intval = ntohl(nintval);

    return 0;
}

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
                    int *chunknum, int *lastchunknum,
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

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
        return NULL;
    p->suite = CCNL_SUITE_CCNTLV;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    // We ignore the TL types of the message for now
    // content and interests are filled in both cases (and only one exists)
    // validation is ignored
    if(ccnl_ccntlv_dehead(data, datalen, &typ, &len)) {
        goto Bail;
    }

    oldpos = *data - start;
    while (ccnl_ccntlv_dehead(data, datalen, &typ, &len) == 0) {
        unsigned char *cp = *data, *cp2;
        int len2 = len;
        switch (typ) {
        case CCNX_TLV_M_Name:
            p->nameptr = start + oldpos;
            while (len2 > 0) {
                unsigned int len3;
                cp2 = cp;
                if (ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3))
                    goto Bail;

                if(chunknum && typ == CCNX_TLV_N_Chunk && ccnl_ccnltv_extractNetworkVarInt(cp, len2, chunknum) < 0) {
                    DEBUGMSG(99, "Error extracting NetworkVarInt for Chunk\n");
                    goto Bail;
                } 

                if (p->compcnt < CCNL_MAX_NAME_COMP) {
                    p->comp[p->compcnt] = cp2;
                    p->complen[p->compcnt] = cp - cp2 + len3;
                    p->compcnt++;
                }  // else out of name component memory: skip
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

        case CCNX_TLV_M_MetaData: {
                unsigned int len3;
                if(ccnl_ccntlv_dehead(&cp, &len2, &typ, &len3)) {
                    DEBUGMSG(99, "error when extracting CCNX_TLV_M_MetaData\n");
                    goto Bail;
                }
                if(lastchunknum && typ == CCNX_TLV_M_ENDChunk) {
                    if(ccnl_ccnltv_extractNetworkVarInt(cp, len2, lastchunknum) < 0) {
                        DEBUGMSG(99, "error when extracting CCNX_TLV_M_ENDChunk\n");
                        goto Bail;
                    } 
                }  
                cp += len3;
                len2 -= len3;
            }
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
//    free_2ptr_list(n, pub);
    return NULL;
}

#endif
// eof
