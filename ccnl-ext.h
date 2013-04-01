/*
 * @f ccnl-ext.h
 * @b header file for CCN lite extentions (forward declarations)
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 * 2013-03-30 created
 */

// ----------------------------------------------------------------------
// referenced by ccn-lite-relay.c:

#ifdef USE_ENCAPS

struct ccnl_encaps_s* ccnl_encaps_new(int protocol, int mtu);

void ccnl_encaps_start(struct ccnl_encaps_s *e, struct ccnl_buf_s *buf,
		       int ifndx, sockunion *su);

int ccnl_encaps_getfragcount(struct ccnl_encaps_s *e, int origlen,
			     int *totallen);

struct ccnl_buf_s* ccnl_encaps_getnextfragment(struct ccnl_encaps_s *e,
					       int *ifndx, sockunion *su);

int ccnl_is_encaps(struct ccnl_buf_s *buf);

struct ccnl_buf_s* ccnl_encaps_handle_fragment(struct ccnl_relay_s *r,
		struct ccnl_face_s *f, unsigned char *data, int datalen);

void ccnl_encaps_destroy(struct ccnl_encaps_s *e);

struct ccnl_buf_s* ccnl_encaps_fragment(struct ccnl_relay_s *ccnl,
					struct ccnl_encaps_s *encaps,
					struct ccnl_buf_s *buf);
#endif // USE_ENCAPS


#ifdef USE_SCHEDULER

struct ccnl_sched_s* ccnl_sched_new(void (cts)(void *aux1, void *aux2),
				    int inter_packet_gap);

void ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len,
		    void *aux1, void *aux2);

void ccnl_sched_destroy(struct ccnl_sched_s *s);

/*
int ccnl_scheduler_register(struct ccnl_scheduler_s *s,
			    struct ccnl_buf_s* (*getPacket)(void *aux1, void *aux2),
			    void *aux1, void *aux2);
  ;
int ccnl_scheduler_unregister(struct ccnl_scheduler_s *s);
int ccnl_scheduler_rts(struct ccnl_face_s *f, struct ccnl_scheduler_s *s, int pktcnt, int size);
void ccnl_scheduler_destroy(struct ccnl_scheduler_s *s);
*/

#else

/*
#define ccnl_scheduler_new()			NULL
#define ccnl_scheduler_register(S,CB,A1,A2)	do{}while(-1)
#define ccnl_scheduler_unregister(S)		do{}while(-1)
#define ccnl_scheduler_rts(S,C,L)		do{}while(-1)
#define ccnl_scheduler_destroy(S)		do{}while(-1)
*/

#endif

int ccnl_is_local_addr(sockunion *su);


void ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
		sockunion *dest, struct ccnl_buf_s *buf);

#ifdef USE_MGMT
int ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
#else
# define ccnl_mgmt(r,p,f)  0
#endif

void ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);


// eof
