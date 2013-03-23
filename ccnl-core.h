/*
 * @f ccnl-core.h
 * @b CCN lite (CCNL), core header file (internal data structures)
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
 * 2011-04-09 created
 * 2013-03-19 updated (ms): modified struct ccnl_relay_s for 'aux' field
 */

#define EXACT_MATCH 1
#define LOOSE_MATCH 0

#define CCNL_FACE_FLAGS_STATIC	1
#define CCNL_FACE_FLAGS_REFLECT	2
#define CCNL_FACE_FLAGS_SERVED	4
#define CCNL_FACE_FLAGS_FWDALLI	8 // forward all interests, also known ones

#define CCNL_ENCAPS_NONE		0
#define CCNL_ENCAPS_SEQUENCED2012	1

#define CCNL_CONTENT_FLAGS_STATIC  0x01
#define CCNL_CONTENT_FLAGS_STALE   0x02

// ----------------------------------------------------------------------

typedef union {
    struct sockaddr sa;
#ifdef USE_ETHERNET
    struct sockaddr_ll eth;
#endif
    struct sockaddr_in ip4;
    struct sockaddr_un ux;
} sockunion;

struct ccnl_txrequest_s {
    struct ccnl_buf_s *buf;
    sockunion dst;
//    void (*txdone)(struct ccnl_sched_s*, int, int);
    void (*txdone)(void*,int,int);
    struct ccnl_face_s* txdone_face;
};

struct ccnl_if_s { // interface for packet IO
    sockunion addr;
#ifdef CCNL_KERNEL
    struct socket *sock;
    struct workqueue_struct *wq;
    void (*old_data_ready)(struct sock *, int);
    struct net_device *netdev;
    struct packet_type ccnl_packet;
#else
    int sock;
#endif
    int reflect; // whether to reflect I packets on this interface
    int fwdalli; // whether to forward all I packets rcvd on this interface
    int mtu;

    int qlen;  // number of pending sends
    int qfront; // index of next packet to send
    struct ccnl_txrequest_s queue[CCNL_MAX_IF_QLEN];
/*
    struct ccnl_buf_s* queue[CCNL_MAX_IF_QLEN];
    sockunion dest[CCNL_MAX_IF_QLEN];
*/

    struct ccnl_sched_s *sched;
//    struct ccnl_face_s **sendface; // which face wants to send
//    int facecnt, faceoffs;
//    struct ccnl_face_s *sendface;
    void (*tx_done)(struct ccnl_sched_s*, int, int);
};

struct ccnl_relay_s {
    struct ccnl_face_s *faces;
    struct ccnl_forward_s *fib;
    struct ccnl_interest_s *pit;
    struct ccnl_content_s *contents; //, *contentsend;
    struct ccnl_buf_s *nonces;
    int contentcnt;		// number of cached items
    int max_cache_entries;	// -1: unlimited
    struct ccnl_if_s ifs[CCNL_MAX_INTERFACES];
    int ifcount;		// number of active interfaces
    char halt_flag;
    struct ccnl_sched_s* (*defaultFaceScheduler)(struct ccnl_relay_s*,
						 void(*cts_done)(void*,void*));

    // (ms) Replaced the commented part with the two fields below. The rationale
    // is that
    // (a) 'stats' can be there even if not used (in fact they re used in most
    // containers, and they could be optionaly activated from the mgmt interface)
    // (b) 'aux' can be casted to ccnl_client_s for use witht the simu container,
    // but may also be used in a number of other contexts (state struct for named
    // func, control block for omnet, etc)
/*
#ifdef CCNL_SIMULATION
    struct ccnl_stats_s *stats;
    struct ccnl_client_s *client; // simulation mode: client state
#endif
*/
    struct ccnl_stats_s *stats;
    void *aux;
};

struct ccnl_buf_s {
    struct ccnl_buf_s *next;
    unsigned int datalen;
    unsigned char data[1];
};

struct ccnl_prefix_s {
    unsigned char **comp;
    int *complen;
    int compcnt;
    unsigned char *path; // memory for name component copies
};

struct ccnl_encaps_s {
    int protocol; // (0=plain CCNx)
    int mtu;
    sockunion dest;
    struct ccnl_buf_s *bigpkt;
    unsigned int sendoffs;
    // transport state, if present:
    int ifndx;

    struct ccnl_buf_s *defrag;
    unsigned char flagbytes;

    unsigned int sendseq;
    unsigned int losscount;
    unsigned int recvseq;
    unsigned char sendseqbytes;
    unsigned char losscountbytes;
    unsigned char recvseqbytes;
};

struct ccnl_face_s {
    struct ccnl_face_s *next, *prev;
    int faceid;
    int ifndx;
    sockunion peer;
    int flags;
    int last_used; // updated when we receive a packet
    struct ccnl_buf_s *outq, *outqend; // queue of packets to send
    struct ccnl_encaps_s *encaps;  // which special datagram armoring
    struct ccnl_sched_s *sched;
};

struct ccnl_forward_s {
    struct ccnl_forward_s *next;
    struct ccnl_prefix_s *prefix;
    struct ccnl_face_s *face;
};

struct ccnl_interest_s {
    struct ccnl_interest_s *next, *prev;
    struct ccnl_face_s *from;
    struct ccnl_pendint_s *pending; // linked list of faces wanting that content
    struct ccnl_prefix_s *prefix;
    struct ccnl_buf_s *nonce;
    struct ccnl_buf_s *ppkd;	   // publisher public key digest
    struct ccnl_buf_s *data;	   // full datagram
    int last_used;
    int retries;
};

struct ccnl_pendint_s { // pending interest
    struct ccnl_pendint_s *next; // , *prev;
    struct ccnl_face_s *face;
    int last_used;
};

struct ccnl_content_s {
    struct ccnl_content_s *next, *prev;
    struct ccnl_prefix_s *prefix;
    struct ccnl_buf_s *ppkd; // publisher public key digest
    struct ccnl_buf_s *data; // full datagram
    int flags;
    unsigned char *content;
    int contentlen;
    // NON-CONFORM: "The [ContentSTore] MUST also implement the Staleness Bit."
    // >> CCNL: currently no stale bit, old content is fully removed <<
    int last_used;
    int served_cnt;
};

// ----------------------------------------------------------------------
// referenced by ccnl-relay.c:

#ifdef USE_ENCAPS

int ccnl_is_encaps(struct ccnl_buf_s *buf);
struct ccnl_buf_s* ccnl_encaps_handle_fragment(struct ccnl_relay_s *r,
					       struct ccnl_face_s *f, unsigned char *data, int datalen);
void ccnl_encaps_destroy(struct ccnl_encaps_s *e);

// referenced by ccnl_relay_sched.c:
struct ccnl_buf_s* ccnl_encaps_fragment(struct ccnl_relay_s *ccnl,
					struct ccnl_encaps_s *encaps,
					struct ccnl_buf_s *buf);
#endif

#ifdef USE_SCHEDULER
struct ccnl_sched_s* ccnl_sched_new(void (cts)(void *aux1, void *aux2),
				    int inter_packet_gap);
void ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len,
		    void *aux1, void *aux2);
void ccnl_sched_destroy(struct ccnl_sched_s *s);
#endif

int ccnl_is_local_addr(sockunion *su);

// ----------------------------------------------------------------------
// macros for double linked lists (these double linked lists are not rings)

#define DBL_LINKED_LIST_ADD(l,e) \
  do { if ((l)) (l)->prev = (e); \
       (e)->next = (l); \
       (l) = (e); \
  } while(0)

#define DBL_LINKED_LIST_REMOVE(l,e) \
  do { if ((l) == (e)) (l) = (e)->next; \
       if ((e)->prev) (e)->prev->next = (e)->next; \
       if ((e)->next) (e)->next->prev = (e)->prev; \
  } while(0)

// #define LINKED_LIST_DROPNFREE_HEAD(l)		\
//  do { void *p = (l)->next;				\
//       ccnl_free(l);					\
//       (l) = p;					\
//  } while(0)


// eof
