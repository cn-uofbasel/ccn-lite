/*
 * @f pkt-localrpc-enc.h
 * @b CCN lite - local RPC Data Representation (RDR) decoding routines
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

struct rdr_ds_s* ccnl_rdr_unserialize(unsigned char *buf, int buflen)
{
    struct rdr_ds_s *ds = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));

    if (!ds)
	return NULL;
    ds->flat = buf;
    ds->flatlen = buflen;
    ds->type = NDN_TLV_RPC_SERIALIZED;

    return ds;
}

int ccnl_rdr_getType(struct rdr_ds_s *ds)
{
    unsigned char *buf;
    int typ, vallen, len;
    struct rdr_ds_s *a, *b, *end;

    if (!ds)
	return -2;
    if (ds->type != NDN_TLV_RPC_SERIALIZED)
	return ds->type;

    buf = ds->flat;
    len = ds->flatlen;
    if (*buf < NDN_TLV_RPC_APPLICATION) { // user defined code point
	ds->type = *buf;
	ds->flatlen = 1;
	return ds->type;
    }
    if (ccnl_ndntlv_dehead(&buf, &len, &typ, &vallen))
	return -3;

    if (vallen > len)
	return -4;
    switch (typ) {
    case NDN_TLV_RPC_NONNEGINT:
	ds->u.nonnegintval = ccnl_ndntlv_nonNegInt(buf, len);
	ds->type = NDN_TLV_RPC_NONNEGINT;
	break;
    case NDN_TLV_RPC_ASCII:
	ds->u.asciilen = vallen;
	ds->aux = (struct rdr_ds_s*) buf;
	ds->type = NDN_TLV_RPC_ASCII;
	ds->flatlen = (buf - ds->flat) + vallen;
	break;
    case NDN_TLV_RPC_BIN:
	ds->u.binlen = vallen;
	ds->aux = (struct rdr_ds_s*) buf;
	ds->type = NDN_TLV_RPC_BIN;
	ds->flatlen = (buf - ds->flat) + vallen;
	break;
    case NDN_TLV_RPC_APPLICATION:
	ds->flatlen = buf - ds->flat;
	a = ccnl_rdr_unserialize(buf, vallen);
	if (ccnl_rdr_getType(a) < 0)
	    return -5;
	buf += a->flatlen;
	vallen -= a->flatlen;
	b = ccnl_rdr_unserialize(buf, vallen);
	if (ccnl_rdr_getType(b) < 0)
	    return -6;
	ds->flatlen += a->flatlen + b->flatlen;
	ds->u.appexpr = a;
	ds->aux = b;
	ds->type = NDN_TLV_RPC_APPLICATION;
	break;
    case NDN_TLV_RPC_LAMBDA:
	ds->flatlen = buf - ds->flat;
	a = ccnl_rdr_unserialize(buf, vallen);
	if (ccnl_rdr_getType(a) < 0)
	    return -7;
	buf += a->flatlen;
	vallen -= a->flatlen;
	b = ccnl_rdr_unserialize(buf, vallen);
	if (ccnl_rdr_getType(b) < 0)
	    return -8;
	ds->flatlen += a->flatlen + b->flatlen;
	ds->u.lambdavar = a;
	ds->aux = b;
	ds->type = NDN_TLV_RPC_LAMBDA;
	break;
    case NDN_TLV_RPC_SEQUENCE:
	ds->type = NDN_TLV_RPC_SEQUENCE;
	ds->flatlen = buf - ds->flat;
	if (vallen > 0) {
	    a = ccnl_rdr_unserialize(buf, vallen);
	    if (ccnl_rdr_getType(a) < 0)
		return -9; // better do a break?
	    ds->flatlen += a->flatlen;
	    ds->aux = a;
	    buf += a->flatlen;
	    vallen -= a->flatlen;
	}
	end = ds;
	while (vallen > 0) {
	    a = ccnl_rdr_unserialize(buf, vallen);
	    if (ccnl_rdr_getType(a) < 0)
		return -10; // better do a break?
	    ds->flatlen += a->flatlen;
	    end->u.nextinseq = (struct rdr_ds_s*) calloc(1, sizeof(struct rdr_ds_s));
	    if (!end->u.nextinseq)
		return -11;
	    end = end->u.nextinseq;
	    end->type = NDN_TLV_RPC_SEQUENCE;
	    end->aux = a;
	    buf += a->flatlen;
	    vallen -= a->flatlen;
	}
	break;
    }

    return ds->type;
}

// eof
