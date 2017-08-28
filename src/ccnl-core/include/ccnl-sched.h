/*
 * @f ccnl-sched.h
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

#ifndef CCNL_SCHED_H
#define CCNL_SCHED_H

#ifndef CCNL_LINUXKERNEL
#include <sys/time.h>
#endif

struct ccnl_relay_s;

struct ccnl_sched_s {
    char mode; // 0=dummy, 1=pktrate
    void (*rts)(struct ccnl_sched_s* s, int cnt, int len, void *aux1, void *aux2);
    // private:
    void (*cts)(void *aux1, void *aux2);
    struct ccnl_relay_s *ccnl;
    void *aux1, *aux2;
    int cnt;
#ifdef USE_CHEMFLOW
    struct cf_rnet *rn;
    struct cf_queue *q;
#else
    void *pendingTimer;
    struct timeval nextTX;
    // simple packet rate limiter:
    int ipi; // inter_packet_interval, minimum time between send() in usec
#endif
};

int 
ccnl_sched_init(void);

void 
ccnl_sched_cleanup(void);

struct ccnl_sched_s*
ccnl_sched_dummy_new(void (cts)(void *aux1, void *aux2),struct ccnl_relay_s *ccnl);

struct ccnl_sched_s*
ccnl_sched_pktrate_new(void (cts)(void *aux1, void *aux2),
        struct ccnl_relay_s *ccnl, int inter_packet_interval);

void
ccnl_sched_destroy(struct ccnl_sched_s *s);

void
ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len,
        void *aux1, void *aux2);

void
ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);

void
ccnl_sched_RX_ok(struct ccnl_relay_s *ccnl, int ifndx, int cnt);

void
ccnl_sched_RX_loss(struct ccnl_relay_s *ccnl, int ifndx, int cnt);

struct ccnl_sched_s*
ccnl_sched_packetratelimiter_new(int inter_packet_interval,
                                 void (*cts)(void *aux1, void *aux2),
                                 void *aux1, void *aux2);

#endif // EOF
