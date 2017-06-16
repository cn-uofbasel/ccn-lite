/*
 * @f ccnl-sockunion.h
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
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
 * 2017-06-16 created
 */

#include "ccnl-defs.h"
#include <netinet/in.h>

typedef union {
    struct sockaddr sa;
#ifdef USE_IPV4
    struct sockaddr_in ip4;
#endif
#ifdef USE_IPV6
    struct sockaddr_in6 ip6;
#endif
#ifdef USE_LINKLAYER
    struct sockaddr_ll linklayer;
#endif
#ifdef USE_WPAN
    struct sockaddr_ieee802154 wpan;
#endif
#ifdef USE_UNIXSOCKET
    struct sockaddr_un ux;
#endif
} sockunion;

int
ccnl_is_local_addr(sockunion *su);

char*
ccnl_addr2ascii(sockunion *su);