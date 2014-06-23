/*
 * @f fwd-localrpc.c
 * @b CCN lite, local RPC processing
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
 * 2014-05-11 created
 */

#include "pkt-ndntlv.h"
#include "pkt-localrpc.h"
#include "pkt-localrpc-enc.c"
#include "pkt-localrpc-dec.c"

int ccnl_rdr_dump(int lev, struct rdr_ds_s *x)
{
    int i, t;
    char *n, tmp[20];

    t = ccnl_rdr_getType(x);
    if (t < NDN_TLV_RPC_SERIALIZED)
	return t;
    if (t < NDN_TLV_RPC_APPLICATION) {
	sprintf(tmp, "v%02x", t);
	n = tmp;
    } else switch (t) {
    case NDN_TLV_RPC_APPLICATION:
	n = "APP"; break;
    case NDN_TLV_RPC_LAMBDA:
	n = "LBD"; break;
    case NDN_TLV_RPC_SEQUENCE:
	n = "SEQ"; break;
    case NDN_TLV_RPC_VAR:
	n = "VAR"; break;
    case NDN_TLV_RPC_NONNEGINT:
	n = "INT"; break;
    case NDN_TLV_RPC_BIN:
	n = "BIN"; break;
    case NDN_TLV_RPC_STR:
	n = "STR"; break;
    default:
	n = "???"; break;
    }
    for (i = 0; i < lev; i++)
	fprintf(stderr, "  ");
    if (t == NDN_TLV_RPC_VAR)
	fprintf(stderr, "%s (0x%x, len=%d)\n", n, t, x->u.varlen);
    else
	fprintf(stderr, "%s (0x%x)\n", n, t);

    switch (t) {
    case NDN_TLV_RPC_APPLICATION:
	ccnl_rdr_dump(lev+1, x->u.fct);
	break;
    case NDN_TLV_RPC_LAMBDA:
	ccnl_rdr_dump(lev+1, x->u.lambdavar);
	break;
    case NDN_TLV_RPC_SEQUENCE:
	break;
    case NDN_TLV_RPC_VAR:
    case NDN_TLV_RPC_NONNEGINT:
    case NDN_TLV_RPC_BIN:
    case NDN_TLV_RPC_STR:
    default:
	return 0;
    }
    x = x->aux;
    while (x) {
	ccnl_rdr_dump(lev+1, x);
	x = x->nextinseq;
    }
    return 0;
}

int
ccnl_emit_RpcReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		    int rc, char *reason, struct rdr_ds_s *content)
{

    struct ccnl_buf_s *pkt;
    struct rdr_ds_s *seq, *element;
    int len;

    len = strlen(reason) + 50;
    if (content)
	len += ccnl_rdr_getFlatLen(content);
    pkt = ccnl_buf_new(NULL, len);
    if (!pkt)
	return 0;

    // we build a sequence, and later change the type in the flattened bytes
    seq = ccnl_rdr_mkSeq();
    element = ccnl_rdr_mkNonNegInt(rc);
    ccnl_rdr_seqAppend(seq, element);
    element = ccnl_rdr_mkStr(reason);
    ccnl_rdr_seqAppend(seq, element);
    if (content) {
	ccnl_rdr_seqAppend(seq, content);
    }
    len = ccnl_rdr_serialize(seq, pkt->data, pkt->datalen);
    ccnl_rdr_free(seq);
    if (len < 0) {
	ccnl_free(pkt);
	return 0;
    }
//    fprintf(stderr, "%d bytes to return face=%p\n", len, from);

    *(pkt->data) = NDN_TLV_RPC_APPLICATION;
    pkt->datalen = len;
    ccnl_face_enqueue(relay, from, pkt);

    return 0;
}

// ----------------------------------------------------------------------

struct rpc_exec_s { // execution context
    struct rdr_ds_s *ostack; // operands
};

struct rpc_exec_s* rpc_exec_new(void)
{
    return ccnl_calloc(1, sizeof(struct rpc_exec_s));
}

void rpc_exec_free(struct rpc_exec_s *exec)
{
    if (!exec) return;
    if (exec->ostack)
	ccnl_rdr_free(exec->ostack);
    ccnl_free(exec);
}

// ----------------------------------------------------------------------

int rpc_syslog(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
	       struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    DEBUGMSG(11, "rpc_syslog\n");
    if (ccnl_rdr_getType(param) == NDN_TLV_RPC_STR) {
	char *cp = ccnl_malloc(param->u.strlen + 1);
	memcpy(cp, param->aux, param->u.strlen);
	cp[param->u.strlen] = '\0';
	DEBUGMSG(1, "rpc_syslog: \"%s\"\n", cp);
	ccnl_free(cp);
	ccnl_emit_RpcReturn(relay, from, 200, "ok", NULL);
    } else {
	DEBUGMSG(12, "rpc_syslog: unknown param type\n");
	ccnl_emit_RpcReturn(relay, from,
			    415, "rpc_syslog: unknown param type", NULL);
    }
    return 0;
}

int rpc_forward(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    int encoding, len;
    char *cp;

    DEBUGMSG(11, "rpc_forward\n");

    if (ccnl_rdr_getType(param) != NDN_TLV_RPC_VAR) {
	ccnl_emit_RpcReturn(relay, from,
			    415, "rpc_forward: expected encoding name", NULL);
	return 0;
    }
	
    cp = ccnl_malloc(param->u.varlen + 1);
    struct rdr_ds_s *val = 0;
    memcpy(cp, param->aux, param->u.varlen);
    cp[param->u.varlen] = '\0';
    if (!strcmp(cp,      "/rpc/const/encoding/ndn2013"))
	encoding = CCNL_SUITE_NDNTLV;
#ifdef CCNL_SUITE_CCNB
    else if (!strcmp(cp, "/rpc/const/encoding/ccnb"))
	encoding = CCNL_SUITE_CCNB;
#endif
    else
	encoding = -1;
    ccnl_free(cp);

    if (encoding < 0) {
	ccnl_emit_RpcReturn(relay, from,
			    415, "rpc_forward: no such encoding", NULL);
	return 0;
    }
    while (param->nextinseq) {
	param = param->nextinseq;
	if (ccnl_rdr_getType(param) != NDN_TLV_RPC_BIN) {
	    ccnl_emit_RpcReturn(relay, from,
				415, "rpc_forward: invalid or no data", NULL);
	    return 0;
	}

	cp = (char*) param->aux;
	len = param->u.binlen;
	switch(encoding) {
#ifdef USE_SUITE_CCNB
	case CCNL_SUITE_CCNB:
	    ccnl_RX_ccnb(relay, from, &cp, &len);
	    break;
#endif
#ifdef USE_SUITE_NDNTLV
	case CCNL_SUITE_NDNTLV:
	    ccnl_RX_ndntlv(relay, from, &cp, &len);
	    break;
#endif
	default:
	    break;
	}
    }
    return 0;
}

int rpc_lookup(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
	       struct rpc_exec_s *exec, struct rdr_ds_s *param)
{
    DEBUGMSG(11, "rpc_lookup\n");

    if (ccnl_rdr_getType(param) == NDN_TLV_RPC_VAR) {
	char *cp = ccnl_malloc(param->u.varlen + 1);
	struct rdr_ds_s *val = 0;
	memcpy(cp, param->aux, param->u.varlen);
	cp[param->u.varlen] = '\0';
	if (!strcmp(cp, "/rpc/config/compileString")) {
	    val = ccnl_rdr_mkStr(compile_string());
	} else if (!strcmp(cp, "/rpc/config/localTime")) {
	    time_t t = time(NULL);
	    char *p = ctime(&t);
	    p[strlen(p) - 1] = '\0';
	    val = ccnl_rdr_mkStr(p);
	}
	ccnl_free(cp);
	if (val)
	    ccnl_emit_RpcReturn(relay, from, 200, "ok", val);
	else
	    ccnl_emit_RpcReturn(relay, from,
			    415, "rpc_lookup: no such variable", NULL);
    } else {
//	DEBUGMSG(1, "rpc_lookup: unknown param type\n");
	ccnl_emit_RpcReturn(relay, from,
			    415, "rpc_lookup: not a variable name", NULL);
    }
    return 0;
}

typedef int(builtinFct)(struct ccnl_relay_s *, struct ccnl_face_s *,
			struct rpc_exec_s *, struct rdr_ds_s *);

struct x_s {
    char *name;
    builtinFct *fct;
} builtin[] = {
    {"/rpc/builtin/lookup",  rpc_lookup},
    {"/rpc/builtin/forward", rpc_forward},
    {"/rpc/builtin/syslog",  rpc_syslog},
    {NULL, NULL}
};

builtinFct* rpc_getBuiltinFct(struct rdr_ds_s *var)
{
    struct x_s *x = builtin;

    if (var->type != NDN_TLV_RPC_VAR)
	return NULL;
    while (x->name) {
	if (strlen(x->name) == var->u.varlen &&
	    !memcmp(x->name, var->aux, var->u.varlen))
	    return x->fct;
	x++;
    }
    return NULL;
}

int
ccnl_localrpc_handleReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
			   struct rdr_ds_s *rc, struct rdr_ds_s *aux)
{
    DEBUGMSG(10, "ccnl_RX_handleReturn %d %d\n",
	     ccnl_rdr_getType(rc), ccnl_rdr_getType(aux));
    
    return 0;
}

int
ccnl_localrpc_handleApplication(struct ccnl_relay_s *relay,
				struct ccnl_face_s *from,
				struct rdr_ds_s *fexpr, struct rdr_ds_s *args)
{
    int ftype, rc;
    builtinFct *fct;
    struct rpc_exec_s *exec;

    DEBUGMSG(10, "ccnl_RX_handleApplication face=%p\n", from);

    ftype = ccnl_rdr_getType(fexpr);
    if (ftype != NDN_TLV_RPC_VAR) {
	DEBUGMSG(11, " (%02x) only constant fct names supported yet\n", ftype);
	ccnl_emit_RpcReturn(relay, from,
			    404, "only constant fct names supported yet", NULL);
	return -1;
    }
    fct = rpc_getBuiltinFct(fexpr);
    if (!fct) {
	DEBUGMSG(11, "  unknown builtin function (type=0x%02x)\n", ftype);
	ccnl_emit_RpcReturn(relay, from, 501, "unknown function", NULL);
	return -1;
    }
    exec = rpc_exec_new();
    rc = fct(relay, from, exec, args);
    rpc_exec_free(exec);
    return rc;
}


int
ccnl_RX_localrpc(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
		 unsigned char **buf, int *buflen)
{
    struct rdr_ds_s *a, *fct;
    int rc = 0, type;

    DEBUGMSG(6, "ccnl_RX_localrpc: %d bytes from face=%p (id=%d.%d)\n",
	     *buflen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc == 0 && *buflen > 0) {
	if (**buf != NDN_TLV_RPC_APPLICATION) {
	    DEBUGMSG(15, "  not an RPC packet\n");
	    return -1;
	}
	a = ccnl_rdr_unserialize(*buf, *buflen);
	if (!a) {
	    DEBUGMSG(15, "  unserialization error\n");
	    return -1;
	}

//	ccnl_rdr_dump(0, a);

	type = ccnl_rdr_getType(a);
	if (type < 0 || type != NDN_TLV_RPC_APPLICATION) {
	    DEBUGMSG(15, "  unserialization error %d\n", type);
	    return -1;
	}
	fct = a->u.fct;
	if (ccnl_rdr_getType(fct) == NDN_TLV_RPC_NONNEGINT) // RPC return msg
	    rc = ccnl_localrpc_handleReturn(relay, from, fct, a->aux);
	else
	    rc = ccnl_localrpc_handleApplication(relay, from, fct, a->aux);
	if (rc < 0) {
//	    DEBUGMSG(15, "  error processing RPC msg\n");
//	    return rc;
	}
	ccnl_rdr_free(a);
	*buf += a->flatlen;
	*buflen -= a->flatlen;
    }

    return 0;
}

// eof
