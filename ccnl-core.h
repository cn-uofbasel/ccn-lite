/*
 * @f ccnl-core.h
 * @b CCN lite (CCNL), core header file (internal data structures)
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
 * 2011-04-09 created
 * 2013-03-19 updated (ms): modified struct ccnl_relay_s for 'aux' field
 */

#ifndef CCNL_CORE
#define CCNL_CORE

#define EXACT_MATCH 1
#define PREFIX_MATCH 0

#define CMP_EXACT   0 // used to compare interests among themselves
#define CMP_MATCH   1 // used to match interest and content
#define CMP_LONGEST 2 // used to lookup the FIB

#define CCNL_FACE_FLAGS_STATIC	1
#define CCNL_FACE_FLAGS_REFLECT	2
#define CCNL_FACE_FLAGS_SERVED	4
#define CCNL_FACE_FLAGS_FWDALLI	8 // forward all interests, also known ones

#define CCNL_FRAG_NONE		0
#define CCNL_FRAG_SEQUENCED2012	1
#define CCNL_FRAG_CCNx2013	2

#define CCNL_CONTENT_FLAGS_STATIC  0x01
#define CCNL_CONTENT_FLAGS_STALE   0x02

enum {
  CCNL_SUITE_CCNB,
  CCNL_SUITE_CCNTLV,
  CCNL_SUITE_NDNTLV
};

// ----------------------------------------------------------------------

typedef union {
    struct sockaddr sa;
#ifdef USE_ETHERNET
    struct sockaddr_ll eth;
#endif
    struct sockaddr_in ip4;
#ifdef USE_UNIXSOCKET
    struct sockaddr_un ux;
#endif
} sockunion;

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
    struct ccnl_sched_s *sched;
};

struct ccnl_relay_s {
    time_t startup_time;
    int id;
    char suite;
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
    struct ccnl_sched_s* (*defaultInterfaceScheduler)(struct ccnl_relay_s*,
						 void(*cts_done)(void*,void*));
    struct ccnl_http_s *http;
    struct ccnl_stats_s *stats;
    void *aux;
    
    struct ccnl_face_s *crypto_face;
    struct ccnl_pendcrypt_s *pendcrypt;
    char *crypto_path;
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

struct ccnl_frag_s {
    int protocol; // (0=plain CCNx)
    int mtu;
    sockunion dest;
    struct ccnl_buf_s *bigpkt;
    unsigned int sendoffs;
    // transport state, if present:
    int ifndx;

    struct ccnl_buf_s *defrag;

    unsigned int sendseq;
    unsigned int losscount;
    unsigned int recvseq;
    unsigned char flagwidth;
    unsigned char sendseqwidth;
    unsigned char losscountwidth;
    unsigned char recvseqwidth;
};

struct ccnl_face_s {
    struct ccnl_face_s *next, *prev;
    int faceid;
    int ifndx;
    sockunion peer;
    int flags;
    int last_used; // updated when we receive a packet
    struct ccnl_buf_s *outq, *outqend; // queue of packets to send
    struct ccnl_frag_s *frag;  // which special datagram armoring
    struct ccnl_sched_s *sched;
};

struct ccnl_forward_s {
    struct ccnl_forward_s *next;
    struct ccnl_prefix_s *prefix;
    struct ccnl_face_s *face;
};

struct ccnl_ccnb_id_s { // interest details
    int minsuffix, maxsuffix, aok;
    struct ccnl_buf_s *ppkd;       // publisher public key digest
};

struct ccnl_ccntlv_id_s { // interest details
    char dummy; // keep the compiler happy and silent
};

struct ccnl_ndntlv_id_s { // interest details
    int minsuffix, maxsuffix, mbf;
    struct ccnl_buf_s *ppkl;       // publisher public key locator
};

struct ccnl_interest_s {
    struct ccnl_buf_s *pkt; // full datagram
    struct ccnl_interest_s *next, *prev;
    struct ccnl_face_s *from;
    struct ccnl_pendint_s *pending; // linked list of faces wanting that content
    struct ccnl_prefix_s *prefix;
    int last_used;
    int retries;
    union {
	struct ccnl_ccnb_id_s ccnb;
	struct ccnl_ccntlv_id_s ccntlv;
	struct ccnl_ndntlv_id_s ndntlv;
    } details;
    char suite;
#ifdef CCNL_NFN
    int propagate;                 //set to 0 to not propagate this interest becauses it is in the NFN-engine
#endif
};

struct ccnl_pendint_s { // pending interest
    struct ccnl_pendint_s *next; // , *prev;
    struct ccnl_face_s *face;
    int last_used;
};

struct ccnl_ccnb_cd_s { // content details
    struct ccnl_buf_s *ppkd;       // publisher public key digest
};

struct ccnl_ccntlv_cd_s { // content details
    char dummy; // keep the compiler happy and silent
};

struct ccnl_ndntlv_cd_s { // content details
    struct ccnl_buf_s *ppkl;       // publisher public key locator
};

struct ccnl_content_s {
    struct ccnl_buf_s *pkt; // full datagram
    struct ccnl_content_s *next, *prev;
    struct ccnl_prefix_s *name;
    struct ccnl_buf_s *ppkd; // publisher public key digest
    int flags;
    unsigned char *content; // pointer into the data buffer
    int contentlen;
    // NON-CONFORM: "The [ContentSTore] MUST also implement the Staleness Bit."
    // >> CCNL: currently no stale bit, old content is fully removed <<
    int last_used;
    int served_cnt;
    union {
	struct ccnl_ccnb_cd_s ccnb;
	struct ccnl_ccntlv_cd_s ccntlv;
	struct ccnl_ndntlv_cd_s ndntlv;
    } details;
    char suite;
};

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

// ----------------------------------------------------------------------
// collect the USE_* macros in a string

const char*
compile_string(void)
{
    static const char *cp = ""
#ifdef USE_CCNxDIGEST
	"USE_CCNxDIGEST "
#endif
#ifdef USE_DEBUG
	"USE_DEBUG "
#endif
#ifdef USE_DEBUG_MALLOC
	"USE_DEBUG_MALLOC "
#endif
#ifdef USE_FRAG
	"USE_FRAG "
#endif
#ifdef USE_ETHERNET
	"USE_ETHERNET "
#endif
#ifdef USE_HTTP_STATUS
	"USE_HTTP_STATUS "
#endif
#ifdef USE_MGMT
	"USE_MGMT "
#endif
#ifdef USE_SCHEDULER
	"USE_SCHEDULER "
#endif
#ifdef USE_SUITE_CCNB
	"USE_SUITE_CCNB "
#endif
#ifdef USE_SUITE_CCNTLV
	"USE_SUITE_CCNTLV "
#endif
#ifdef USE_SUITE_NDNTLV
	"USE_SUITE_NDNTLV "
#endif
#ifdef USE_UNIXSOCKET
	"USE_UNIXSOCKET "
#endif
#ifdef CCNL_NFN
    "USE_NFN "
#endif
#ifdef CCNL_NFN_MONITOR
    "USE_NFN_MONITOR "
#endif
#ifdef CCNL_NACK
    "USE_NACK "
#endif
	;
  return cp;
}

#endif /*CCNL_CORE*/
// eof
