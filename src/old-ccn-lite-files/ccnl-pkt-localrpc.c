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

#include "ccnl-pkt-localrpc.h"

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

int
ccnl_lrpc_fieldlen(unsigned long val)
{
    if (val < 253) return 1;
    if (val <= 0x0ffff) return 3;
    if (val <= 0xffffffffL) return 5;
    return 9;
}

int
ccnl_lrpc_TLlen(unsigned int type, unsigned int len)
{
  /*
    if (type < 4 && len < 64)
        return 1;
    return 1 + ccnl_lrpc_fieldlen(type) + ccnl_lrpc_fieldlen(len);
  */
    return ccnl_lrpc_fieldlen(type) + ccnl_lrpc_fieldlen(len);
}

int
ccnl_lrpc_TLVlen(int typ, int nofbytes)
{
    return ccnl_lrpc_TLlen(typ, nofbytes) + nofbytes;
}

int
ccnl_lrpc_varlenint(unsigned char **buf, int *len, int *val)
{
    return ccnl_ndntlv_varlenint(buf, len, val);
}

int
ccnl_lrpc_dehead(unsigned char **buf, int *len,
                 int *typ, int *vallen)
{
/*
    if (*len <= 0)
        return -1;
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
    if (ccnl_lrpc_varlenint(buf, len, typ))
        return -1;
    if (ccnl_lrpc_varlenint(buf, len, vallen))
        return -1;
//    fprintf(stderr, "==%d %d\n", *typ, *vallen);
    return 0;
}

unsigned long int
ccnl_lrpc_nonNegInt(unsigned char *buf, int vallen)
{
    return ccnl_ndntlv_nonNegInt(buf, vallen);
}

// ----------------------------------------------------------------------

struct rdr_ds_s* ccnl_rdr_unserialize(unsigned char *buf, int buflen)
{
    struct rdr_ds_s *ds;

    ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!ds)
        return NULL;

    ds->flat = buf;
    ds->flatlen = buflen;
    ds->type = LRPC_NOT_SERIALIZED;

    return ds;
}

int ccnl_rdr_getType(struct rdr_ds_s *ds)
{
    unsigned char *buf;
    int typ, vallen, len;
    struct rdr_ds_s *a, *end;

    if (!ds)
        return -2;
    if (ds->type != LRPC_NOT_SERIALIZED)
        return ds->type;

    buf = ds->flat;
    len = ds->flatlen;
/*
    if (*buf < LRPC_APPLICATION) { //  && *buf >= 0x10) { // user defined code point
        ds->type = *buf;
        ds->flatlen = 1;
        return ds->type;
    }
*/
    if (ccnl_lrpc_dehead(&buf, &len, &typ, &vallen))
        return -3;

    //    fprintf(stderr, "typ=%d, len=%d, vallen=%d\n", typ, len, vallen);

    if (vallen > len)
        return -4;
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
        if (!a || ccnl_rdr_getType(a) < 0)
            return -5;
        //        fprintf(stderr, ".. app used %d bytes\n", a->flatlen);
        buf += a->flatlen;
        vallen -= a->flatlen;
        ds->u.fct = a;
        ds->flatlen += a->flatlen;
        ds->type = typ;
        break;
    case LRPC_LAMBDA:
        ds->flatlen = buf - ds->flat;
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0)
            return -6;
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
        if (!a || ccnl_rdr_getType(a) < 0)
            return -5;
        fprintf(stderr, ".. seq/req/rep used %d bytes\n", a->flatlen);
        buf += a->flatlen;
        vallen -= a->flatlen;
        ds->u.aux = a;
        ds->flatlen += a->flatlen;
*/
        ds->type = typ;
        break;
    default:
        return -1;
    }

    //    fprintf(stderr, "== left bytes=%d\n", vallen);
    end = 0;
    while (vallen > 0) {
      //        fprintf(stderr, " *left bytes=%d\n", vallen);
        a = ccnl_rdr_unserialize(buf, vallen);
        if (!a || ccnl_rdr_getType(a) < 0)
            return -10;
        ds->flatlen += a->flatlen;
        if (!end)
            ds->aux = a;
        else
            end->nextinseq = a;
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
    if (!ds)
        return 0;

    ds->type = LRPC_APPLICATION;
    ds->flatlen = -1;
    ds->u.fct = expr;
    ds->aux = arg;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkSeq(void)
{
    struct rdr_ds_s *ds;

    ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));
    if (!ds)
        return 0;

    ds->type = LRPC_SEQUENCE;
    ds->flatlen = -1;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el)
{
    struct rdr_ds_s *p;

    if (!seq) // || seq->type != LRPC_SEQUENCE)
        return NULL;

    if (!seq->aux) {
        seq->aux = el;
        return seq;
    }

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
    if (!seq || seq->type != LRPC_SEQUENCE)
        return NULL;
    return seq->u.nextinseq;
}

struct rdr_ds_s* ccnl_rdr_seqGetEntry(struct rdr_ds_s *seq)
{
    if (!seq || seq->type != LRPC_SEQUENCE)
        return NULL;
    return seq->aux;
}
*/

struct rdr_ds_s* ccnl_rdr_mkNonNegInt(unsigned int val)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = LRPC_NONNEGINT;
    ds->flatlen = -1;
    ds->u.nonnegintval = val;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkCodePoint(unsigned char cp)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = cp;
    ds->flatlen = 1;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkVar(char *s)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = LRPC_FLATNAME;
    ds->flatlen = -1;
    ds->u.namelen = s ? strlen(s) : 0;
    ds->aux = (struct rdr_ds_s*) s;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkBin(char *data, int len)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = LRPC_BIN;
    ds->flatlen = -1;
    ds->u.binlen = len;
    ds->aux = (struct rdr_ds_s*) data;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkStr(char *s)
{
    struct rdr_ds_s *ds = ccnl_rdr_mkVar(s);

    if (ds)
        ds->type = LRPC_STR;
    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkNonce(char *data, int len)
{
    struct rdr_ds_s *ds = ccnl_rdr_mkBin(data, len);

    if (ds)
        ds->type = LRPC_NONCE;
    return ds;
}

int ccnl_rdr_getFlatLen(struct rdr_ds_s *ds) // incl TL header
{
    int len = 0;
    unsigned int val;
    struct rdr_ds_s *aux;

    if (!ds)
        return -1;
    if (ds->flatlen >= 0)
        return ds->flatlen;

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
        len = ccnl_rdr_getFlatLen(ds->u.fct);
        break;
    case LRPC_LAMBDA:
        len = ccnl_rdr_getFlatLen(ds->u.lambdavar);
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
        len += ccnl_rdr_getFlatLen(aux);
        aux = aux->nextinseq;
    }
    len = ccnl_lrpc_TLVlen(ds->type, len);
done:
    ds->flatlen = len;
    return len;
}

static int ccnl_rdr_serialize_fillTorL(long val, unsigned char *buf)
{
    int len, i, t;

    if (val < 253) {
        *buf = val;
        return 1;
    }
    if (val <= 0xffff)
        len = 2, t = 253;
    else if (val <= 0xffffffffL)
        len = 4, t = 254;
    else
        len = 8, t = 255;

    *buf = t;
    for (i = 0, buf += len; i < len; i++, buf--) {
        *buf = val & 0xff;
        val = val >> 8;
    }
    return len + 1;
}

static int ccnl_rdr_serialize_fillTandL(int typ, int len, unsigned char *buf)
{
  /*
    int i = 1;

    if (typ < 4 && len < 64) {
        *buf = (typ << 6) | len;
    } else {
  */
    int i = 0;
    {
        *buf = 0;
        i += ccnl_rdr_serialize_fillTorL(typ, buf + i);
        i += ccnl_rdr_serialize_fillTorL(len, buf + i);
    }

//    fprintf(stderr, "typ=%d len=%d, bytes=%d\n", typ, len, i);
    return i;
}

static void ccnl_rdr_serialize_fill(struct rdr_ds_s *ds, unsigned char *at)
{
    int len;
    struct rdr_ds_s *goesfirst, *aux;
    unsigned int val;

    if (!ds)
        return;

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
        return;
    case LRPC_BIN:
        len = ccnl_rdr_serialize_fillTandL(LRPC_BIN, ds->u.binlen, at);
        memcpy(at + len, ds->aux, ds->u.binlen);
        return;
    case LRPC_STR:
        len = ccnl_rdr_serialize_fillTandL(LRPC_STR, ds->u.strlen, at);
        memcpy(at + len, ds->aux, ds->u.strlen);
        return;
    case LRPC_NONCE:
        len = ccnl_rdr_serialize_fillTandL(LRPC_NONCE, ds->u.binlen, at);
        memcpy(at + len, ds->aux, ds->u.binlen);
        return;
    case LRPC_NONNEGINT:
        len = ds->flatlen;
        ccnl_rdr_serialize_fillTandL(LRPC_NONNEGINT, len-2, at);
        at += len - 1;
        val = ds->u.nonnegintval;
        for (len -= 2; len > 0; len--) {
            *at = val & 0x0ff;
            val = val >> 8;
            at--;
        }
        return;

    case LRPC_APPLICATION:
        len = ccnl_rdr_getFlatLen(ds->u.fct);
        goesfirst = ds->u.fct;
        break;
    case LRPC_PT_REQUEST:
    case LRPC_PT_REPLY:
    case LRPC_SEQUENCE:
        len = 0;
        goesfirst = NULL;
        break;
    case LRPC_LAMBDA:
        len = ccnl_rdr_getFlatLen(ds->u.lambdavar);
        goesfirst = ds->u.lambdavar;
        break;
    default:
        DEBUGMSG(WARNING, "serialize_fill() error (ds->type=%d)\n", ds->type);
        return;
    }

    aux = ds->aux;
    while (aux) {
        len += ccnl_rdr_getFlatLen(aux);
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
}

int ccnl_rdr_serialize(struct rdr_ds_s *ds, unsigned char *buf, int buflen)
{
    int len;

    if (!ds)
        return -1;

    len = ccnl_rdr_getFlatLen(ds);
    if (len > buflen)
        return -1;
    ccnl_rdr_serialize_fill(ds, buf);

    return len;
}

#endif // USE_SUITE_LOCALRPC

// eof
