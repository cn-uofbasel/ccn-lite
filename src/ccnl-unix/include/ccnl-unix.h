/*
 * @f ccnl-unix.h
 * @b CCN lite, core CCNx protocol logic
 *
 * Copyright (C) 2011-18 University of Basel
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

#ifndef CCNL_UNIX_H
#define CCNL_UNIX_H

#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#include <netinet/in.h>
#include "ccnl-sockunion.h"

#include "ccnl-relay.h"
#include "ccnl-if.h"
#include "ccnl-buf.h"

#ifdef USE_LINKLAYER
#if !(defined(__FreeBSD__) || defined(__APPLE__))
int
ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype);
#endif
#endif

#ifdef USE_WPAN
int
ccnl_open_wpandev(char *devname, struct sockaddr_ieee802154 *swpan);
#endif

#ifdef USE_UNIXSOCKET
int
ccnl_open_unixpath(char *path, struct sockaddr_un *ux);
#endif

#ifdef USE_IPV4
int
ccnl_open_udpdev(int port, struct sockaddr_in *si);
#endif

#ifdef USE_IPV6
int
ccnl_open_udp6dev(int port, struct sockaddr_in6 *sin);
#endif

#ifdef USE_LINKLAYER
int
ccnl_eth_sendto(int sock, unsigned char *dst, unsigned char *src,
                unsigned char *data, int datalen);
#endif

#ifdef USE_WPAN
int
ccnl_wpan_sendto(int sock, unsigned char *data, int datalen,
                 struct sockaddr_ieee802154 *dst);
#endif

#ifdef USE_SCHEDULER
struct ccnl_sched_s*
ccnl_relay_defaultFaceScheduler(struct ccnl_relay_s *ccnl,
                                void(*cb)(void*,void*));
struct ccnl_sched_s*
ccnl_relay_defaultInterfaceScheduler(struct ccnl_relay_s *ccnl,
                                     void(*cb)(void*,void*));
#endif // USE_SCHEDULER

void 
ccnl_ageing(void *relay, void *aux);

#if defined(USE_IPV4) || defined(USE_IPV6)
void
ccnl_relay_udp(struct ccnl_relay_s *relay, int port, int af, int suite);
#endif

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf);

void
ccnl_relay_config(struct ccnl_relay_s *relay, char *ethdev, char *wpandev,
                  int udpport1, int udpport2,
		  int udp6port1, int udp6port2, int httpport,
                  char *uxpath, int suite, int max_cache_entries,
                  char *crypto_face_path);

int
ccnl_io_loop(struct ccnl_relay_s *ccnl);

void
ccnl_populate_cache(struct ccnl_relay_s *ccnl, char *path);

#endif // CCNL_UNIX_H
