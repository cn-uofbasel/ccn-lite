/*
 * @f ccnl-if.h
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

#ifndef CCNL_IF_H
#define CCNL_IF_H

#if defined(CCNL_RIOT)
#include "sched.h"
#endif

#include "ccnl-sched.h"
#include "ccnl-face.h"



struct ccnl_txrequest_s {
    struct ccnl_buf_s *buf;
    sockunion dst;
    void (*txdone)(void*, int, int);
    struct ccnl_face_s* txdone_face;
};

struct ccnl_if_s { // interface for packet IO
    sockunion addr;
#ifdef CCNL_LINUXKERNEL
    struct socket *sock;
    struct workqueue_struct *wq;
    void (*old_data_ready)(struct sock *);
    struct net_device *netdev;
    struct packet_type ccnl_packet;
#elif defined(CCNL_ARDUINO)
    EthernetUDP *sock;
#elif defined(CCNL_RIOT)
    kernel_pid_t if_pid;
    int sock;
    uint8_t hwaddr[CCNL_MAX_ADDRESS_LEN];
    uint16_t addr_len;
#else
    int sock;
#endif
    int reflect; // whether to reflect I packets on this interface
    int fwdalli; // whether to forward all I packets rcvd on this interface
    uint32_t mtu;

    size_t qlen;  // number of pending sends
    size_t qfront; // index of next packet to send
    struct ccnl_txrequest_s queue[CCNL_MAX_IF_QLEN];
    struct ccnl_sched_s *sched;

#ifdef USE_STATS
    uint32_t rx_cnt, tx_cnt;
#endif
};

void
ccnl_interface_cleanup(struct ccnl_if_s *i);

#if !defined(CCNL_LINUXKERNEL) && !defined(CCNL_ANDROID)
int
ccnl_close_socket(int s);
#endif

#endif // EOF
