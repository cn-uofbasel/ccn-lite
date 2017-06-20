/*
 * @f ccnl-ext-localrpc.h
 * @b CCN-lite - local RPC processing logic
 *
 * Copyright (C) 2014-2018, Christian Tschudin, University of Basel
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
 * 2017-20-06 created
 */

#ifndef CCNL_LOCALRPC_H
#define CCNL_LOCALRPC_H

#include "../ccnl-core/ccnl-relay.h"
#include "../ccnl-core/ccnl-face.h"

int
ccnl_localrpc_exec(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   unsigned char **buf, int *buflen);


#ifdef CCNL_ARDUINO
const char compile_string[] PROGMEM = ""
#else
const char *compile_string = ""
#endif

#ifdef USE_CCNxDIGEST
        "CCNxDIGEST, "
#endif
#ifdef USE_DEBUG
        "DEBUG, "
#endif
#ifdef USE_DEBUG_MALLOC
        "DEBUG_MALLOC, "
#endif
#ifdef USE_ECHO
        "ECHO, "
#endif
#ifdef USE_LINKLAYER
        "ETHERNET, "
#endif
#ifdef USE_WPAN
        "WPAN, "
#endif
#ifdef USE_FRAG
        "FRAG, "
#endif
#ifdef USE_HMAC256
        "HMAC256, "
#endif
#ifdef USE_HTTP_STATUS
        "HTTP_STATUS, "
#endif
#ifdef USE_KITE
        "KITE, "
#endif
#ifdef USE_LOGGING
        "LOGGING, "
#endif
#ifdef USE_MGMT
        "MGMT, "
#endif
#ifdef USE_NACK
        "NACK, "
#endif
#ifdef USE_NFN
        "NFN, "
#endif
#ifdef USE_NFN_MONITOR
        "NFN_MONITOR, "
#endif
#ifdef USE_NFN_NSTRANS
        "NFN_NSTRANS, "
#endif
#ifdef USE_SCHEDULER
        "SCHEDULER, "
#endif
#ifdef USE_SIGNATURES
        "SIGNATURES, "
#endif
#ifdef USE_SUITE_CCNB
        "SUITE_CCNB, "
#endif
#ifdef USE_SUITE_CCNTLV
        "SUITE_CCNTLV, "
#endif
#ifdef USE_SUITE_CISTLV
        "SUITE_CISTLV, "
#endif
#ifdef USE_SUITE_IOTTLV
        "SUITE_IOTTLV, "
#endif
#ifdef USE_SUITE_LOCALRPC
        "SUITE_LOCALRPC, "
#endif
#ifdef USE_SUITE_NDNTLV
        "SUITE_NDNTLV, "
#endif
#ifdef USE_UNIXSOCKET
        "UNIXSOCKET, "
#endif
        ;


#endif