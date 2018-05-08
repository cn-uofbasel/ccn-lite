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

#ifndef CCNL_SOCKET_UNION_H
#define CCNL_SOCKET_UNION_H

//#define _DEFAULT_SOURCE

#include "ccnl-defs.h"
#ifndef CCNL_LINUXKERNEL
#include <netinet/in.h>
#include <net/ethernet.h>

#ifndef CCNL_RIOT
#include <sys/un.h>
#else
#include "net/packet.h"
#include <net/packet.h>
#endif


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#if defined(__FreeBSD__) || defined(__APPLE__)
#  include <sys/types.h>
#  include <sys/socket.h>
#  ifdef USE_LINKLAYER
#pragma message "ethernet support in FreeBSD and MacOS is work in progress \
 and not fully implemented!"
#  endif
#elif defined(__linux__)
#  include <endian.h>
#ifndef CCNL_RIOT
#  include <linux/if_ether.h>  // ETH_ALEN
#  include <linux/if_packet.h> // sockaddr_ll
#endif //CCNL_RIOT
#endif
#endif //!CCNL_LINUXKERNEL

#ifdef USE_WPAN
/* TODO: remove when af_ieee802154.h is in linux mainline */
#define IEEE802154_ADDR_LEN 8

typedef enum {
    IEEE802154_ADDR_NONE = 0x0,
    IEEE802154_ADDR_SHORT = 0x2,
    IEEE802154_ADDR_LONG = 0x3,
} wpan_addr_type_t;

struct ieee802154_addr_sa {
    int addr_type;
    uint16_t pan_id;
    union {
        uint8_t hwaddr[IEEE802154_ADDR_LEN];
        uint16_t short_addr;
    } addr;
};

struct sockaddr_ieee802154 {
    sa_family_t family;
    struct ieee802154_addr_sa addr;
};
#endif

typedef union {
    struct sockaddr sa;
#ifdef USE_IPV4
    struct sockaddr_in ip4;
#endif
#ifdef USE_IPV6
    struct sockaddr_in6 ip6;
#endif
#ifdef USE_LINKLAYER
#if (!defined(__FreeBSD__) && !defined(__APPLE__)) || \
    (defined(CCNL_RIOT) && defined(__FreeBSD__)) ||  \
    (defined(CCNL_RIOT) && defined(__APPLE__)) 
    struct sockaddr_ll linklayer;
#endif
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

/**
 * @brief Returns a string representation of a given socket
 *
 * This function checks if the given socket type is supported by CCN-lite and
 * returns its string representation. In no circumstance should one pass this
 * function without a check to the return type to a function expecting a
 * string, e.g. "printf("from: %s\n", ccnl_addr2ascii(some_type));"!
 *
 * @param[in] su The socket type to represent as string
 *
 * @return NULL if \ref su was NULL
 * @return NULL if the given socket type is not supported
 * @return A string representation of \ref su
 */
char* ccnl_addr2ascii(sockunion *su);

int
ccnl_addr_cmp(sockunion *s1, sockunion *s2);

char*
ll2ascii(unsigned char *addr, size_t len);

char
_half_byte_to_char(uint8_t half_byte);

//char 
//*inet_ntoa(struct in_addr in);


#endif
