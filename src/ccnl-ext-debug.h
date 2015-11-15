/*
 * @f ccnl-ext-debug.h
 * @b CCNL debugging support, dumping routines, memory tracking, stats
 *
 * Copyright (C) 2011-13, Christian Tschudin, University of Basel
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
 * 2014-10-30 created <christopher.scherb@unibas.ch
 */

#ifndef CCNL_EXT_DEBUG_H
#define CCNL_EXT_DEBUG_H

#ifdef USE_DEBUG
#ifdef USE_DEBUG_MALLOC

struct mhdr {
    struct mhdr *next;
    char *fname;
    int lineno, size;
#ifdef CCNL_ARDUINO
    double tstamp;
#else
    char *tstamp; // Linux kernel (no double), also used for CCNL_UNIX
#endif
} *mem;

#endif // USE_DEBUG_MALLOC

#ifndef CCNL_LINUXKERNEL
struct ccnl_stats_s {
    void* log_handler;
    FILE *ofp;
    int log_sent;
    int log_recv;
    int log_recv_old;
    int log_sent_old;
    int log_need_rt_seqn;
    int log_content_delivery_rate_per_s;
    double log_start_t;
    double log_cdr_t;
    //
    FILE *ofp_r, *ofp_s, *ofp_q;
    double log_recv_t_i;
    double log_recv_t_c;
    double log_sent_t_i;
    double log_sent_t_c;
};
#endif // CCNL_LINUXKERNEL

#endif // USE_DEBUG

// eof
#endif /* CCNL-EXT-DEBUG_H */
