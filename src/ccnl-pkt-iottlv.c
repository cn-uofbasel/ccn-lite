/*
 * @f ccnl-pkt-iottlv.c
 * @b CCN lite - dense IOT packet format parsing and composing
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
 * 2014-11-05 created
 */

#ifndef PKT_IOTTLV_C
#define PKT_IOTTLV_C

#ifdef USE_SUITE_IOTTLV

#include "ccnl-pkt-iottlv.h"

// ----------------------------------------------------------------------
// packet decomposition

static int
ccnl_iottlv_varlenint(unsigned char **buf, int *len, int *val)
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

int
ccnl_iottlv_peekType(unsigned char *buf, int len)
{
    int typ;

    if (len < 1)
        return -1;
    if (*buf)
        return *buf >> 6;
    if (ccnl_iottlv_varlenint(&buf, &len, &typ) < 0)
        return -1;
    return typ;
}

// parse TL (returned in typ and vallen) and adjust buf and len
int
ccnl_iottlv_dehead(unsigned char **buf, int *len,
                   int *typ, int *vallen)
{
    if (*len < 1)
        return -1;
    if (**buf) {
        *typ = **buf >> 6;
        *vallen = **buf & 0x3f;
        *buf += 1;
        *len -= 1;
        return 0;
    }
    *buf += 1;
    *len -= 1;
    if (ccnl_iottlv_varlenint(buf, len, (int*) typ) < 0)
        return -1;
    if (ccnl_iottlv_varlenint(buf, len, (int*) vallen) < 0)
        return -1;
    return 0;
}

struct ccnl_prefix_s *
ccnl_iottlv_parseHierarchicalName(unsigned char *data, int datalen)
{
    int typ, len = datalen, len2;
    struct ccnl_prefix_s *p;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p)
        return NULL;
    p->suite = CCNL_SUITE_IOTTLV;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen)
        return NULL;

    p->nameptr = data;
    p->namelen = len;
    while (len > 0) {
        if (ccnl_iottlv_dehead(&data, &len, &typ, &len2))
            goto Bail;
        if (typ == IOT_TLV_PN_Component &&
                               p->compcnt < CCNL_MAX_NAME_COMP) {
            p->comp[p->compcnt] = data;
            p->complen[p->compcnt] = len2;
            p->compcnt++;
        }
        data += len2;
        len -= len2;
    }
    datalen -= p->namelen;
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
    free_prefix(p);
    return NULL;
}

// we use one extraction routine for both request and reply pkts
struct ccnl_buf_s*
ccnl_iottlv_extract(unsigned char *start, unsigned char **data, int *datalen,
                    struct ccnl_prefix_s **name, int *ttl,
                    unsigned char **content, int *contlen)
{
    unsigned char *cp, tmp[10];
    int len, typ, len2, cplen;
    struct ccnl_prefix_s *n = NULL;
    struct ccnl_buf_s *buf;

    DEBUGMSG(DEBUG, "ccnl_iottlv_extract len=%d\n", *datalen);

    if (content)
        *content = NULL;

    while (*datalen) {
        if (ccnl_iottlv_dehead(data, datalen, &typ, &len))
            goto Bail;
        switch (typ) {
        case IOT_TLV_R_OptHeader:
        {
            cp = *data;
            cplen = len;
            while (cplen > 0 && !ccnl_iottlv_dehead(&cp, &cplen, &typ, &len2)) {
                if (typ == IOT_TLV_H_HopLim && len2 == 1 && ttl)
                    *ttl = *cp;
                cp += len2;
                cplen -= len2;
            }
            break;
        }
        case IOT_TLV_R_Name:
            cp = *data;
            cplen = len;
            while (cplen > 0 && !ccnl_iottlv_dehead(&cp, &cplen, &typ, &len2)) {
                if (typ == IOT_TLV_N_PathName) {
                    if (n)
                        free_prefix(n);
                    n = ccnl_iottlv_parseHierarchicalName(cp, len2);
                    if (!n)
                        goto Bail;
                }
                cp += len2;
                cplen -= len2;
            }
            break;
        case IOT_TLV_R_Payload:
            cp = *data;
            cplen = len;
            if (!ccnl_iottlv_dehead(&cp, &cplen, &typ, &len2)) {
                if (content && typ == IOT_TLV_PL_Data) {
                    *content = cp;
                    *contlen = len2;
		}
	    }
            break;
        default:
            break;
        }
        *data += len;
        *datalen -= len;
    }
    if (*datalen > 0)
        goto Bail;

// we receive a nacked packet (without switch code), we need to add
// it when creating the packet buffer
    len = sizeof(tmp);
    len2 = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &len, tmp);
    if (len2 < 0) {
        DEBUGMSG(ERROR, "prending code should not return -1\n");
        len2 = 0;
    }
    start -= len2;
    buf = ccnl_buf_new(start, *data - start);
    if (buf && len2)
        memcpy(buf->data, tmp + len, len2);

    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (content && *content)
        *content = buf->data + (*content - start);
    if (name) {
        if (n) {
            int i;
            for (i = 0; i < n->compcnt; i++)
                n->comp[i] = buf->data + (n->comp[i] - start);
            if (n->nameptr)
                n->nameptr = buf->data + (n->nameptr - start);
        }
        *name = n;
    } else
        free_prefix(n);

    return buf;
Bail:
    free_prefix(n);
    return NULL;
}

// ----------------------------------------------------------------------
// packet composition

#ifdef NEEDS_PACKET_CRAFTING

int
ccnl_iottlv_prependTLval(unsigned long val, int *offset, unsigned char *buf)
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

// write given T and L *before* position buf+offset, adj offset and return len
int
ccnl_iottlv_prependTL(unsigned int type, unsigned short len,
                      int *offset, unsigned char *buf)
{
    int oldoffset = *offset;

    if (*offset < 1)
        return -1;
    if (type < 4 && len < 64) {
        unsigned char b = (type << 6) | len;
        if (b != 0 && b != 0x80) { // 0x80 is the "switch encoding" magic byte
            buf[--(*offset)] = b;
            return 1;
        }
    }
    if (ccnl_iottlv_prependTLval(len, offset, buf) < 0)
        return -1;
    if (ccnl_iottlv_prependTLval(type, offset, buf) < 0)
        return -1;
    buf[--(*offset)] = 0;
    return oldoffset - *offset;
}

// write len bytes *before* position buf[offset], adjust offset
int
ccnl_iottlv_prependBlob(unsigned short type, unsigned char *blob,
                        unsigned short len, int *offset, unsigned char *buf)
{
    int rc;
    if (*offset < (len + 1))
        return -1;
    memcpy(buf + *offset - len, blob, len);
    *offset -= len;
    rc = ccnl_iottlv_prependTL(type, len, offset, buf);
    if (rc < 0)
        return -1;
    return len + rc;
}

// write given prefix and chunknum *before* buf[offs], adjust offset
// and return bytes used
int
ccnl_iottlv_prependName(struct ccnl_prefix_s *name,
                        int *offset, unsigned char *buf) {

    int oldoffset = *offset, cnt;

    if (name->chunknum) {
        DEBUGMSG(ERROR, "writing IOT2014 name: chunk number not supported\n");
    }

#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN) {
        if (ccnl_iottlv_prependBlob(IOT_TLV_PN_Component,
                                (unsigned char*) "NFN", 3, offset, buf) < 0)
            return -1;
        if (name->nfnflags & CCNL_PREFIX_THUNK)
            if (ccnl_iottlv_prependBlob(IOT_TLV_PN_Component,
                                (unsigned char*) "THUNK", 5, offset, buf) < 0)
                return -1;
    }
#endif

    for (cnt = name->compcnt - 1; cnt >= 0; cnt--) {
        if (ccnl_iottlv_prependBlob(IOT_TLV_PN_Component, name->comp[cnt],
                                    name->complen[cnt], offset, buf) < 0)
            return -1;
    }
    if (ccnl_iottlv_prependTL(IOT_TLV_N_PathName, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;
    if (ccnl_iottlv_prependTL(IOT_TLV_R_Name, oldoffset - *offset,
                              offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Request pkt+hdr *before* buf[offs], adjust offs and return bytes used
int
ccnl_iottlv_prependRequest(struct ccnl_prefix_s *name, int *ttl,
                           int *offset, unsigned char *buf)
{
    int oldoffset;

    oldoffset = *offset;
    if (ccnl_iottlv_prependName(name, offset, buf) < 0)
        return -1;
    if (ttl) {
        int oldoffset2 = *offset;

        // insert any optional header fiels here

        buf[--(*offset)] = *ttl;
        if (ccnl_iottlv_prependTL(IOT_TLV_H_HopLim, 1, offset, buf) < 0)
            return -1;
        if (ccnl_iottlv_prependTL(IOT_TLV_R_OptHeader,
                              oldoffset2 - *offset, offset, buf) < 0)
            return -1;
    }
    if (ccnl_iottlv_prependTL(IOT_TLV_Request,
                              oldoffset - *offset, offset, buf) < 0)
        return -1;

    return oldoffset - *offset;
}

// write Reply pktr *before* buf[offs], adjust offs and return bytes used
int
ccnl_iottlv_prependReply(struct ccnl_prefix_s *name, 
                         unsigned char *payload, int paylen, 
                         int *offset, int *contentpos,
                         unsigned int *final_block_id,
                         unsigned char *buf)
{
    int oldoffset = *offset;

    if (contentpos)
        *contentpos = *offset - paylen;

    // fill in backwards
    if (ccnl_iottlv_prependBlob(IOT_TLV_PL_Data, payload, paylen,
                                                           offset, buf) < 0)
        return -1;
    if (ccnl_iottlv_prependTL(IOT_TLV_R_Payload,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;
    if (ccnl_iottlv_prependName(name, offset, buf) < 0)
        return -1;
    if (ccnl_iottlv_prependTL(IOT_TLV_Reply,
                                        oldoffset - *offset, offset, buf) < 0)
        return -1;
    if (contentpos)
        *contentpos -= *offset;

    return oldoffset - *offset;
}

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_SUITE_IOTTLV
#endif // PKT_IOTTLV_C

// eof
