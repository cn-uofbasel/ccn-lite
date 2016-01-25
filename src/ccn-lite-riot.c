/*
 * @f ccn-lite-riot.c
 * @b RIOT adaption layer
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
 * Copyright (C) 2015, Oliver Hahm, INRIA
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
 * 2015-10-26 created (based on ccn-lite-minimalrelay.c)
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* RIOT specific includes */
#include "kernel_types.h"
#include "random.h"
#include "timex.h"
#include "xtimer.h"
#include "net/gnrc/netreg.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/netapi.h"
#include "net/packet.h"
#include "ccn-lite-riot.h"

#include "ccnl-os-time.c"

#define free_2ptr_list(a,b)     ccnl_free(a), ccnl_free(b)
#define free_3ptr_list(a,b,c)   ccnl_free(a), ccnl_free(b), ccnl_free(c)
#define free_4ptr_list(a,b,c,d) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d);
#define free_5ptr_list(a,b,c,d,e) ccnl_free(a), ccnl_free(b), ccnl_free(c), ccnl_free(d), ccnl_free(e);

#define free_prefix(p)  do{ if(p) \
                free_4ptr_list(p->bytes,p->comp,p->complen,p); } while(0)
#define free_content(c) do{ /* free_prefix(c->name); */ free_packet(c->pkt); \
                        ccnl_free(c); } while(0)

#define local_producer(...)             0

//----------------------------------------------------------------------
static msg_t _msg_queue[CCNL_QUEUE_SIZE];

/* stack for the CCN-Lite eventloop */
static char _ccnl_stack[CCNL_STACK_SIZE];

static kernel_pid_t _ccnl_event_loop_pid = KERNEL_PID_UNDEF;

#include "ccnl-defs.h"
#include "ccnl-core.h"

void free_packet(struct ccnl_pkt_s *pkt);

struct ccnl_interest_s* ccnl_interest_remove(struct ccnl_relay_s *ccnl,
                     struct ccnl_interest_s *i);
int ccnl_pkt2suite(unsigned char *data, int len, int *skip);

char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr,
                    int ccntlv_skip, int escape_components, int call_slash);
#define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)

char* ccnl_addr2ascii(sockunion *su);
void ccnl_core_addToCleanup(struct ccnl_buf_s *buf);
const char* ccnl_suite2str(int suite);
bool ccnl_isSuite(int suite);

// ----------------------------------------------------------------------
struct ccnl_relay_s theRelay;
struct ccnl_face_s *loopback_face;
extern int debug_level;
void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf);

extern struct ccnl_buf_s* ccnl_buf_new(void *data, int len);
int ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

#include "ccnl-core.c"

gnrc_netreg_entry_t ccnl_riot_ne;

typedef int (*ccnl_mkInterestFunc)(struct ccnl_prefix_s*, int*, unsigned char*, int);
typedef int (*ccnl_isContentFunc)(unsigned char*, int);

extern ccnl_mkInterestFunc ccnl_suite2mkInterestFunc(int suite);
extern ccnl_isContentFunc ccnl_suite2isContentFunc(int suite);


// ----------------------------------------------------------------------
struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b = ccnl_malloc(sizeof(*b) + len);

    if (!b)
        return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
        memcpy(b->data, data, len);
    return b;
}

int
ccnl_open_netif(kernel_pid_t if_pid, gnrc_nettype_t netreg_type)
{
    assert(pid_is_valid(if_pid));
    if (!gnrc_netif_exist(if_pid)) {
        return -1;
    }

    /* get current interface from CCN-Lite's relay */
    struct ccnl_if_s *i;
    i = &theRelay.ifs[theRelay.ifcount];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->if_pid = if_pid;
    i->addr.sa.sa_family = AF_PACKET;

    gnrc_netapi_get(if_pid, NETOPT_MAX_PACKET_SIZE, 0, &(i->mtu), sizeof(i->mtu));
    DEBUGMSG(DEBUG, "interface's MTU is set to %i\n", i->mtu);

    /* advance interface counter in relay */
    theRelay.ifcount++;

    /* configure the interface to use the specified nettype protocol */
    gnrc_netapi_set(if_pid, NETOPT_PROTO, 0, &netreg_type, sizeof(gnrc_nettype_t));
    /* register for this nettype */
    ccnl_riot_ne.demux_ctx =  GNRC_NETREG_DEMUX_CTX_ALL;
    ccnl_riot_ne.pid = _ccnl_event_loop_pid;
    return gnrc_netreg_register(netreg_type, &ccnl_riot_ne);
}

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
    (void) ccnl;
    int rc;
    DEBUGMSG(TRACE, "ccnl_ll_TX %d bytes\n", buf ? buf->datalen : -1);
    switch(dest->sa.sa_family) {
        case AF_PACKET: {
                            gnrc_pktsnip_t *hdr;
                            gnrc_pktsnip_t *pkt= gnrc_pktbuf_add(NULL, buf->data,
                                                                 buf->datalen,
                                                                 GNRC_NETTYPE_CCN);

                            if (pkt == NULL) {
                                puts("error: packet buffer full");
                                return;
                            }
                            hdr = gnrc_netif_hdr_build(NULL, 0,
                                                       dest->linklayer.sll_addr,
                                                       dest->linklayer.sll_halen);

                            if (hdr == NULL) {
                                puts("error: packet buffer full");
                                gnrc_pktbuf_release(pkt);
                                return;
                            }
                            LL_PREPEND(pkt, hdr);

                            bool is_bcast = true;
                            /* TODO: handle broadcast addresses which are not all 0xFF */
                            for (unsigned i = 0; i < dest->linklayer.sll_halen; i++) {
                                if (dest->linklayer.sll_addr[i] != UINT8_MAX) {
                                    is_bcast = false;
                                    break;
                                }
                            }

                            if (is_bcast) {
                                gnrc_netif_hdr_t *nethdr = (gnrc_netif_hdr_t *)hdr->data;

                                nethdr->flags = GNRC_NETIF_HDR_FLAGS_BROADCAST;
                            }

                            if (gnrc_netapi_send(ifc->if_pid, pkt) < 1) {
                                puts("error: unable to send\n");
                                gnrc_pktbuf_release(pkt);
                                return;
                            }
                            break;
                        }
        default:
                        DEBUGMSG(WARNING, "unknown transport\n");
                        break;
    }
    (void) rc; /* just to silence a compiler warning (if USE_DEBUG is not set) */
}


int ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    (void) ccnl;
    DEBUGMSG(DEBUG, "Received something for the application");

    gnrc_pktsnip_t *pkt= gnrc_pktbuf_add(NULL, c->pkt->content,
                                         c->pkt->contlen,
                                         GNRC_NETTYPE_CCN_CHUNK);
    if (pkt == NULL) {
        DEBUGMSG(WARNING, "Something went wrong allocating buffer for the chunk!");
        return -1;
    }

    if (!gnrc_netapi_dispatch_receive(GNRC_NETTYPE_CCN_CHUNK,
                                      GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        DEBUGMSG(DEBUG, "ccn-lite: unable to forward packet as no one is \
                 interested in it\n");
        gnrc_pktbuf_release(pkt);
    }

    return 0;
}

void ccnl_ageing(void *relay, void *aux)
{
    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(SEC_IN_USEC, ccnl_ageing, relay, 0);
}

void
_receive(struct ccnl_relay_s *ccnl, msg_t *m)
{
    int i;
    for (i = 0; i < ccnl->ifcount; i++) {
        if (ccnl->ifs[i].if_pid == m->sender_pid) {
            break;
        }
    }
    if (i == ccnl->ifcount) {
        puts("No matching CCN interface found, skipping message");
        return;
    }

    gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)m->content.ptr;
    gnrc_pktsnip_t *ccn_pkt, *netif_pkt;
    LL_SEARCH_SCALAR(pkt, ccn_pkt, type, GNRC_NETTYPE_CCN);
    LL_SEARCH_SCALAR(pkt, netif_pkt, type, GNRC_NETTYPE_NETIF);
    gnrc_netif_hdr_t *nethdr = (gnrc_netif_hdr_t *)netif_pkt->data;
    sockunion su;
    memset(&su, 0, sizeof(su));
    su.sa.sa_family = AF_PACKET;
    su.linklayer.sll_halen = nethdr->src_l2addr_len;
    memcpy(&su.linklayer.sll_addr, gnrc_netif_hdr_get_src_addr(nethdr), nethdr->src_l2addr_len);

    ccnl_core_RX(ccnl, i, ccn_pkt->data, ccn_pkt->size, &su.sa, sizeof(su.sa));
    gnrc_pktbuf_release(pkt);
}

void
*_ccnl_event_loop(void *arg)
{
    msg_init_queue(_msg_queue, CCNL_QUEUE_SIZE);
    struct ccnl_relay_s *ccnl = (struct ccnl_relay_s*) arg;
    debug_level = DEBUG;

    ccnl_set_timer(SEC_IN_USEC, ccnl_ageing, ccnl, 0);

    /* XXX: https://xkcd.com/221/ */
    random_init(0x4);

    while(!ccnl->halt_flag) {

        msg_t m, reply;
        DEBUGMSG(VERBOSE, "ccn-lite: waiting for incoming message.\n");
        int usec = ccnl_run_events();
        if (xtimer_msg_receive_timeout(&m, usec) < 0) {
            continue;
        }

        switch (m.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUGMSG(DEBUG, "ccn-lite: GNRC_NETAPI_MSG_TYPE_RCV received\n");
                _receive(ccnl, &m);
                break;

            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUGMSG(DEBUG, "ccn-lite: GNRC_NETAPI_MSG_TYPE_SND received\n");
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                DEBUGMSG(DEBUG, "ccn-lite: reply to unsupported get/set\n");
                reply.content.value = -ENOTSUP;
                msg_reply(&m, &reply);
                break;
            default:
                break;
        }

    }
    return NULL;
}

kernel_pid_t
ccnl_start(void)
{
    loopback_face = ccnl_get_face_or_create(&theRelay, -1, NULL, 0);
    /* start the CCN-Lite event-loop */
    _ccnl_event_loop_pid =  thread_create(_ccnl_stack, sizeof(_ccnl_stack),
                                          THREAD_PRIORITY_MAIN - 1,
                                          THREAD_CREATE_STACKTEST, _ccnl_event_loop,
                                          &theRelay, "ccnl");
    return _ccnl_event_loop_pid;
}

int
ccnl_wait_for_chunk(void *buf, size_t buf_len)
{
    gnrc_netreg_entry_t _ne;
    /* register for content chunks */
    _ne.demux_ctx =  GNRC_NETREG_DEMUX_CTX_ALL;
    _ne.pid = sched_active_pid;
    gnrc_netreg_register(GNRC_NETTYPE_CCN_CHUNK, &_ne);

    int res = (-1);

    while (1) { /* wait for a content pkt (ignore interests) */
        DEBUGMSG(DEBUG, "  waiting for packet\n");

        /* TODO: receive from socket or interface */
        msg_t m;
        if (xtimer_msg_receive_timeout(&m, SEC_IN_USEC) >= 0) {
            DEBUGMSG(DEBUG, "received something\n");
        }
        else {
            /* TODO: handle timeout reasonably */
            DEBUGMSG(WARNING, "timeout\n");
            res = -ETIMEDOUT;
            break;
        }

        if (m.type == GNRC_NETAPI_MSG_TYPE_RCV) {
            DEBUGMSG(TRACE, "It's from the stack!\n");
            gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)m.content.ptr;
            DEBUGMSG(DEBUG, "Type is: %i\n", pkt->type);
            if (pkt->type == GNRC_NETTYPE_CCN_CHUNK) {
                char *c = (char*) pkt->data;
                DEBUGMSG(INFO, "Content is: %s\n", c);
                size_t len = (pkt->size > buf_len) ? buf_len : pkt->size;
                memcpy(buf, pkt->data, len);
                res = (int) len;
                gnrc_pktbuf_release(pkt);
            }
            else {
                DEBUGMSG(WARNING, "Unkown content\n");
                gnrc_pktbuf_release(pkt);
                continue;
            }
            break;
        }
        else {
            /* TODO: reduce timeout value */
            DEBUGMSG(DEBUG, "Unknow message received, ignore it\n");
        }
    }

    /* unregister again, we're not expecting more chunks */
    gnrc_netreg_unregister(GNRC_NETTYPE_CCN_CHUNK, &_ne);

    return res;
}

int
ccnl_send_interest(int suite, char *name, uint8_t *addr,
                               size_t addr_len, unsigned int *chunknum,
                               unsigned char *buf, size_t buf_len)

{
    struct ccnl_prefix_s *prefix;

    if (suite != CCNL_SUITE_NDNTLV) {
        DEBUGMSG(WARNING, "Suite not supported by RIOT!");
        return -1;
    }

    ccnl_mkInterestFunc mkInterest;
    ccnl_isContentFunc isContent;

    mkInterest = ccnl_suite2mkInterestFunc(suite);
    isContent = ccnl_suite2isContentFunc(suite);

    if (!mkInterest || !isContent) {
        DEBUGMSG(WARNING, "No functions for this suite were found!");
        return(-1);
    }

    prefix = ccnl_URItoPrefix(name, suite, NULL, chunknum);

    if (!prefix) {
        DEBUGMSG(ERROR, "prefix could not be created!\n");
        return -1;
    }

    int nonce = random_uint32();
    /* TODO: support other transports than AF_PACKET */
    sockunion sun;
    sun.sa.sa_family = AF_PACKET;
    memcpy(&(sun.linklayer.sll_addr), addr, addr_len);
    sun.linklayer.sll_halen = addr_len;

    struct ccnl_face_s *fibface = ccnl_get_face_or_create(&theRelay, 0, &sun.sa, sizeof(sun.linklayer));
    fibface->flags |= CCNL_FACE_FLAGS_STATIC;
    ccnl_add_fib_entry(&theRelay, prefix, fibface);

    DEBUGMSG(DEBUG, "nonce: %i\n", nonce);

    int len = mkInterest(prefix, &nonce, buf, buf_len);

    unsigned char *start = buf;
    unsigned char *data = buf;
    struct ccnl_pkt_s *pkt;

    int typ;
    int int_len;

    /* TODO: support other suites */
    if (ccnl_ndntlv_dehead(&data, &len, (int*) &typ, &int_len) || (int) int_len > len) {
        DEBUGMSG(WARNING, "  invalid packet format\n");
        return -1;
    }
    pkt = ccnl_ndntlv_bytes2pkt(NDN_TLV_Interest, start, &data, &len);

    struct ccnl_interest_s *i = ccnl_interest_new(&theRelay, loopback_face, &pkt);
    ccnl_interest_append_pending(i, loopback_face);
    ccnl_interest_propagate(&theRelay, i);

    return 0;
}
