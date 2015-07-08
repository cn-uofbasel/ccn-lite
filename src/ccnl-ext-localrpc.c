/*
 * @f ccnl-ext-localrpc.c
 * @b CCN-lite - local RPC processing logic
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

#ifdef USE_SUITE_LOCALRPC

struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, char *nfnexpr, unsigned int *chunknum);

int ccnl_rdr_dump(int lev, struct rdr_ds_s *x)
{
    int i, t;
    char *n; //, tmp[20];

    t = ccnl_rdr_getType(x);
    if (t < LRPC_NOT_SERIALIZED)
        return t;
    /*
    if (t < LRPC_APPLICATION) {
        sprintf(tmp, "v%02x", t);
        n = tmp;
    } else
    */
    switch (t) {
    case LRPC_PT_REQUEST:
        n = "REQ"; break;
    case LRPC_PT_REPLY:
        n = "REP"; break;
    case LRPC_APPLICATION:
        n = "APP"; break;
    case LRPC_LAMBDA:
        n = "LBD"; break;
    case LRPC_SEQUENCE:
        n = "SEQ"; break;
    case LRPC_FLATNAME:
        n = "VAR"; break;
    case LRPC_NONNEGINT:
        n = "INT"; break;
    case LRPC_BIN:
        n = "BIN"; break;
    case LRPC_STR:
        n = "STR"; break;
    case LRPC_NONCE:
        n = "NCE"; break;
    default:
        n = "???"; break;
    }
    for (i = 0; i < lev; i++)
        fprintf(stderr, "  ");
    if (t == LRPC_FLATNAME)
        fprintf(stderr, "%s (0x%x, len=%d)\n", n, t, x->u.namelen);
    else
        fprintf(stderr, "%s (0x%x)\n", n, t);

    switch (t) {
    case LRPC_PT_REQUEST:
    case LRPC_PT_REPLY:
    case LRPC_APPLICATION:
        ccnl_rdr_dump(lev+1, x->u.fct);
        break;
    case LRPC_LAMBDA:
        ccnl_rdr_dump(lev+1, x->u.lambdavar);
        break;
    case LRPC_SEQUENCE:
        break;
    case LRPC_FLATNAME:
    case LRPC_NONNEGINT:
    case LRPC_BIN:
    case LRPC_STR:
    case LRPC_NONCE:
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

// ----------------------------------------------------------------------

int
ccnl_emit_RpcReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                    struct rdr_ds_s *nonce, int rc, char *reason,
                    struct rdr_ds_s *content)
{

    struct ccnl_buf_s *pkt;
    struct rdr_ds_s *seq, *element;
    int len, switchlen;
    unsigned char tmp[10];

    len = strlen(reason) + 50; // add some headroom
    if (content)
        len += ccnl_rdr_getFlatLen(content);
    pkt = ccnl_buf_new(NULL, len);
    if (!pkt)
        return 0;

    // we build a sequence, and later change the type in the flattened bytes
    seq = ccnl_rdr_mkSeq();
    element = ccnl_rdr_mkNonce((char*)nonce->aux, nonce->u.binlen);
    ccnl_rdr_seqAppend(seq, element);
    element = ccnl_rdr_mkNonNegInt(rc);
    ccnl_rdr_seqAppend(seq, element);
    element = ccnl_rdr_mkStr(reason);
    ccnl_rdr_seqAppend(seq, element);
    if (content) {
        ccnl_rdr_seqAppend(seq, content);
    }

    len = sizeof(tmp);
    switchlen = ccnl_switch_prependCoding(CCNL_ENC_LOCALRPC, &len, tmp);
    if (switchlen > 0)
        memcpy(pkt->data, tmp+len, switchlen);
    else // this should not happen
        switchlen = 0;

    len = ccnl_rdr_serialize(seq, pkt->data + switchlen,
                             pkt->datalen - switchlen);
    ccnl_rdr_free(seq);
    if (len < 0) {
        ccnl_free(pkt);
        return 0;
    }
//    fprintf(stderr, "%d bytes to return face=%p\n", len, from);

    *(pkt->data + switchlen) = LRPC_PT_REPLY;
    pkt->datalen = switchlen + len;
    ccnl_face_enqueue(relay, from, pkt);

    return 0;
}

// ----------------------------------------------------------------------


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

int
rpc_syslog(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
           struct rdr_ds_s *nonce, struct rpc_exec_s *exec,
           struct rdr_ds_s *param)
{
    DEBUGMSG(DEBUG, "rpc_syslog\n");

    if (ccnl_rdr_getType(param) == LRPC_STR) {
        char *cp = ccnl_malloc(param->u.strlen + 1);
        memcpy(cp, param->aux, param->u.strlen);
        cp[param->u.strlen] = '\0';
        DEBUGMSG(DEBUG, "rpc_syslog: \"%s\"\n", cp);
        ccnl_free(cp);
        ccnl_emit_RpcReturn(relay, from, nonce, 200, "ok", NULL);
    } else {
        DEBUGMSG(DEBUG, "rpc_syslog: unknown param type\n");
        ccnl_emit_RpcReturn(relay, from, nonce,
                            415, "rpc_syslog: unknown param type", NULL);
    }
    return 0;
}

int
rpc_cacheAdd(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
             struct rdr_ds_s *nonce, struct rpc_exec_s *exec,
             struct rdr_ds_s *param)
{
    int len, cnt = 0;

    DEBUGMSG(DEBUG, "rpc_cacheAdd\n");

    if (!ccnl_is_local_addr(&from->peer)) {
        ccnl_emit_RpcReturn(relay, from, nonce,
                            403, "invalid access: non-local address", NULL);
        return 0;
    }

    while (param) {
        if (ccnl_rdr_getType(param) != LRPC_BIN) {
            ccnl_emit_RpcReturn(relay, from, nonce,
                                415, "invalid or no data", NULL);
            return 0;
        }
        /*
        ucp = (unsigned char*) param->aux;
        */
        len = param->u.binlen;

        // not implemented yet ...
        // len = param->u.binlen;
        DEBUGMSG(INFO, "not implemented: rpc_cacheAdd (%d bytes)\n", len);
        // first convert aux bits to ccnl_content_s ...
        // then call ccnl_content_add2cache();
        cnt++;
        param = param->nextinseq;
    }
    ccnl_emit_RpcReturn(relay, from, nonce,
                        415, "rpc_cacheAdd: not implemented yet", NULL);
    return 0;
}

int
rpc_cacheRemove(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                struct rdr_ds_s *nonce, struct rpc_exec_s *exec,
                struct rdr_ds_s *param)
{
    int cnt = 0;

    DEBUGMSG(DEBUG, "rpc_cacheRemove\n");

    if (!ccnl_is_local_addr(&from->peer)) {
        ccnl_emit_RpcReturn(relay, from, nonce,
                            403, "invalid access: non-local address", NULL);
        return 0;
    }

    while (param) {
        struct ccnl_content_s *c = relay->contents;
        char *p;
        struct ccnl_prefix_s *prefix;

        if (ccnl_rdr_getType(param) != LRPC_FLATNAME) {
            ccnl_emit_RpcReturn(relay, from, nonce,
                                415, "rpc_cacheRemove: invalid name", NULL);
            return 0;
        }
        p = ccnl_malloc(param->u.strlen + 1);
        memcpy(p, param->aux, param->u.strlen);
        p[param->u.strlen] = '\0';
        prefix = ccnl_URItoPrefix(p, CCNL_SUITE_DEFAULT, NULL, NULL);

        while (c) {
            if (!ccnl_prefix_cmp(c->pkt->pfx, NULL, prefix, CMP_EXACT)) {
                struct ccnl_content_s *tmp = c->next;
                ccnl_content_remove(relay, c);
                DEBUGMSG(DEBUG, "content %s removed\n",
                         ccnl_prefix_to_path(prefix));
                cnt++;
                c = tmp;
            } else
                c = c->next;
        }
        ccnl_free(p);
        free_prefix(prefix);

        param = param->nextinseq;
    }
    {
        char *p = ccnl_malloc(100);
        sprintf(p, "rpc_cacheRemove: removed %d entries\n", cnt);
        ccnl_emit_RpcReturn(relay, from, nonce, 415, p, NULL);
        ccnl_free(p);
    }

    return 0;
}

int
rpc_forward(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
            struct rdr_ds_s *nonce, struct rpc_exec_s *exec,
            struct rdr_ds_s *param)
{
    int encoding, len;
    char *cp;
    unsigned char *ucp;

    DEBUGMSG(DEBUG, "rpc_forward\n");

    if (ccnl_rdr_getType(param) != LRPC_FLATNAME) {
        ccnl_emit_RpcReturn(relay, from, nonce,
                            415, "rpc_forward: expected encoding name", NULL);
        return 0;
    }

    cp = ccnl_malloc(param->u.namelen + 1);
    memcpy(cp, param->aux, param->u.namelen);
    cp[param->u.namelen] = '\0';
#ifdef CCNL_SUITE_CCNB
    if (!strcmp(cp, "/rpc/const/encoding/ccnb"))
        encoding = CCNL_SUITE_CCNB;
    else
#endif
#ifdef CCNL_SUITE_CCNTLV
    if (!strcmp(cp, "/rpc/const/encoding/ccnx2014"))
        encoding = CCNL_SUITE_CCNTLV;
    else
#endif
#ifdef CCNL_SUITE_IOTTLV
    if (!strcmp(cp, "/rpc/const/encoding/iot2014"))
        encoding = CCNL_SUITE_IOTTLV;
    else
#endif
#ifdef CCNL_SUITE_NDNTLV
    if (!strcmp(cp,      "/rpc/const/encoding/ndn2013"))
        encoding = CCNL_SUITE_NDNTLV;
    else
#endif
        encoding = -1;
    ccnl_free(cp);

    if (encoding < 0) {
        ccnl_emit_RpcReturn(relay, from, nonce,
                            415, "rpc_forward: no such encoding", NULL);
        return 0;
    }
    while (param->nextinseq) {
        param = param->nextinseq;
        if (ccnl_rdr_getType(param) != LRPC_BIN) {
            ccnl_emit_RpcReturn(relay, from, nonce,
                                415, "rpc_forward: invalid or no data", NULL);
            return 0;
        }

        ucp = (unsigned char*) param->aux;
        len = param->u.binlen;
        switch(encoding) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
            ccnl_ccnb_forwarder(relay, from, &ucp, &len);
            break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
            ccnl_ccntlv_forwarder(relay, from, &ucp, &len);
            break;
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV:
            ccnl_iottlv_forwarder(relay, from, &ucp, &len);
            break;
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
            ccnl_ndntlv_forwarder(relay, from, &ucp, &len);
            break;
#endif
        default:
            break;
        }
    }
    return 0;
}

int
rpc_lookup(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
           struct rdr_ds_s *nonce, struct rpc_exec_s *exec,
           struct rdr_ds_s *param)
{
    DEBUGMSG(DEBUG, "rpc_lookup\n");

    if (ccnl_rdr_getType(param) == LRPC_FLATNAME) {
        char *cp = ccnl_malloc(param->u.namelen + 1);
        struct rdr_ds_s *val = 0;
        memcpy(cp, param->aux, param->u.namelen);
        cp[param->u.namelen] = '\0';
        if (!strcmp(cp, "/rpc/config/compileString")) {
          val = ccnl_rdr_mkStr((char*)compile_string);
        } else if (!strcmp(cp, "/rpc/config/localTime")) {
            time_t t = time(NULL);
            char *p = ctime(&t);
            p[strlen(p) - 1] = '\0';
            val = ccnl_rdr_mkStr(p);
        }
        ccnl_free(cp);
        if (val)
            ccnl_emit_RpcReturn(relay, from, nonce, 200, "ok", val);
        else
            ccnl_emit_RpcReturn(relay, from, nonce,
                            415, "rpc_lookup: no such variable", NULL);
    } else {
        ccnl_emit_RpcReturn(relay, from, nonce,
                            415, "rpc_lookup: not a variable name", NULL);
    }
    return 0;
}


struct x_s {
    char *name;
    rpcBuiltinFct *fct;
} builtin[] = {
    {"/rpc/builtin/cache/add",    rpc_cacheAdd},
    {"/rpc/builtin/cache/remove", rpc_cacheRemove},
    {"/rpc/builtin/forward",      rpc_forward},
    {"/rpc/builtin/lookup",       rpc_lookup},
    {"/rpc/builtin/syslog",       rpc_syslog},
    {NULL, NULL}
};

rpcBuiltinFct*
rpc_getBuiltinFct(struct rdr_ds_s *var)
{
    struct x_s *x = builtin;

    if (var->type != LRPC_FLATNAME)
        return NULL;
    while (x->name) {
        if (strlen(x->name) == var->u.namelen &&
            !memcmp(x->name, var->aux, var->u.namelen))
            return x->fct;
        x++;
    }
    return NULL;
}

int
ccnl_localrpc_handleReply(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                           struct rdr_ds_s *aux)
{
    DEBUGMSG(DEBUG, "ccnl_localrpc_handleReply %d %d\n",
             ccnl_rdr_getType(aux), ccnl_rdr_getType(aux->nextinseq));

    return 0;
}

int
ccnl_localrpc_handleRequest(struct ccnl_relay_s *relay,
                            struct ccnl_face_s *from,
//                            struct rdr_ds_s *fexpr, struct rdr_ds_s *args)
                            struct rdr_ds_s *req)
{
    struct rdr_ds_s *nonce, *fexpr;
    struct rpc_exec_s *exec;
    int ftype, ntype, rc = -1;
    rpcBuiltinFct *fct;

    DEBUGMSG(DEBUG, "ccnl_localrpc_handleRequest face=%p\n", (void*) from);

    nonce = req->aux;
    if (!nonce)
        return -1;
    fexpr = nonce->nextinseq;
    ntype = ccnl_rdr_getType(nonce);
    ftype = ccnl_rdr_getType(fexpr);

    if (ntype != LRPC_NONCE || ftype != LRPC_APPLICATION) {
        DEBUGMSG(DEBUG, " malformed RPC request (%02x %02x)\n", ntype, ftype);
        ccnl_emit_RpcReturn(relay, from, nonce,
                            404, "malformed RPC request", NULL);
        goto done;
    }
    ftype = ccnl_rdr_getType(fexpr->u.fct);
    if (ftype != LRPC_FLATNAME) {
        DEBUGMSG(DEBUG, " (%02x) only constant fct names supported yet\n", ftype);
        ccnl_emit_RpcReturn(relay, from, nonce,
                            404, "only constant fct names supported yet", NULL);
        goto done;
    }
    fct = rpc_getBuiltinFct(fexpr->u.fct);
    if (!fct) {
        DEBUGMSG(DEBUG, "  unknown RPC builtin function (type=0x%02x)\n", ftype);
        ccnl_emit_RpcReturn(relay, from, nonce, 501, "unknown function", NULL);
        goto done;

    }
    exec = rpc_exec_new();
    rc = fct(relay, from, nonce, exec, fexpr->aux);
    rpc_exec_free(exec);
done:

    return rc;
}

int
ccnl_localrpc_exec(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   unsigned char **buf, int *buflen)
{
    struct rdr_ds_s *a; // , *fct;
    int rc = 0, type;

    DEBUGMSG(DEBUG, "ccnl_localrpc_exec: %d bytes from face=%p (id=%d.%d)\n",
             *buflen, (void*)from, relay->id, from ? from->faceid : -1);

    while (rc == 0 && *buflen > 0) {
        if (**buf && **buf != LRPC_PT_REQUEST && **buf != LRPC_PT_REPLY) {
            DEBUGMSG(DEBUG, "  not an RPC packet\n");
            return -1;
        }
        a = ccnl_rdr_unserialize(*buf, *buflen);
        if (!a) {
            DEBUGMSG(DEBUG, "  unserialization error\n");
            return -1;
        }

        //        ccnl_rdr_dump(0, a);

        type = ccnl_rdr_getType(a);
        //        fprintf(stderr, "type=%d\n", type);
        if (type == LRPC_PT_REQUEST)
            rc = ccnl_localrpc_handleRequest(relay, from, a);
        else if (type == LRPC_PT_REPLY)
            rc = ccnl_localrpc_handleReply(relay, from, a);
        else {
            DEBUGMSG(DEBUG, "  unserialization error %d\n", type);
            return -1;
        }


        /*
        fct = a->u.fct;
        if (ccnl_rdr_getType(fct) == LRPC_NONNEGINT) // RPC return msg
        else
        */

        if (rc < 0) {
//          DEBUGMSG(WARNING, "  error processing RPC msg\n");
//          return rc;
        }
        ccnl_rdr_free(a);
        *buf += a->flatlen;
        *buflen -= a->flatlen;
    }

    return 0;
}

#endif //USE_SUITE_LOCALRPC

// eof
