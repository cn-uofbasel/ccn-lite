/*
 * @f ccn-lite-riot.c
 * @b RIOT adaption layer
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
 * Copyright (C) 2015, 2016, Oliver Hahm, INRIA
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
#include "sched.h"
#include "random.h"
#include "timex.h"
#include "xtimer.h"
#include "net/gnrc/netreg.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/netapi.h"
#include "net/packet.h"
#include "ccn-lite-riot.h"

#include "ccnl-os-time.h"
#include "ccnl-fwd.h"
#include "ccnl-producer.h"
#include "ccnl-pkt-builder.h"

/**
 * @brief RIOT specific local variables
 * @{
 */

/**
 * @brief message queue for eventloop
 */
static msg_t _msg_queue[CCNL_QUEUE_SIZE];

/**
 * @brief stack for the CCN-Lite eventloop
 */
static char _ccnl_stack[CCNL_STACK_SIZE];

/**
 * currently configured suite
 */
static int _ccnl_suite = CCNL_SUITE_NDNTLV;

/**
 * @}
 */

kernel_pid_t ccnl_event_loop_pid = KERNEL_PID_UNDEF;

evtimer_msg_t ccnl_evtimer;

#include "ccnl-defs.h"
#include "ccnl-core.h"

/**
 * @brief Central relay information
 */
struct ccnl_relay_s ccnl_relay;

/**
 * @brief Local loopback face
 */
static struct ccnl_face_s *loopback_face;

/**
 * @brief Debugging level
 */
extern int debug_level;

/**
 * @brief (Link layer) Send function
 *
 * @par[in] ccnl    Relay to use
 * @par[in] ifc     Interface to send over
 * @par[in] dest    Destination's address information
 * @par[in] buf     Data to send
 */
void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf);

/**
 * @brief Callback for packet reception which should be passed to the application
 *
 * @par[in] ccnl    The relay the packet was received on
 * @par[in] c       Content of the received packet
 *
 * @returns 0 on success
 * @return -1 on error
 */
int ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

/**
 * @brief netreg entry for CCN-Lite packets
 */
static gnrc_netreg_entry_t _ccnl_ne;

/* add a netif to CCN-lite's interfaces, set the nettype, and register a receiver */
int
ccnl_open_netif(kernel_pid_t if_pid, gnrc_nettype_t netreg_type)
{
    assert(pid_is_valid(if_pid));
    if (gnrc_netif_get_by_pid(if_pid) == NULL) {
        return -1;
    }
    if (ccnl_relay.ifcount >= CCNL_MAX_INTERFACES) {
        DEBUGMSG(WARNING, "cannot open more than %u interfaces for CCN-Lite\n",
                 (unsigned) CCNL_MAX_INTERFACES);
        return -1;
    }

    /* get current interface from CCN-Lite's relay */
    struct ccnl_if_s *i;
    i = &ccnl_relay.ifs[ccnl_relay.ifcount];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    i->if_pid = if_pid;
    i->addr.sa.sa_family = AF_PACKET;

    int res;
    uint16_t mtu;
    res = gnrc_netapi_get(if_pid, NETOPT_MAX_PDU_SIZE, 0, &(mtu), sizeof(mtu));
    if (res < 0) {
        DEBUGMSG(ERROR, "error: unable to determine MTU for if=<%u>\n", (unsigned) i->if_pid);
        return -ECANCELED;
    }
    i->mtu = (int)mtu;
    DEBUGMSG(DEBUG, "interface's MTU is set to %lu\n", (unsigned long) i->mtu);

    res = gnrc_netapi_get(if_pid, NETOPT_ADDR_LEN, 0, &(i->addr_len), sizeof(i->addr_len));
    if (res < 0) {
        DEBUGMSG(ERROR, "error: unable to determine address length for if=<%u>\n", (unsigned) if_pid);
        return -ECANCELED;
    }
    DEBUGMSG(DEBUG, "interface's address length is %u\n", (unsigned) i->addr_len);

    res = gnrc_netapi_get(if_pid, NETOPT_ADDRESS, 0, i->hwaddr, i->addr_len);
    if (res < 0) {
        DEBUGMSG(ERROR, "error: unable to get address for if=<%u>\n", (unsigned) if_pid);
        return -ECANCELED;
    }
    DEBUGMSG(DEBUG, "interface's address is %s\n", ll2ascii(i->hwaddr, i->addr_len));

    /* advance interface counter in relay */
    ccnl_relay.ifcount++;

    /* configure the interface to use the specified nettype protocol */
    gnrc_netapi_set(if_pid, NETOPT_PROTO, 0, &netreg_type, sizeof(gnrc_nettype_t));
    /* register for this nettype if not already done */
    if (_ccnl_ne.demux_ctx == 0) {
        gnrc_netreg_entry_init_pid(&_ccnl_ne, GNRC_NETREG_DEMUX_CTX_ALL,
                                   ccnl_event_loop_pid);
        return gnrc_netreg_register(netreg_type, &_ccnl_ne);
    }
    return 0;
}

/* (link layer) sending function */
void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
    (void) ccnl;
    int rc;
    DEBUGMSG(TRACE, "ccnl_ll_TX %d bytes to %s\n", (buf ? (int) buf->datalen : -1), ccnl_addr2ascii(dest));

    (void) ifc;
    switch(dest->sa.sa_family) {
        /* link layer sending */
        case AF_PACKET: {
                            /* allocate memory */
                            gnrc_pktsnip_t *hdr = NULL;
                            gnrc_pktsnip_t *pkt= gnrc_pktbuf_add(NULL, buf->data,
                                                                 buf->datalen,
                                                                 GNRC_NETTYPE_CCN);

                            if (pkt == NULL) {
                                printf("error: packet buffer full trying to allocate %d bytes\n", (int)buf->datalen);
                                return;
                            }

                            /* check for loopback */
                            bool is_loopback = false;
                            if (ifc->addr_len == dest->linklayer.sll_halen) {
                                if (memcmp(ifc->hwaddr, dest->linklayer.sll_addr, dest->linklayer.sll_halen) == 0) {
                                    /* build link layer header */
                                    hdr = gnrc_netif_hdr_build(NULL, dest->linklayer.sll_halen,
                                                               dest->linklayer.sll_addr,
                                                               dest->linklayer.sll_halen);

                                    gnrc_netif_hdr_set_src_addr((gnrc_netif_hdr_t *)hdr->data, ifc->hwaddr, ifc->addr_len);
                                    is_loopback = true;
                                }
                            }

                            /* for the non-loopback case */
                            if (hdr == NULL) {
                                hdr = gnrc_netif_hdr_build(NULL, 0,
                                                           dest->linklayer.sll_addr,
                                                           dest->linklayer.sll_halen);
                            }

                            /* check if header building succeeded */
                            if (hdr == NULL) {
                                puts("error: packet buffer full trying to allocate netif_hdr");
                                gnrc_pktbuf_release(pkt);
                                return;
                            }
                            LL_PREPEND(pkt, hdr);

                            if (is_loopback) {
                                    DEBUGMSG(DEBUG, "loopback packet\n");
                                    if (gnrc_netapi_receive(ccnl_event_loop_pid, pkt) < 1) {
                                        DEBUGMSG(ERROR, "error: unable to loopback packet, discard it\n");
                                        gnrc_pktbuf_release(pkt);
                                    }
                                    return;
                            }

                            /* distinguish between broadcast and unicast */
                            bool is_bcast = true;
                            /* TODO: handle broadcast addresses which are not all 0xFF */
                            for (unsigned i = 0; i < dest->linklayer.sll_halen; i++) {
                                if (dest->linklayer.sll_addr[i] != UINT8_MAX) {
                                    is_bcast = false;
                                    break;
                                }
                            }

                            if (is_bcast) {
                                DEBUGMSG(DEBUG, " is broadcast\n");
                                gnrc_netif_hdr_t *nethdr = (gnrc_netif_hdr_t *)hdr->data;
                                nethdr->flags = GNRC_NETIF_HDR_FLAGS_BROADCAST;
                            }

                            /* actual sending */
                            DEBUGMSG(DEBUG, " try to pass to GNRC (%i): %p\n", (int) ifc->if_pid, (void*) pkt);
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

/* packets delivered to the application */
int
ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c)
{
    (void) ccnl;
    DEBUGMSG(DEBUG, "Received something of size %u for the application\n", c->pkt->contlen);

    gnrc_pktsnip_t *pkt= gnrc_pktbuf_add(NULL, c->pkt->content,
                                         c->pkt->contlen,
                                         GNRC_NETTYPE_CCN_CHUNK);
    if (pkt == NULL) {
        DEBUGMSG(WARNING, "Something went wrong allocating buffer for the chunk!\n");
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

/* receiving callback for CCN packets */
void
_receive(struct ccnl_relay_s *ccnl, msg_t *m)
{
    int i;
    /* iterate over interfaces */
    for (i = 0; i < ccnl->ifcount; i++) {
        if (ccnl->ifs[i].if_pid == m->sender_pid) {
            break;
        }
    }

    if (i == ccnl->ifcount) {
        DEBUGMSG(WARNING, "No matching CCN interface found, assume it's from the default interface\n");
        i = 0;
    }

    /* packet parsing */
    gnrc_pktsnip_t *pkt = (gnrc_pktsnip_t *)m->content.ptr;
    gnrc_pktsnip_t *ccn_pkt, *netif_pkt;
    LL_SEARCH_SCALAR(pkt, ccn_pkt, type, GNRC_NETTYPE_CCN);
    LL_SEARCH_SCALAR(pkt, netif_pkt, type, GNRC_NETTYPE_NETIF);
    gnrc_netif_hdr_t *nethdr = (gnrc_netif_hdr_t *)netif_pkt->data;
    sockunion su;
    memset(&su, 0, sizeof(su));
    (void )nethdr;
    su.sa.sa_family = AF_PACKET;
    su.linklayer.sll_halen = nethdr->src_l2addr_len;
    memcpy(su.linklayer.sll_addr, gnrc_netif_hdr_get_src_addr(nethdr), nethdr->src_l2addr_len);

    /* call CCN-lite callback and free memory in packet buffer */
    ccnl_core_RX(ccnl, i, ccn_pkt->data, ccn_pkt->size, &su.sa, sizeof(su.sa));
    gnrc_pktbuf_release(pkt);
}

static void
ccnl_interest_retransmit(struct ccnl_relay_s *relay, struct ccnl_interest_s *ccnl_int)
{
    if(ccnl_int->retries >= CCNL_MAX_INTEREST_RETRANSMIT) {
        return;
    }

    ccnl_int->evtmsg_retrans.msg.type = CCNL_MSG_INT_RETRANS;
    ccnl_int->evtmsg_retrans.msg.content.ptr = ccnl_int;
    ((evtimer_event_t *)&ccnl_int->evtmsg_retrans)->offset = CCNL_INTEREST_RETRANS_TIMEOUT;
    evtimer_add_msg(&ccnl_evtimer, &ccnl_int->evtmsg_retrans, ccnl_event_loop_pid);
    ccnl_int->retries++;
    ccnl_interest_propagate(relay, ccnl_int);
}

/* the main event-loop */
void
*_ccnl_event_loop(void *arg)
{
    struct ccnl_content_s *content;
    struct ccnl_pkt_s *pkt;
    struct ccnl_prefix_s *prefix;
    struct ccnl_interest_s *ccnl_int;
    struct ccnl_face_s *face;
    char *spref;

    msg_init_queue(_msg_queue, CCNL_QUEUE_SIZE);
    evtimer_init_msg(&ccnl_evtimer);
    struct ccnl_relay_s *ccnl = (struct ccnl_relay_s*) arg;

    while(!ccnl->halt_flag) {
        msg_t m, reply, mr;
        DEBUGMSG(VERBOSE, "ccn-lite: waiting for incoming message.\n");
        msg_receive(&m);

        switch (m.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                DEBUGMSG(DEBUG, "ccn-lite: GNRC_NETAPI_MSG_TYPE_RCV received\n");
                _receive(ccnl, &m);
                break;

            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUGMSG(DEBUG, "ccn-lite: GNRC_NETAPI_MSG_TYPE_SND received\n");
                pkt = (struct ccnl_pkt_s *) m.content.ptr;
                ccnl_fwd_handleInterest(ccnl, loopback_face, &pkt, ccnl_ndntlv_cMatch);
                ccnl_pkt_free(pkt);
                break;

            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                DEBUGMSG(DEBUG, "ccn-lite: reply to unsupported get/set\n");
                reply.content.value = -ENOTSUP;
                msg_reply(&m, &reply);
                break;
            case CCNL_MSG_CS_ADD:
                DEBUGMSG(VERBOSE, "ccn-lite: CS add\n");
                content = (struct ccnl_content_s *)m.content.ptr;
                ccnl_cs_add(ccnl, content);
                break;
            case CCNL_MSG_CS_DEL:
                DEBUGMSG(VERBOSE, "ccn-lite: CS remove\n");
                prefix = (struct ccnl_prefix_s *)m.content.ptr;
                spref = ccnl_prefix_to_path(prefix);
                if (!spref) {
                    DEBUGMSG(WARNING, "ccn-lite: CS remove failed, because of no memory available\n");
                    break;
                }
                if (ccnl_cs_remove(ccnl, spref) < 0) {
                    DEBUGMSG(WARNING, "removing CS entry failed\n");
                }
                ccnl_free(spref);
                break;
            case CCNL_MSG_CS_LOOKUP:
                DEBUGMSG(VERBOSE, "ccn-lite: CS lookup\n");
                prefix = (struct ccnl_prefix_s *)m.content.ptr;
                spref = ccnl_prefix_to_path(prefix);
                mr.type = CCNL_MSG_CS_LOOKUP;
                if (spref) {
                    content = ccnl_cs_lookup(ccnl, spref);
                    ccnl_free(spref);
                }
                else {
                    DEBUGMSG(WARNING, "ccn-lite: CS lookup failed, because of no memory available\n");
                    content = NULL;
                }
                mr.content.ptr = content;
                msg_reply(&m, &mr);
                break;
            case CCNL_MSG_INT_RETRANS:
                ccnl_int = (struct ccnl_interest_s *)m.content.ptr;
                ccnl_interest_retransmit(ccnl, ccnl_int);
                break;
            case CCNL_MSG_INT_TIMEOUT:
                ccnl_int = (struct ccnl_interest_s *)m.content.ptr;
                ccnl_interest_remove(ccnl, ccnl_int);
                break;
            case CCNL_MSG_FACE_TIMEOUT:
                face = (struct ccnl_face_s *)m.content.ptr;
                if (face &&
                    !(face->flags & CCNL_FACE_FLAGS_STATIC)) {
                    ccnl_face_remove(ccnl, face);
                }
                break;
            default:
                DEBUGMSG(WARNING, "ccn-lite: unknown message type\n");
                break;
        }

    }
    return NULL;
}

/* trampoline function creating the loopback face */
kernel_pid_t
ccnl_start(void)
{
    loopback_face = ccnl_get_face_or_create(&ccnl_relay, -1, NULL, 0);
    loopback_face->flags |= CCNL_FACE_FLAGS_STATIC;

    ccnl_relay.max_cache_entries = CCNL_CACHE_SIZE;
    ccnl_relay.max_pit_entries = CCNL_DEFAULT_MAX_PIT_ENTRIES;
    ccnl_relay.ccnl_ll_TX_ptr = &ccnl_ll_TX;

    /* start the CCN-Lite event-loop */
    ccnl_event_loop_pid =  thread_create(_ccnl_stack, sizeof(_ccnl_stack),
                                          CCNL_THREAD_PRIORITY,
                                          THREAD_CREATE_STACKTEST, _ccnl_event_loop,
                                          &ccnl_relay, "ccnl");
    return ccnl_event_loop_pid;
}

static xtimer_t _wait_timer;
static msg_t _timeout_msg;
int
ccnl_wait_for_chunk(void *buf, size_t buf_len, uint64_t timeout)
{
    int res = (-1);

    if (timeout == 0) {
        timeout = CCNL_MAX_INTEREST_RETRANSMIT * US_PER_SEC;
    }

    while (1) { /* wait for a content pkt (ignore interests) */
        DEBUGMSG(DEBUG, "  waiting for packet\n");

        /* TODO: receive from socket or interface */
        _timeout_msg.type = CCNL_MSG_TIMEOUT;
        xtimer_set_msg64(&_wait_timer, timeout, &_timeout_msg, thread_getpid());
        msg_t m;
        msg_receive(&m);
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
            xtimer_remove(&_wait_timer);
            break;
        }
        else if (m.type == CCNL_MSG_TIMEOUT) {
            res = -ETIMEDOUT;
            break;
        }
        else {
            /* TODO: reduce timeout value */
            DEBUGMSG(DEBUG, "Unknow message received, ignore it\n");
        }
    }

    return res;
}

/* TODO: move everything below here to ccn-lite-core-utils */

/* generates and send out an interest */
int
ccnl_send_interest(struct ccnl_prefix_s *prefix, unsigned char *buf, int buf_len,
                   ccnl_interest_opts_u *int_opts)
{
    int ret = 0;
    size_t len = 0;
    ccnl_interest_opts_u default_opts;
    default_opts.ndntlv.nonce = 0;
    default_opts.ndntlv.mustbefresh = false;
    default_opts.ndntlv.interestlifetime = CCNL_INTEREST_TIMEOUT * 1000; // ms

    if (_ccnl_suite != CCNL_SUITE_NDNTLV) {
        DEBUGMSG(WARNING, "Suite not supported by RIOT!\n");
        return -1;
    }

    DEBUGMSG(INFO, "interest for chunk number: %lu\n", (prefix->chunknum == NULL) ? (unsigned long) 0 : (unsigned long) *prefix->chunknum);

    if (!prefix) {
        DEBUGMSG(ERROR, "prefix could not be created!\n");
        return -2;
    }

    if (!int_opts) {
        int_opts = &default_opts;
    }

    if (!int_opts->ndntlv.nonce) {
        int_opts->ndntlv.nonce = random_uint32();
    }

    DEBUGMSG(DEBUG, "nonce: %" PRIi32 "\n", int_opts->ndntlv.nonce);

    ccnl_mkInterest(prefix, int_opts, buf, (buf + buf_len), &len, (size_t *)&buf_len);

    buf += buf_len;

    unsigned char *start = buf;
    unsigned char *data = buf;
    struct ccnl_pkt_s *pkt, *pktc;
    (void) pktc;

    uint64_t typ;
    size_t int_len;

    /* TODO: support other suites */
    if (ccnl_ndntlv_dehead(&data, &len, &typ, &int_len) || (int_len > len)) {
        DEBUGMSG(WARNING, "  invalid packet format\n");
        return -3;
    }

    pkt = ccnl_ndntlv_bytes2pkt(NDN_TLV_Interest, start, &data, &len);

    if (!pkt) {
        DEBUGMSG(WARNING, "could not create Interest pkt\n");
        return -4;
    }

    msg_t m = { .type = GNRC_NETAPI_MSG_TYPE_SND, .content.ptr = pkt };
    ret = msg_send(&m, ccnl_event_loop_pid);
    if(ret < 1){
        DEBUGMSG(WARNING, "could not send Interest: %i\n", ret);
    }

    return ret;
}
