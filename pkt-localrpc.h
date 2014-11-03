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

// #define NDN_TLV_RPC_USERDEFINEDNAME  0x00..0x7f

#ifndef PKT_LOCAL_RPC_H
#define PKT_LOCAL_RPC_H

#define NDN_TLV_RPC_APPLICATION         0x80
#define NDN_TLV_RPC_LAMBDA              0x81

// data marshalling
#define NDN_TLV_RPC_SEQUENCE            0x82
#define NDN_TLV_RPC_NAME                0x83
#define NDN_TLV_RPC_NONNEGINT           0x84
#define NDN_TLV_RPC_BIN                 0x85
#define NDN_TLV_RPC_STR                 0x86


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


#define NDN_TLV_RPC_SERIALIZED          -1

// prototypes for fwd-localrpc.c:
int ccnl_localrpc_RX_rpcreturn();
int ccnl_localrpc_RX_rpc();


#endif //PKT_LOCAL_RPC_H
// eof
