/*
 * @f ccnl-frag.h
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

#ifndef CCNL_FRAG_H
#define CCNL_FRAG_H

#include "ccnl-sockunion.h"
#include "ccnl-face.h"
#include "ccnl-relay.h"

// returns >=0 if content consumed, buf and len pointers updated
typedef int (RX_datagram)(struct ccnl_relay_s*, struct ccnl_face_s*,
                          unsigned char**, int*);

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

struct serialFragPDU_s { // collect all fields of a numbered HBH fragment
    int contlen;
    unsigned char *content;
    unsigned int flags, ourseq, ourloss, yourseq, HAS;
    unsigned char flagwidth, ourseqwidth, ourlosswidth, yourseqwidth;
};

struct ccnl_frag_s*
ccnl_frag_new(int protocol, int mtu);

void
ccnl_frag_reset(struct ccnl_frag_s *e, struct ccnl_buf_s *buf,
                  int ifndx, sockunion *dst);

int
ccnl_frag_getfragcount(struct ccnl_frag_s *e, int origlen, int *totallen);

#ifdef OBSOLTE_BY_2015_06
#ifdef USE_SUITE_CCNB
struct ccnl_buf_s*
ccnl_frag_getnextSEQD2012(struct ccnl_frag_s *e, int *ifndx, sockunion *su);
struct ccnl_buf_s*
ccnl_frag_getnextCCNx2013(struct ccnl_frag_s *fr, int *ifndx, sockunion *su);
#endif // USE_SUITE_CCNB
struct ccnl_buf_s*
ccnl_frag_getnextSEQD2015(struct ccnl_frag_s *fr, int *ifndx, sockunion *su);
#endif // OBSOLTE_BY_2015_06

struct ccnl_buf_s*
ccnl_frag_getnextBE2015(struct ccnl_frag_s *fr, int *ifndx, sockunion *su);

struct ccnl_buf_s*
ccnl_frag_getnext(struct ccnl_frag_s *fr, int *ifndx, sockunion *su);

int
ccnl_frag_nomorefragments(struct ccnl_frag_s *e);

void
ccnl_frag_destroy(struct ccnl_frag_s *e);

void
serialFragPDU_init(struct serialFragPDU_s *s);

#ifdef OBSOLETE_BY_2015_06
// processes a SEQENCED fragment. Once fully assembled we call the callback
void
ccnl_frag_RX_serialfragment(RX_datagram callback,
                            struct ccnl_relay_s *relay,
                            struct ccnl_face_s *from,
                            struct serialFragPDU_s *s)

#ifdef USE_SUITE_CCNB

#define getNumField(var,len,flag,rem) \
        DEBUGMSG_EFRA(TRACE, "  parsing " rem "\n"); \
        if (ccnl_ccnb_unmkBinaryInt(data, datalen, &var, &len) != 0) \
            goto Bail; \
        s.HAS |= flag
#define HAS_FLAGS  0x01
#define HAS_OSEQ   0x02
#define HAS_OLOS   0x04
#define HAS_YSEQ   0x08

int
ccnl_frag_RX_frag2012(RX_datagram callback,
                        struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                        unsigned char **data, int *datalen);
int
ccnl_frag_RX_CCNx2013(RX_datagram callback,
                       struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                       unsigned char **data, int *datalen);


#endif USE_SUITE_CCNB

int
ccnl_frag_RX_Sequenced2015(RX_datagram callback, struct ccnl_relay_s *relay,
                           struct ccnl_face_s *from, int mtu,
                           unsigned int bits, unsigned int seqno,
                           unsigned char **data, int *datalen);

#endif // OBSOLTE_BY_2015_06

int
ccnl_frag_RX_BeginEnd2015(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from, int mtu,
                          unsigned int bits, unsigned int seqno,
                          unsigned char **data, int *datalen);

struct ccnl_buf_s*
ccnl_frag_getnext(struct ccnl_frag_s *fr, int *ifndx, sockunion *su);

#endif //CCNL_FRAG_H
