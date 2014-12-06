/*
 * @f pkt-localrpc.h
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

#ifndef PKT_LOCALRPC_H
#define PKT_LOCALRPC_H


struct rpc_exec_s { // execution context
    struct rdr_ds_s *ostack; // operands
};

typedef int(rpcBuiltinFct)(struct ccnl_relay_s *, struct ccnl_face_s *,
                           struct rpc_exec_s *, struct rdr_ds_s *);


// #define LRPC_USERDEFINEDNAME  0x10..0x7f

#define LRPC_APPLICATION         0x80
#define LRPC_LAMBDA              0x81

// data marshalling
#define LRPC_SEQUENCE            0x82
#define LRPC_BIN                 0x83

#define LRPC_NONNEGINT           0x00 // low values for IoT encoding
#define LRPC_FLATNAME            0x01
#define LRPC_STR                 0x02


struct rdr_ds_s { // RPC Data Representation (RPR) data structure
    int type;
    int flatlen;
    unsigned char *flat;
    struct rdr_ds_s *nextinseq;
    union {
        struct rdr_ds_s *fct;
        struct rdr_ds_s *lambdavar;
        unsigned int nonnegintval;
        int namelen;
        int binlen;
        int strlen;
    } u;
    struct rdr_ds_s *aux;
};

/* use of struct rdr_ds_s:

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

#endif //PKT_LOCALRPC_H
// eof
