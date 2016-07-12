/*
 * @f pkt-ccnb.c
 * @b CCN lite - parse, create and manipulate CCNb formatted packets
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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
 * 2011-03-13 created (cft): orig name ccnl-parse-ccnb.c
 * 2014-03-17 renamed to prepare for a world with many wire formats
 * 2014-03-20 extracted from ccnl-core.c
 * 2014-11-05 merged from pkt-ccnb-dec.c and pkt-ccnb-enc.c
 */



#ifdef USE_SUITE_CCNB

#include "ccnl-pkt-ccnb.h"

int
ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len,
                  unsigned char **valptr, int *vallen);

// ----------------------------------------------------------------------
// ccnb parsing support

int
ccnl_ccnb_dehead(unsigned char **buf, int *len, int *num, int *typ)
{
    int i;
    int val = 0;

    if (*len > 0 && **buf == 0) { // end
        *num = *typ = 0;
        *buf += 1;
        *len -= 1;
        return 0;
    }
    for (i = 0; (unsigned int) i < sizeof(i) && i < *len; i++) {
        unsigned char c = (*buf)[i];
        if ( c & 0x80 ) {
            *num = (val << 4) | ((c >> 3) & 0xf);
            *typ = c & 0x7;
            *buf += i+1;
            *len -= i+1;
            return 0;
        }
        val = (val << 7) | c;
    }
    return -1;
}

static int
ccnl_ccnb_hunt_for_end(unsigned char **buf, int *len,
             unsigned char **valptr, int *vallen)
{
    int typ, num;

    while (ccnl_ccnb_dehead(buf, len, &num, &typ) == 0) {
        if (num==0 && typ==0)
            return 0;
        if (ccnl_ccnb_consume(typ, num, buf, len, valptr, vallen) < 0)
            return -1;
    }
    return -1;
}

int
ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len,
                  unsigned char **valptr, int *vallen)
{
    if (typ == CCN_TT_BLOB || typ == CCN_TT_UDATA) {
        if (valptr)  *valptr = *buf;
        if (vallen)  *vallen = num;
        *buf += num, *len -= num;
        return 0;
    }
    if (typ == CCN_TT_DTAG || typ == CCN_TT_DATTR)
        return ccnl_ccnb_hunt_for_end(buf, len, valptr, vallen);
//  case CCN_TT_TAG, CCN_TT_ATTR:
//  case DTAG, DATTR:
    return -1;
}

int
ccnl_ccnb_data2uint(unsigned char *cp, int len)
{
    int i, val;

    for (i = 0, val = 0; i < len; i++)
        if (isdigit(cp[i]))
            val = 10*val + cp[i] - '0';
        else
            return -1;
    return val;
}

struct ccnl_pkt_s*
ccnl_ccnb_bytes2pkt(unsigned char *start, unsigned char **data, int *datalen)
{
    struct ccnl_pkt_s *pkt;
    unsigned char *cp;
    int num, typ, len, oldpos;
    struct ccnl_prefix_s *p;

    DEBUGMSG(TRACE, "ccnl_ccnb_extract\n");

    //pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(*pkt));
    pkt = (struct ccnl_pkt_s *) ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;
    pkt->suite = CCNL_SUITE_CCNB;
    pkt->val.final_block_id = -1;
    pkt->s.ccnb.scope = 3;
    pkt->s.ccnb.aok = 3;
    pkt->s.ccnb.maxsuffix = CCNL_MAX_NAME_COMP;

    pkt->pfx = p = ccnl_prefix_new(CCNL_SUITE_CCNB, CCNL_MAX_NAME_COMP);
    if (!p) {
        ccnl_free(pkt);
        return NULL;
    }
    p->compcnt = 0;

    oldpos = *data - start;
    while (ccnl_ccnb_dehead(data, datalen, &num, &typ) == 0) {
        if (num==0 && typ==0) // end
            break;
        if (typ == CCN_TT_DTAG) {
            switch (num) {
            case CCN_DTAG_NAME:
                p->nameptr = start + oldpos;
                for (;;) {
                    if (ccnl_ccnb_dehead(data, datalen, &num, &typ) != 0)
                        goto Bail;
                    if (num==0 && typ==0)
                        break;
                    if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                        p->compcnt < CCNL_MAX_NAME_COMP) {
                        if (ccnl_ccnb_hunt_for_end(data, datalen, p->comp + p->compcnt,
                                p->complen + p->compcnt) < 0) goto Bail;
                        p->compcnt++;
                    } else {
                        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0)
                            goto Bail;
                    }
                }
                p->namelen = *data - p->nameptr;
#ifdef USE_NFN
                if (p->compcnt > 0 && p->complen[p->compcnt-1] == 3 &&
                                    !memcmp(p->comp[p->compcnt-1], "NFN", 3)) {
                    p->nfnflags |= CCNL_PREFIX_NFN;
                    p->compcnt--;
                }
#ifdef USE_TIMEOUT_KEEPALIVE
                if (p->compcnt > 0 && p->complen[p->compcnt-1] == 5 &&
                                    !memcmp(p->comp[p->compcnt-1], "ALIVE", 5)) {
                    p->nfnflags |= CCNL_PREFIX_KEEPALIVE;
                    p->compcnt--;
                }
#endif
#endif
                break;
            case CCN_DTAG_CONTENT:
                if (ccnl_ccnb_consume(typ, num, data, datalen,
                                      &pkt->content, &pkt->contlen) < 0)
                    goto Bail;
                oldpos = *data - start;
                continue;
            case CCN_DTAG_SCOPE:
            case CCN_DTAG_ANSWERORIGKIND:
            case CCN_DTAG_MINSUFFCOMP:
            case CCN_DTAG_MAXSUFFCOMP:
            case CCN_DTAG_NONCE:
            case CCN_DTAG_PUBPUBKDIGEST:
                if (ccnl_ccnb_hunt_for_end(data, datalen, &cp, &len) < 0)
                    goto Bail;
                switch (num) {
                case CCN_DTAG_SCOPE:
                    if (len == 1)
                        pkt->s.ccnb.scope = isdigit(*cp) && (*cp < '3') ?
                            *cp - '0' : -1;
                    break;
                case CCN_DTAG_ANSWERORIGKIND:
                    pkt->s.ccnb.aok = ccnl_ccnb_data2uint(cp, len);
                    break;
                case CCN_DTAG_MINSUFFCOMP:
                    pkt->s.ccnb.minsuffix = ccnl_ccnb_data2uint(cp, len);
                    break;
                case CCN_DTAG_MAXSUFFCOMP:
                    pkt->s.ccnb.maxsuffix = ccnl_ccnb_data2uint(cp, len);
                    break;
                case CCN_DTAG_NONCE:
                    if (!pkt->s.ccnb.nonce)
                        pkt->s.ccnb.nonce = ccnl_buf_new(cp, len);
                    break;
                case CCN_DTAG_PUBPUBKDIGEST:
                    if (!pkt->s.ccnb.ppkd)
                        pkt->s.ccnb.ppkd = ccnl_buf_new(cp, len);
                    break;
                case CCN_DTAG_EXCLUDE:
                    DEBUGMSG(WARNING, "ccnb 'exclude' field ignored\n");
                    break;
                }
                break;
            default:
                if (ccnl_ccnb_hunt_for_end(data, datalen, &cp, &len) < 0)
                    goto Bail;
            }
            oldpos = *data - start;
            continue;
        }
        if (ccnl_ccnb_consume(typ, num, data, datalen, 0, 0) < 0)
            goto Bail;
        oldpos = *data - start;
    }
    pkt->pfx = p;
    pkt->buf = ccnl_buf_new(start, *data - start);
    // carefully rebase ptrs to new buf because of 64bit pointers:
    if (pkt->content)
        pkt->content = pkt->buf->data + (pkt->content - start);
    for (num = 0; num < p->compcnt; num++)
            p->comp[num] = pkt->buf->data + (p->comp[num] - start);
    if (p->nameptr)
        p->nameptr = pkt->buf->data + (p->nameptr - start);

    return pkt;
Bail:
    free_packet(pkt);
    return NULL;
}

int
ccnl_ccnb_unmkBinaryInt(unsigned char **data, int *datalen,
                        unsigned int *result, unsigned char *width)
{
    unsigned char *cp = *data;
    int len = *datalen, typ, num;
    unsigned int val = 0;

    if (ccnl_ccnb_dehead(&cp, &len, &num, &typ) != 0 || typ != CCN_TT_BLOB)
        return -1;
    if (width) {
      if (*width < num)
          num = *width;
      else
          *width = num;
    }

    // big endian (network order):
    while (num-- > 0 && len > 0) {
        val = (val << 8) | *cp++;
        len--;
    }
    *result = val;

    if (len < 1 || *cp != '\0') // no end-of-entry
        return -1;
    *data = cp+1;
    *datalen = len-1;
    return 0;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PREFIX_MATCHING

// returns: 0=match, -1=otherwise
int
ccnl_ccnb_cMatch(struct ccnl_pkt_s *p, struct ccnl_content_s *c)
{
    assert(p);
    assert(p->suite == CCNL_SUITE_CCNB);

    if (!ccnl_i_prefixof_c(p->pfx, p->s.ccnb.minsuffix, p->s.ccnb.maxsuffix, c))
        return -1;
    if (p->s.ccnb.ppkd && !buf_equal(p->s.ccnb.ppkd, c->pkt->s.ccnb.ppkd))
        return -1;
    // FIXME: should check stale bit in aok here
    return 0;
}
#endif

// ----------------------------------------------------------------------
// ccnb encoding support

#ifdef NEEDS_PACKET_CRAFTING

int
ccnl_ccnb_mkHeader(unsigned char *buf, unsigned int num, unsigned int tt)
{
    unsigned char tmp[100];
    int len = 0, i;

    *tmp = 0x80 | ((num & 0x0f) << 3) | tt;
    len = 1;
    num = num >> 4;

    while (num > 0) {
        tmp[len++] = num & 0x7f;
        num = num >> 7;
    }
    for (i = len-1; i >= 0; i--)
        *buf++ = tmp[i];
    return len;
}

int
ccnl_ccnb_addBlob(unsigned char *out, char *cp, int cnt)
{
    int len;

    len = ccnl_ccnb_mkHeader(out, cnt, CCN_TT_BLOB);
    memcpy(out+len, cp, cnt);
    len += cnt;

    return len;
}

int
ccnl_ccnb_mkField(unsigned char *out, unsigned int num, int typ,
                  unsigned char *data, int datalen)
{
    int len;

    len = ccnl_ccnb_mkHeader(out, num, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out + len, datalen, typ);
    memcpy(out + len, data, datalen);
    len += datalen;
    out[len++] = 0; // end-of-field

    return len;
}

int
ccnl_ccnb_mkBlob(unsigned char *out, unsigned int num, unsigned int tt,
                 char *cp, int cnt)
{
    (void) tt;
    return ccnl_ccnb_mkField(out, num, CCN_TT_BLOB,
                             (unsigned char*) cp, cnt);
}

int
ccnl_ccnb_mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt,
                    char *str)
{
    (void) tt;
    return ccnl_ccnb_mkField(out, num, CCN_TT_BLOB,
                             (unsigned char*) str, strlen(str));
}

int
ccnl_ccnb_mkBinaryInt(unsigned char *out, unsigned int num, unsigned int tt,
                      unsigned int val, int bytes)
{
    int len = ccnl_ccnb_mkHeader(out, num, tt);

    if (!bytes) {
        for (bytes = sizeof(val) - 1; bytes > 0; bytes--)
            if (val >> (8*bytes))
                break;
        bytes++;
    }
    len += ccnl_ccnb_mkHeader(out+len, bytes, CCN_TT_BLOB);

    while (bytes > 0) { // big endian
        bytes--;
        out[len++] = 0x0ff & (val >> (8*bytes));
    }

    out[len++] = 0; // end-of-entry
    return len;
}

int
ccnl_ccnb_mkComponent(unsigned char *val, int vallen, unsigned char *out)
{
    int len;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    len += ccnl_ccnb_mkHeader(out+len, vallen, CCN_TT_BLOB);
    memcpy(out+len, val, vallen);
    len += vallen;
    out[len++] = 0; // end-of-component

    return len;
}

int
ccnl_ccnb_mkName(struct ccnl_prefix_s *name, unsigned char *out)
{
    int len, i;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    for (i = 0; i < name->compcnt; i++) {
        len += ccnl_ccnb_mkComponent(name->comp[i], name->complen[i], out+len);
    }
#ifdef USE_NFN
    if (name->nfnflags & CCNL_PREFIX_NFN) {
        len += ccnl_ccnb_mkComponent((unsigned char*) "NFN", 3, out+len);
    }
#endif
    out[len++] = 0; // end-of-name

    return len;
}

// ----------------------------------------------------------------------

int
ccnl_ccnb_fillInterest(struct ccnl_prefix_s *name, int *nonce,
                       unsigned char *out, int outlen)
{
    (void) outlen;
    int len = 0;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += ccnl_ccnb_mkName(name, out+len);
    if (nonce) {
        len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
        len += ccnl_ccnb_mkHeader(out+len, sizeof(unsigned int), CCN_TT_BLOB);
        memcpy(out+len, (void*)nonce, sizeof(unsigned int));
        len += sizeof(unsigned int);
    }
    out[len++] = 0; // end-of-interest

    return len;
}

int
ccnl_ccnb_fillContent(struct ccnl_prefix_s *name, unsigned char *data,
                      int datalen, int *contentpos, unsigned char *out)
{
    int len = 0;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);

    len += ccnl_ccnb_mkName(name, out+len);
    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += ccnl_ccnb_mkHeader(out+len, datalen, CCN_TT_BLOB);
    if (contentpos)
        *contentpos = len;
    memcpy(out+len, data, datalen);
    if (contentpos)
        *contentpos = len;
    len += datalen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-content obj

    return len;
}

#endif // NEEDS_PACKET_CRAFTING

#endif // USE_SUITE_CCNB

// eof
