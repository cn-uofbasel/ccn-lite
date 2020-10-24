/*
 * @f ccnl-pkt-localrpc.h
 * @b CCN lite - header file for local RPC support
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

#ifndef CCNL_PKT_LOCALRPC_H
#define CCNL_PKT_LOCALRPC_H

#ifndef CCNL_LINUXKERNEL
#include "ccnl-relay.h"
#include "ccnl-face.h"
#else
#include "../../ccnl-core/include/ccnl-relay.h"
#include "../../ccnl-core/include/ccnl-face.h"
#endif

struct rpc_exec_s { // execution context
    struct rdr_ds_s *ostack; // operands
};

typedef int(rpcBuiltinFct)(struct ccnl_relay_s *, struct ccnl_face_s *,
                           struct rdr_ds_s *, struct rpc_exec_s *,
                           struct rdr_ds_s *);


#define LRPC_PT_REQUEST          0x81
#define LRPC_PT_REPLY            0x82

// #define LRPC_USERDEFINEDNAME  0x10..0x7f

#define LRPC_APPLICATION         0x2

#define LRPC_FLATNAME            0x0
#define LRPC_NONNEGINT           0x1
#define LRPC_STR                 0x3

#define LRPC_SEQUENCE            0x4
#define LRPC_LAMBDA              0x5
#define LRPC_BIN                 0x6
#define LRPC_NONCE               0x7


struct rdr_ds_s { // RPC Data Representation (RDR) data structure
    int32_t type;
    size_t flatlen; // The value of flatlen is only significant if flat is non-NULL!
    uint8_t *flat;
    struct rdr_ds_s *nextinseq;
    union {
        struct rdr_ds_s *fct;
        struct rdr_ds_s *lambdavar;
        uint64_t nonnegintval;
        size_t namelen;
        size_t binlen;
        size_t strlen;
    } u;
    struct rdr_ds_s *aux;
};

/* use of struct rdr_ds_s:

  Req->u.fct (the RPC proper)
     ->aux (nonce)

  Rep->u.nonnegintval (nonce)
     ->aux (seq)
              ->u.nonnegintval (retcode)
              ->u.nextinseq (more info, string) ...

  App->u.fct (function)
     ->aux (1st param)
              -> u.nextinseq (2nd param) ...

  Lambda->u.lambdavar
     ->aux (1st body part)
              -> u.nextinseq (2nd body part) ...

  Seq->aux (first element)
              -> u.nextinseq (2nd element)
                                -> u.nextinseq (3rd element) ...

  Var->aux (char*)
  BIN->aux (char*)
  STR->aux (char*)

*/


#define LRPC_NOT_SERIALIZED          -1



int 
ccnl_rdr_getType(struct rdr_ds_s *ds);

void 
ccnl_rdr_free(struct rdr_ds_s *x);

struct rdr_ds_s* 
ccnl_rdr_mkSeq(void);

struct rdr_ds_s* 
ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el);

struct rdr_ds_s*
ccnl_rdr_mkNonNegInt(uint64_t val);

int8_t
ccnl_rdr_serialize(struct rdr_ds_s *ds, uint8_t *buf, size_t buflen, size_t *res);

struct rdr_ds_s*
ccnl_rdr_unserialize(uint8_t *buf, size_t buflen);

int8_t
ccnl_rdr_getFlatLen(struct rdr_ds_s *ds, size_t *flatlen);

struct rdr_ds_s*
ccnl_rdr_mkNonce(uint8_t *data, size_t len);

struct rdr_ds_s* 
ccnl_rdr_mkStr(char *s);

int8_t
ccnl_lrpc_dehead(uint8_t **buf, size_t *len,
                 uint64_t *typ, size_t *vallen);

#endif // CCNL_PKT_LOCALRPC_H
// eof
