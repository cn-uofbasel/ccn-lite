/*
 * @f pkt-localrpc-enc.h
 * @b CCN lite - local RPC Data Representation (RDR) encoding routines
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
 */

// #include "pkt-ndntlv.h"
// #include "pkt-localrpc.h"

// RDR encoding library

void ccnl_rdr_free(struct rdr_ds_s *x)
{
    while (x) {
	struct rdr_ds_s *y = x->nextinseq;

	switch (x->type) {
	case NDN_TLV_RPC_APPLICATION:
	    ccnl_rdr_free(x->u.fct);
	    ccnl_rdr_free(x->aux);
	    break;
	case NDN_TLV_RPC_LAMBDA:
	    ccnl_rdr_free(x->u.lambdavar);
	    ccnl_rdr_free(x->aux);
	    break;
	case NDN_TLV_RPC_SEQUENCE:
	    ccnl_rdr_free(x->aux);
	    break;
	case NDN_TLV_RPC_NAME:
	case NDN_TLV_RPC_NONNEGINT:
	case NDN_TLV_RPC_BIN:
	case NDN_TLV_RPC_STR:
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

    ds->type = NDN_TLV_RPC_APPLICATION;
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

    ds->type = NDN_TLV_RPC_SEQUENCE;
    ds->flatlen = -1;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el)
{
    struct rdr_ds_s *p;

    if (!seq) // || seq->type != NDN_TLV_RPC_SEQUENCE)
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
    p->type = NDN_TLV_RPC_SEQUENCE;
    p->flatlen = -1;
    p->aux = el;

    seq->flatlen = -1;
*/
    return seq;
}

/*
struct rdr_ds_s* ccnl_rdr_seqGetNext(struct rdr_ds_s *seq)
{
    if (!seq || seq->type != NDN_TLV_RPC_SEQUENCE)
	return NULL;
    return seq->u.nextinseq;
}

struct rdr_ds_s* ccnl_rdr_seqGetEntry(struct rdr_ds_s *seq)
{
    if (!seq || seq->type != NDN_TLV_RPC_SEQUENCE)
	return NULL;
    return seq->aux;
}
*/

struct rdr_ds_s* ccnl_rdr_mkNonNegInt(unsigned int val)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_NONNEGINT;
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
    ds->type = NDN_TLV_RPC_NAME;
    ds->flatlen = -1;
    ds->u.namelen = s ? strlen(s) : 0;
    ds->aux = (struct rdr_ds_s*) s;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkBin(char *data, int len)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) ccnl_calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_BIN;
    ds->flatlen = -1;
    ds->u.binlen = len;
    ds->aux = (struct rdr_ds_s*) data;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkStr(char *s)
{
    struct rdr_ds_s *ds = ccnl_rdr_mkVar(s);

    if (ds)
	ds->type = NDN_TLV_RPC_STR;
    return ds;
}


int ndn_tlv_fieldlen(unsigned long val)
{
    if (val < 253) return 1;
    if (val <= 0x0ffff) return 3;
    if (val <= 0xffffffffL) return 5;
    return 9;
}

static int ndn_tlv_len(int typ, int nofbytes)
{
    return ndn_tlv_fieldlen(typ) + ndn_tlv_fieldlen(nofbytes) + nofbytes;
}

int ccnl_rdr_getFlatLen(struct rdr_ds_s *ds) // incl TL header
{
    int len;
    unsigned int val;
    struct rdr_ds_s *aux;

    if (!ds)
	return -1;
    if (ds->flatlen >= 0)
	return ds->flatlen;

    switch(ds->type) {
    case NDN_TLV_RPC_NAME:
	len = ndn_tlv_len(NDN_TLV_RPC_NAME, ds->u.namelen);
	goto done;
    case NDN_TLV_RPC_NONNEGINT:
	val = ds->u.nonnegintval;
	len = 0;
	do {
	    val = val >> 8;
	    len++;
	} while (val);
	len = ndn_tlv_len(NDN_TLV_RPC_NONNEGINT, len);
	goto done;
    case NDN_TLV_RPC_BIN:
	len = ndn_tlv_len(NDN_TLV_RPC_BIN, ds->u.binlen);
	goto done;
    case NDN_TLV_RPC_STR:
	len = ndn_tlv_len(NDN_TLV_RPC_NAME, ds->u.strlen);
	goto done;

    case NDN_TLV_RPC_APPLICATION:
	len = ccnl_rdr_getFlatLen(ds->u.fct);
	break;
    case NDN_TLV_RPC_LAMBDA:
	len = ccnl_rdr_getFlatLen(ds->u.lambdavar);
	break;
    case NDN_TLV_RPC_SEQUENCE:
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
    len = ndn_tlv_len(ds->type, len);
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
    int i;

    i = ccnl_rdr_serialize_fillTorL(typ, buf);
    i += ccnl_rdr_serialize_fillTorL(len, buf + i);

    return i;
}

static void ccnl_rdr_serialize_fill(struct rdr_ds_s *ds, unsigned char *at)
{
    int len;
    struct rdr_ds_s *goesfirst, *aux;
    unsigned int val;

    if (!ds)
	return;

    if (ds->type < NDN_TLV_RPC_APPLICATION) { // user defined code point
	*at = ds->type;
	return;
    }

    switch(ds->type) {
    case NDN_TLV_RPC_NAME:
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_NAME, ds->u.namelen, at);
	memcpy(at + len, ds->aux, ds->u.namelen);
	return;
    case NDN_TLV_RPC_BIN:
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_BIN, ds->u.binlen, at);
	memcpy(at + len, ds->aux, ds->u.binlen);
	return;
    case NDN_TLV_RPC_STR:
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_STR, ds->u.strlen, at);
	memcpy(at + len, ds->aux, ds->u.strlen);
	return;
    case NDN_TLV_RPC_NONNEGINT:
	len = ds->flatlen;
	ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_NONNEGINT, len-2, at);
	at += len - 1;
	val = ds->u.nonnegintval;
	for (len -= 2; len > 0; len--) {
	    *at = val & 0x0ff;
	    val = val >> 8;
	    at--;
	}
	return;

    case NDN_TLV_RPC_APPLICATION:
	len = ccnl_rdr_getFlatLen(ds->u.fct);
	goesfirst = ds->u.fct;
	break;
    case NDN_TLV_RPC_SEQUENCE:
	len = 0;
	goesfirst = NULL;
	break;
    case NDN_TLV_RPC_LAMBDA:
	len = ccnl_rdr_getFlatLen(ds->u.lambdavar);
	goesfirst = ds->u.lambdavar;
	break;
    default:
	fprintf(stderr, "serialize_fill() error\n");
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


// eof
