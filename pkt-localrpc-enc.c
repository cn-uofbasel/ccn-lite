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
    if (!x)
	return;
    switch (x->type) {
    case NDN_TLV_RPC_APPLICATION:
	ccnl_rdr_free(x->u.appexpr);
	ccnl_rdr_free(x->aux);
	break;
    case NDN_TLV_RPC_LAMBDA:
	ccnl_rdr_free(x->u.lambdavar);
	ccnl_rdr_free(x->aux);
	break;
    case NDN_TLV_RPC_SEQUENCE:
	while (x) {
	    struct rdr_ds_s *y = x;
	    ccnl_rdr_free(x->aux);
	    x = x->u.nextinseq;
	    free(y);
	}
	break;
    case NDN_TLV_RPC_ASCII:
    case NDN_TLV_RPC_NONNEGINT:
    case NDN_TLV_RPC_BIN:
    default:
	break;
    }
    if (x)
      free(x);
}

struct rdr_ds_s* ccnl_rdr_mkApp(struct rdr_ds_s *expr, struct rdr_ds_s *arg)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_APPLICATION;
    ds->flatlen = -1;
    ds->u.appexpr = expr;
    ds->aux = arg;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkSeq(void)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_SEQUENCE;
    ds->flatlen = -1;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el)
{
    struct rdr_ds_s *p;

    if (!seq || seq->type != NDN_TLV_RPC_SEQUENCE)
	return NULL;

    if (!seq->aux) {
	seq->aux = el;
	return seq;
    }

    for (p = seq; p->u.nextinseq; p = p->u.nextinseq) {}
    p->u.nextinseq = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));
    if (!p->u.nextinseq) {
	// should free old data structures of seq
	return 0;
    }
    p = p->u.nextinseq;
    p->type = NDN_TLV_RPC_SEQUENCE;
    p->flatlen = -1;
    p->aux = el;

    seq->flatlen = -1;
    return seq;
}

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

struct rdr_ds_s* ccnl_rdr_mkNonNegInt(unsigned int val)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_NONNEGINT;
    ds->flatlen = -1;
    ds->u.nonnegintval = val;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkCodePoint(unsigned char cp)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = cp;
    ds->flatlen = 1;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkAscii(char *s)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_ASCII;
    ds->flatlen = -1;
    ds->u.asciilen = s ? strlen(s) : 0;
    ds->aux = (struct rdr_ds_s*) s;

    return ds;
}

struct rdr_ds_s* ccnl_rdr_mkBin(char *data, int len)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds) return 0;
    ds->type = NDN_TLV_RPC_BIN;
    ds->flatlen = -1;
    ds->u.binlen = len;
    ds->aux = (struct rdr_ds_s*) data;

    return ds;
}

inline int ndn_tlv_fieldlen(unsigned long val)
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

    if (!ds)
	return -1;
    if (ds->flatlen >= 0)
	return ds->flatlen;
    switch(ds->type) {
    case NDN_TLV_RPC_ASCII:
	len = ndn_tlv_len(NDN_TLV_RPC_ASCII, ds->u.asciilen);
	break;
    case NDN_TLV_RPC_NONNEGINT:
	val = ds->u.nonnegintval;
	len = 0;
	do {
	    val = val >> 8;
	    len++;
	} while (val);
	len = ndn_tlv_len(NDN_TLV_RPC_NONNEGINT, len);
	break;
    case NDN_TLV_RPC_BIN:
	len = ndn_tlv_len(NDN_TLV_RPC_BIN, ds->u.binlen);
	break;
    case NDN_TLV_RPC_APPLICATION:
	len = ccnl_rdr_getFlatLen(ds->u.appexpr) + ccnl_rdr_getFlatLen(ds->aux);
	len = ndn_tlv_len(NDN_TLV_RPC_APPLICATION, len);
	break;
    case NDN_TLV_RPC_LAMBDA:
	len = ccnl_rdr_getFlatLen(ds->u.lambdavar) + ccnl_rdr_getFlatLen(ds->aux);
	len = ndn_tlv_len(NDN_TLV_RPC_LAMBDA, len);
	break;
    case NDN_TLV_RPC_SEQUENCE:
	len = ds->aux ? ccnl_rdr_getFlatLen(ds->aux) : 0;
	while (ds->u.nextinseq) {
	    ds = ds->u.nextinseq;
	    len += ccnl_rdr_getFlatLen(ds->aux);
	}
	len = ndn_tlv_len(NDN_TLV_RPC_SEQUENCE, len);
	break;
    default:
	len = -1;
	break;
    }
    ds->flatlen = len;
    return len;
}

static int ccnl_rdr_serialize_fillTorL(int val, unsigned char *buf)
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
    for (i = 0, buf++; i < len; i++, buf++) {
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
    int len, len2;
    struct rdr_ds_s *ds2;
    unsigned int val;

    if (!ds)
	return;

    if (ds->type < NDN_TLV_RPC_APPLICATION) { // user defined code point
	*at = ds->type;
	return;
    }

    switch(ds->type) {
    case NDN_TLV_RPC_ASCII:
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_ASCII, ds->u.asciilen, at);
	memcpy(at + len, ds->aux, ds->u.asciilen);
	break;
    case NDN_TLV_RPC_BIN:
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_BIN, ds->u.binlen, at);
	memcpy(at + len, ds->aux, ds->u.binlen);
	break;
    case NDN_TLV_RPC_APPLICATION:
	len2 = ccnl_rdr_getFlatLen(ds->u.appexpr);
	len = len2 + ccnl_rdr_getFlatLen(ds->aux);
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_APPLICATION, len, at);
	ccnl_rdr_serialize_fill(ds->u.appexpr, at + len);
	ccnl_rdr_serialize_fill(ds->aux, at + len + len2);
	break;
    case NDN_TLV_RPC_SEQUENCE:
	len = ds->aux ? ccnl_rdr_getFlatLen(ds->aux) : 0;
	ds2 = ds;
	while (ds2->u.nextinseq) {
	    ds2 = ds2->u.nextinseq;
	    len += ccnl_rdr_getFlatLen(ds2->aux);
	}
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_SEQUENCE, len, at);
	if (ds->aux) {
	    ccnl_rdr_serialize_fill(ds->aux, at + len);
	    len += ccnl_rdr_getFlatLen(ds->aux);
	    while (ds->u.nextinseq) {
		ds = ds->u.nextinseq;
		ccnl_rdr_serialize_fill(ds->aux, at + len);
		len += ccnl_rdr_getFlatLen(ds->aux);
	    }
	}
	break;
    case NDN_TLV_RPC_LAMBDA:
	len2 = ccnl_rdr_getFlatLen(ds->u.lambdavar);
	len = len2 + ccnl_rdr_getFlatLen(ds->aux);
	len = ccnl_rdr_serialize_fillTandL(NDN_TLV_RPC_LAMBDA, len, at);
	ccnl_rdr_serialize_fill(ds->u.lambdavar, at + len);
	ccnl_rdr_serialize_fill(ds->aux, at + len + len2);
	break;
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
	break;
    default:
	fprintf(stderr, "serialize_fill() error\n");
	break;
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
