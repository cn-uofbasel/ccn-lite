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

// #define NDN_TLV_RPC_VARIABLE           0x00..0x7f

#define NDN_TLV_RPC_APPLICATION		0x80
#define NDN_TLV_RPC_LAMBDA		0x81

// data marshalling
#define NDN_TLV_RPC_SEQUENCE		0x82
#define NDN_TLV_RPC_ASCII		0x83
#define NDN_TLV_RPC_INT			0x84
#define NDN_TLC_RPC_BIN			0x85

// prototypes for fwd-localrpc.c:
int ccnl_localrpc_RX_rpcreturn();
int ccnl_localrpc_RX_rpc();

// eof
