/*
 * @f ccnl-core.h
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-15, Christian Tschudin, University of Basel
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

#define CCNL_FACE_FLAGS_STATIC  1
#define CCNL_FACE_FLAGS_REFLECT 2
#define CCNL_FACE_FLAGS_SERVED  4
#define CCNL_FACE_FLAGS_FWDALLI 8 // forward all interests, also known ones

#define CCNL_FRAG_NONE          0
#define CCNL_FRAG_SEQUENCED2012 1
#define CCNL_FRAG_CCNx2013      2
#define CCNL_FRAG_SEQUENCED2015 3
#define CCNL_FRAG_BEGINEND2015  4

#ifdef CCNL_RIOT
#define CCNL_LLADDR_STR_MAX_LEN    (3 * 8)
#else
/* unless a platform supports a link layer with longer addresses than Ethernet,
 * 6 is enough */
#define CCNL_LLADDR_STR_MAX_LEN    (3 * 6)
#endif
// ----------------------------------------------------------------------

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
    void (*old_data_ready)(struct sock *);
    struct net_device *netdev;
    struct packet_type ccnl_packet;
#elif defined(CCNL_ARDUINO)
    EthernetUDP *sock;
#elif defined(CCNL_RIOT)
    kernel_pid_t if_pid;
    int sock;
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

#ifdef USE_STATS
    uint32_t rx_cnt, tx_cnt;
#endif
};

struct ccnl_relay_s {
#ifndef CCNL_ARDUINO
    time_t startup_time;
#endif
    int id;
    struct ccnl_face_s *faces;
    struct ccnl_forward_s *fib;
    struct ccnl_interest_s *pit;
    struct ccnl_content_s *contents; //, *contentsend;
    struct ccnl_buf_s *nonces;
    int contentcnt;             // number of cached items
    int max_cache_entries;      // -1: unlimited
    struct ccnl_if_s ifs[CCNL_MAX_INTERFACES];
    int ifcount;                // number of active interfaces
    char halt_flag;
    struct ccnl_sched_s* (*defaultFaceScheduler)(struct ccnl_relay_s*,
                                                 void(*cts_done)(void*,void*));
    struct ccnl_sched_s* (*defaultInterfaceScheduler)(struct ccnl_relay_s*,
                                                 void(*cts_done)(void*,void*));
#ifdef USE_HTTP_STATUS
    struct ccnl_http_s *http;
#endif
    void *aux;

#ifdef USE_NFN
    struct ccnl_krivine_s *km;
#endif

  /*
    struct ccnl_face_s *crypto_face;
    struct ccnl_pendcrypt_s *pendcrypt;
    char *crypto_path;
  */
};

struct ccnl_buf_s {
    struct ccnl_buf_s *next;
    ssize_t datalen;
    unsigned char data[1];
};

struct ccnl_prefix_s {
    unsigned char **comp;
    int *complen;
    int compcnt;
    char suite;
    unsigned char *nameptr; // binary name (for fast comparison)
    ssize_t namelen; // valid length of name memory
    unsigned char *bytes;   // memory for name component copies
    int *chunknum; // -1 to disable
#ifdef USE_NFN
    unsigned int nfnflags;
# define CCNL_PREFIX_NFN   0x01

# define CCNL_PREFIX_COMPU 0x04
    unsigned char *nfnexpr;
#endif
};

struct ccnl_frag_s {
    int protocol; // fragmentation protocol, 0=none
    int mtu;
    sockunion dest;
    struct ccnl_buf_s *bigpkt; // outgoing bytes
    unsigned int sendoffs;
    int outsuite; // suite of outgoing packet
    // transport state, if present:
    int ifndx;

    // int insuite; // suite of incoming packet series
    struct ccnl_buf_s *defrag; // incoming bytes

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

typedef void (*tapCallback)(struct ccnl_relay_s *, struct ccnl_face_s *,
                            struct ccnl_prefix_s *, struct ccnl_buf_s *);

struct ccnl_forward_s {
    struct ccnl_forward_s *next;
    struct ccnl_prefix_s *prefix;
    tapCallback tap;
    struct ccnl_face_s *face;
    char suite;
};

struct ccnl_interest_s {
    struct ccnl_interest_s *next, *prev;
    struct ccnl_pkt_s *pkt;
    struct ccnl_face_s *from;
    struct ccnl_pendint_s *pending; // linked list of faces wanting that content
    unsigned short flags;
#define CCNL_PIT_COREPROPAGATES    0x01
#define CCNL_PIT_TRACED            0x02
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
    struct ccnl_pkt_s *pkt;
    unsigned short flags;
#define CCNL_CONTENT_FLAGS_STATIC  0x01
#define CCNL_CONTENT_FLAGS_STALE   0x02
    // NON-CONFORM: "The [ContentSTore] MUST also implement the Staleness Bit."
    // >> CCNL: currently no stale bit, old content is fully removed <<
    int last_used;
    int served_cnt;
};

struct ccnl_pktdetail_ccnb_s {
    int minsuffix, maxsuffix, aok, scope;
    struct ccnl_buf_s *nonce;
    struct ccnl_buf_s *ppkd;        // publisher public key digest
};

struct ccnl_pktdetail_ccntlv_s {
    struct ccnl_buf_s *keyid;       // publisher keyID
};

struct ccnl_pktdetail_iottlv_s {
    int ttl;
};

struct ccnl_pktdetail_ndntlv_s {
    int minsuffix, maxsuffix, mbf, scope;
    struct ccnl_buf_s *nonce;      // nonce
    struct ccnl_buf_s *ppkl;       // publisher public key locator
};

// packet flags:  0000ebtt

#define CCNL_PKT_REQUEST    0x01 // "Interest"
#define CCNL_PKT_REPLY      0x02 // "Object", "Data"
#define CCNL_PKT_FRAGMENT   0x03 // "Fragment"
#define CCNL_PKT_FRAG_BEGIN 0x04 // see also CCNL_DATA_FRAG_FLAG_FIRST etc
#define CCNL_PKT_FRAG_END   0x08

struct ccnl_pkt_s {
    struct ccnl_buf_s *buf;        // the packet's bytes
    struct ccnl_prefix_s *pfx;     // prefix/name
    unsigned char *content;        // pointer into the data buffer
    int contlen;
    unsigned int type;   // suite-specific value (outermost type)
    union {
        int final_block_id;
        unsigned int seqno;
    } val;
    union {
        struct ccnl_pktdetail_ccnb_s   ccnb;
        struct ccnl_pktdetail_ccntlv_s ccntlv;
        struct ccnl_pktdetail_iottlv_s iottlv;
        struct ccnl_pktdetail_ndntlv_s ndntlv;
    } s;                           // suite specific packet details
#ifdef USE_HMAC256
    unsigned char *hmacStart;
    int hmacLen;
    unsigned char *hmacSignature;
#endif
    unsigned int flags;
    char suite;
};

typedef int (*dispatchFct)(struct ccnl_relay_s*, struct ccnl_face_s*,
                           unsigned char**, int*);
typedef int (*cMatchFct)(struct ccnl_pkt_s *p, struct ccnl_content_s *c);

struct ccnl_suite_s {
    dispatchFct RX;
    cMatchFct cMatch;
};

// ----------------------------------------------------------------------

struct ccnl_lambdaTerm_s {
    char *v;
    struct ccnl_lambdaTerm_s *m, *n;
    // if m is 0, we have a var  v
    // is both m and n are not 0, we have an application  (M N)
    // if n is 0, we have a lambda term  @v M
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

#endif /*CCNL_CORE*/
// eof
