/*
 * @f ccnl-pkt-localrpc.c
 * @b CCN lite - local RPC Data Representation (RDR) de- and encoding
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
 * 2014-06-18 created
 * 2014-11-05 merged from pkt-localrpc-dec.c and pkt-localrpc-enc.c
 */

#ifdef USE_SUITE_LOCALRPC
#ifndef CCNL_LINUXKERNEL
#include "ccnl-pkt-localrpc.h"
#include "ccnl-core.h"
#include "ccnl-pkt-ndntlv.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ccnl-pkt-localrpc.h>

#else
#include "../include/ccnl-pkt-localrpc.h"
#include "../../ccnl-core/include/ccnl-core.h"
#include "../../ccnl-pkt/include/ccnl-pkt-ndntlv.h"
#endif

// ----------------------------------------------------------------------
// RDR decoding library
/*

  design philosophy: we only unserialize what we really have to, which
  is signaled through the "getType()" call.

  That is: The "unserialize()" call only notes the given byte buffer
  but does not do any decoding.

  A first "getType()" decodes just as much is needed to give an answer.

  If the resulting representation data structure has subfields, you
  have to access them individually and do a "getType()" there, too.

  Note that you have to manage yourself the buffer for unserialization.
  In particluar you should only release it when the representation tree
  does not need access to it anymore.

*/

// ----------------------------------------------------------------------
// TLV encoding, decoding

size_t
ccnl_lrpc_fieldlen(uint64_t val)
{
    if (val < 253U) {
        return 1;
    }
    if (val <= 0x0ffffU) {
        return 3;
    }
    if (val <= 0xffffffffUL) {
        return 5;
    }
    return 9;
}

size_t
ccnl_lrpc_TLlen(uint64_t type, size_t len)
{
  /*
    if (type < 4 && len < 64)
        return 1;
    return 1 + ccnl_lrpc_fieldlen(type) + ccnl_lrpc_fieldlen(len);
  */
    return ccnl_lrpc_fieldlen(type) + ccnl_lrpc_fieldlen(len);
}

size_t
ccnl_lrpc_TLVlen(int32_t typ, size_t nofbytes)
{
    // This function is ony ever called with an >=0 enum constant as typ
    return ccnl_lrpc_TLlen((uint64_t) typ, nofbytes) + nofbytes;
}

int8_t
ccnl_lrpc_varlenint(uint8_t **buf, size_t *len, uint64_t *val)
{
    return ccnl_ndntlv_varlenint(buf, len, val);
}

int8_t
ccnl_lrpc_dehead(uint8_t **buf, size_t *len,
                 uint64_t *typ, size_t *vallen)
{
/*
    if (*len <= 0) {
        return -1;
    }
    if (**buf != 0) {
        *typ = **buf >> 6;
        *vallen = **buf & 0x3f;
        (*buf)++;
        (*len)--;
        return 0;
    }
//    fprintf(stderr, "==\n");
    (*buf)++;
    (*len)--;
*/
    uint64_t vallen_int;
    if (ccnl_lrpc_varlenint(buf, len, typ)) {
        return -1;
    }
    if (ccnl_lrpc_varlenint(buf, len, &vallen_int)) {
        return -1;
    }
    if (vallen_int > SIZE_MAX) {
        return -1;
    }
    *vallen = (size_t) vallen_int;
//    fprintf(stderr, "==%d %d\n", *typ, *vallen);
    return 0;
}

uint64_t
ccnl_lrpc_nonNegInt(uint8_t *buf, size_t vallen)
{
    return ccnl_ndntlv_nonNegInt(buf, vallen);
}

// ----------------------------------------------------------------------

struct rdr_ds_s* ccnl_rdr_unserialize(uint8_t *buf, size_t buflen)
{
    struct rdr_ds_s *ds;

    ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!ds) {
        return NULL;
    }

    ds->flat = buf;
    ds->flatlen = buflen;
    ds->type = LRPC_NOT_SERIALIZED;

    return ds;
}

int32_t ccnl_rdr_getType(struct rdr_ds_s *ds)
{
    uint8_t *buf;
    uint64_t typ;
    size_t vallen, len;
    struct rdr_ds_s *a, *end;

    if (!ds || !ds->flatlen) {
        return -2;
    }
    if (ds->type != LRPC_NOT_SERIALIZED) {
        return ds->type;
    }

    buf = ds->flat;
    len = ds->flatlen;
/*
    if (*buf < LRPC_APPLICATION) { //  && *buf >= 0x10) { // user defined code point
        ds->type = *buf;
        ds->flatlen = 1;
        return ds->type;
    }
*/
    if (ccnl_lrpc_dehead(&buf, &len, &typ, &vallen)) {
        return -3;
    }
    if (typ > INT32_MAX) {
        return -3;
    }

    //    fprintf(stderr, "typ=%d, len=%d, vallen=%d\n", typ, len, vallen);

    if (vallen > len) {
        return -4;
    }
    switch (typ) {
    case LRPC_NONNEGINT:
        ds->u.nonnegintval = ccnl_lrpc_nonNegInt(buf, vallen);
        ds->type = LRPC_NONNEGINT;
        ds->flatlen = (buf - ds->flat) + vallen;
        return 0;
    case LRPC_FLATNAME:
        ds->u.namelen = vallen;
        ds->aux = (struct rdr_ds_s*) buf;
        ds->type = LRPC_FLATNAME;
        ds->flatlen = (buf - ds->flat) + vallen;
        return 0;
    case LRPC_BIN:
        ds->u.binlen = vallen;
        ds->aux = (struct rdr_ds_s*) buf;
        ds->type = LRPC_BIN;
        ds->flatlen = (buf - ds->flat) + vallen;
        return 0;
    case LRPC_STR:
        ds->u.strlen = vallen;
        ds->aux = (struct rdr_ds_s*) buf;
        ds->type = LRPC_STR;
        ds->flatlen = (buf - ds->flat) + vallen;
        return 0;
    case LRPC_NONCE:
        ds->u.binlen = vallen;
        ds->aux = (struct rdr_ds_s*) buf;
        ds->type = LRPC_NONCE;
        ds->flatlen = (buf - ds->flat) + vallen;
        return 0;

    case LRPC_APPLICATION:
        ds->flatlen = buf - ds->flat;
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0) {
            return -5;
        }
        //        fprintf(stderr, ".. app used %d bytes\n", a->flatlen);
        buf += a->flatlen;
        vallen -= a->flatlen;
        ds->u.fct = a;
        ds->flatlen += a->flatlen;
        ds->type = (int32_t) typ;
        break;
    case LRPC_LAMBDA:
        ds->flatlen = buf - ds->flat;
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0) {
            return -6;
        }
        buf += a->flatlen;
        vallen -= a->flatlen;
        ds->u.lambdavar = a;
        ds->flatlen += a->flatlen;
        ds->type = LRPC_LAMBDA;
        break;
    case LRPC_PT_REQUEST:
    case LRPC_PT_REPLY:
    case LRPC_SEQUENCE:
        ds->flatlen = buf - ds->flat;
/*
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0) {
            return -5;
        }
        fprintf(stderr, ".. seq/req/rep used %d bytes\n", a->flatlen);
        buf += a->flatlen;
        vallen -= a->flatlen;
        ds->u.aux = a;
        ds->flatlen += a->flatlen;
*/
        ds->type = (int32_t) typ;
        break;
    default:
        return -1;
    }

    //    fprintf(stderr, "== left bytes=%d\n", vallen);
    end = 0;
    while (vallen > 0) {
      //        fprintf(stderr, " *left bytes=%d\n", vallen);
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0) {
            return -10;
        }
        ds->flatlen += a->flatlen;
        if (!end) {
            ds->aux = a;
        } else {
            end->nextinseq = a;
        }
        end = a;
        buf += a->flatlen;
        vallen -= a->flatlen;
    }

    return ds->type;
}

// ----------------------------------------------------------------------
// RDR encoding library

void ccnl_rdr_free(struct rdr_ds_s *x)
{
    while (x) {
        struct rdr_ds_s *y = x->nextinseq;

        switch (x->type) {
        case LRPC_PT_REQUEST:
        case LRPC_APPLICATION:
            ccnl_rdr_free(x->u.fct);
            ccnl_rdr_free(x->aux);
            break;
        case LRPC_LAMBDA:
            ccnl_rdr_free(x->u.lambdavar);
            ccnl_rdr_free(x->aux);
            break;
        case LRPC_SEQUENCE:
            ccnl_rdr_free(x->aux);
            break;
        case LRPC_FLATNAME:
        case LRPC_NONCE:
        case LRPC_BIN:
        case LRPC_STR:
            // do not free ->aux as it points into the packet byte array
        case LRPC_NONNEGINT:
        default:
            break;
        }
        ccnl_free(x);
        x = y;
    }
}

struct rdr_ds_s* ccnl_rdr_mkApp(struct rdr_ds_s *expr, struct rdr_ds_s *arg)
{
    struct rdr_ds_s *ds;

    ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!ds) {
        return 0;
    }

    ds->type = LRPC_APPLICATION;
    ds->u.fct = expr;
    ds->aux = arg;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkSeq(void)
{
    struct rdr_ds_s *ds;

    ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!ds) {
        return 0;
    }

    ds->type = LRPC_SEQUENCE;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el)
{
    struct rdr_ds_s *p;

    if (!seq) {// || seq->type != LRPC_SEQUENCE) {
        return NULL;
    }

    if (!seq->aux) {
        seq->aux = el;
        return seq;
    }

    // Bodiless for - iterate to the last nextinseq
    for (p = seq->aux; p->nextinseq; p = p->nextinseq) {}

    p->nextinseq = el;
/*
    p->u.nextinseq = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!p->u.nextinseq) {
        // should free old data structures of seq
        return 0;
    }
    p = p->u.nextinseq;
    p->type = LRPC_SEQUENCE;
    p->flatlen = -1;
    p->aux = el;

    seq->flatlen = -1;
*/
    return seq;
}

/*
struct rdr_ds_s* ccnl_rdr_seqGetNext(struct rdr_ds_s *seq)
{
    if (!seq || seq->type != LRPC_SEQUENCE) {
        return NULL;
    }
    return seq->u.nextinseq;
}

struct rdr_ds_s* ccnl_rdr_seqGetEntry(struct rdr_ds_s *seq)
{
    if (!seq || seq->type != LRPC_SEQUENCE) {
        return NULL;
    }
    return seq->aux;
}
*/

struct rdr_ds_s* ccnl_rdr_mkNonNegInt(uint64_t val)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) {
        return 0;
    }
    ds->type = LRPC_NONNEGINT;
    ds->u.nonnegintval = val;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkCodePoint(unsigned char cp)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) {
        return 0;
    }
    ds->type = cp;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkVar(char *s)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) {
        return 0;
    }
    ds->type = LRPC_FLATNAME;
    ds->u.namelen = s ? strlen(s) : 0;
    ds->aux = (struct rdr_ds_s*) s;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkBin(uint8_t *data, size_t len)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) {
        return 0;
    }
    ds->type = LRPC_BIN;
    ds->u.binlen = len;
    ds->aux = (struct rdr_ds_s*) data;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkStr(char *s)
{
    struct rdr_ds_s *ds = ccnl_rdr_mkVar(s);

    if (ds) {
        ds->type = LRPC_STR;
    }
    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkNonce(uint8_t *data, size_t len)
{
    struct rdr_ds_s *ds = ccnl_rdr_mkBin(data, len);

    if (ds) {
        ds->type = LRPC_NONCE;
    }
    return ds;
}

int8_t ccnl_rdr_getFlatLen(struct rdr_ds_s *ds, size_t *flatlen) // incl TL header
{
    size_t len = 0;
    uint64_t val;
    struct rdr_ds_s *aux;

    if (!ds) {
        return -1;
    }
    if (ds->flat) {
        *flatlen = ds->flatlen;
        return 0;
    }

    switch(ds->type) {
    case LRPC_FLATNAME:
        len = ccnl_lrpc_TLVlen(LRPC_FLATNAME, ds->u.namelen);
        goto done;
    case LRPC_NONNEGINT:
        val = ds->u.nonnegintval;
        do {
            val = val >> 8;
            len++;
        } while (val);
        len = ccnl_lrpc_TLVlen(LRPC_NONNEGINT, len);
        goto done;
    case LRPC_BIN:
        len = ccnl_lrpc_TLVlen(LRPC_BIN, ds->u.binlen);
        goto done;
    case LRPC_STR:
        len = ccnl_lrpc_TLVlen(LRPC_FLATNAME, ds->u.strlen);
        goto done;
    case LRPC_NONCE:
        len = ccnl_lrpc_TLVlen(LRPC_NONCE, ds->u.binlen);
        goto done;

    case LRPC_APPLICATION:
        if (ccnl_rdr_getFlatLen(ds->u.fct, &len)) {
            return -1;
        }
        break;
    case LRPC_LAMBDA:
        if (ccnl_rdr_getFlatLen(ds->u.lambdavar, &len)) {
            return -1;
        }
        break;
    case LRPC_PT_REQUEST:
    case LRPC_PT_REPLY:
    case LRPC_SEQUENCE:
        len = 0;
        break;
    default:
        return -1;
    }
    aux = ds->aux;
    while (aux) {
        if (ccnl_rdr_getFlatLen(aux, &len)) {
            return -1;
        }
        aux = aux->nextinseq;
    }
    len = ccnl_lrpc_TLVlen(ds->type, len);
done:
    ds->flatlen = len;
    *flatlen += len;
    return 0;
}

static size_t
ccnl_rdr_serialize_fillTorL(uint64_t val, uint8_t *buf)
{
    size_t len, i ;
    uint8_t t;

    if (val < 253U) {
        *buf = (uint8_t) val;
        return 1;
    }
    if (val <= 0xffffU) {
        len = 2, t = 253;
    } else if (val <= 0xffffffffUL) {
        len = 4, t = 254;
    } else {
        len = 8, t = 255;
    }

    *buf = t;
    for (i = 0, buf += len; i < len; i++, buf--) {
        *buf = (uint8_t) (val & 0xff);
        val = val >> 8;
    }
    return len + 1;
}

static size_t
ccnl_rdr_serialize_fillTandL(int typ, size_t len, uint8_t *buf)
{
  /*
    int i = 1;

    if (typ < 4 && len < 64) {
        *buf = (typ << 6) | len;
    } else {
  */
    size_t i = 0;
    {
        *buf = 0;
        i += ccnl_rdr_serialize_fillTorL((uint64_t) typ, buf + i); //FIXME: Is this ever called with negative type?
        i += ccnl_rdr_serialize_fillTorL(len, buf + i);
    }

//    fprintf(stderr, "typ=%d len=%d, bytes=%d\n", typ, len, i);
    return i;
}

static int8_t ccnl_rdr_serialize_fill(struct rdr_ds_s *ds, uint8_t *at)
{
    size_t len;
    struct rdr_ds_s *goesfirst, *aux;
    uint64_t val;

    if (!ds) {
        return -1;
    }

/*
    if (ds->type < LRPC_APPLICATION) { // user defined code point
        *at = ds->type;
        return;
    }
*/

    switch(ds->type) {
    case LRPC_FLATNAME:
        len = ccnl_rdr_serialize_fillTandL(LRPC_FLATNAME, ds->u.namelen, at);
        memcpy(at + len, ds->aux, ds->u.namelen);
        return 0;
    case LRPC_BIN:
        len = ccnl_rdr_serialize_fillTandL(LRPC_BIN, ds->u.binlen, at);
        memcpy(at + len, ds->aux, ds->u.binlen);
        return 0;
    case LRPC_STR:
        len = ccnl_rdr_serialize_fillTandL(LRPC_STR, ds->u.strlen, at);
        memcpy(at + len, ds->aux, ds->u.strlen);
        return 0;
    case LRPC_NONCE:
        len = ccnl_rdr_serialize_fillTandL(LRPC_NONCE, ds->u.binlen, at);
        memcpy(at + len, ds->aux, ds->u.binlen);
        return 0;
    case LRPC_NONNEGINT:
        len = ds->flatlen;
        ccnl_rdr_serialize_fillTandL(LRPC_NONNEGINT, len-2, at);
        at += len - 1;
        val = ds->u.nonnegintval;
        for (len -= 2; len > 0; len--) {
            *at = (uint8_t) (val & 0x0ff);
            val = val >> 8;
            at--;
        }
        return 0;

    case LRPC_APPLICATION:
        if (ccnl_rdr_getFlatLen(ds->u.fct, &len)) {
            return -1;
        }
        goesfirst = ds->u.fct;
        break;
    case LRPC_PT_REQUEST:
    case LRPC_PT_REPLY:
    case LRPC_SEQUENCE:
        len = 0;
        goesfirst = NULL;
        break;
    case LRPC_LAMBDA:
        if (ccnl_rdr_getFlatLen(ds->u.lambdavar, &len)) {
            return -1;
        }
        goesfirst = ds->u.lambdavar;
        break;
    default:
        DEBUGMSG(WARNING, "serialize_fill() error (ds->type=%d)\n", ds->type);
        return -1;
    }

    aux = ds->aux;
    while (aux) {
        if (ccnl_rdr_getFlatLen(aux, &len)) {
            return -1;
        }
        aux = aux->nextinseq;
    }
    at += ccnl_rdr_serialize_fillTandL(ds->type, len, at);
    if (goesfirst) {
        ccnl_rdr_serialize_fill(goesfirst, at);
        at += goesfirst->flatlen;
    }
    aux = ds->aux;
    while (aux) {
        ccnl_rdr_serialize_fill(aux, at);
        at += aux->flatlen;
        aux = aux->nextinseq;
    }
    return 0;
}

int8_t
ccnl_rdr_serialize(struct rdr_ds_s *ds, uint8_t *buf, size_t buflen, size_t *res)
{
    size_t len;

    if (!ds) {
        return -1;
    }

    if (ccnl_rdr_getFlatLen(ds, &len)) {
        return -1;
    }
    if (len > buflen) {
        return -1;
    }
    ccnl_rdr_serialize_fill(ds, buf);

    *res = len;
    return 0;
}

#endif // USE_SUITE_LOCALRPC

/* suppress empty translation unit error */
typedef int unused_typedef;

// eof
