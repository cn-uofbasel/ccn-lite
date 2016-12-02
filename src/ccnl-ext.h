/*
 * @f ccnl-ext.h
 * @b header file for CCN lite extentions (forward declarations)
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
 * 2013-03-30 created
 */

#pragma once
#if defined(USE_FRAG) || defined(USE_MGMT) || defined(USE_NFN) || defined(USE_SIGNATURES) || defined(USE_SUITE_LOCALRPC)
# ifndef NEEDS_PACKET_CRAFTING
#  define NEEDS_PACKET_CRAFTING
# endif
#endif

// ----------------------------------------------------------------------

// ccnl-core.c

struct ccnl_interest_s* ccnl_interest_remove(struct ccnl_relay_s *ccnl,
                     struct ccnl_interest_s *i);

// ccnl-core-util.c
#ifndef CCNL_LINUXKERNEL
   char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr,
                    int ccntlv_skip, int escape_components, int call_slash);
#  define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)
#else
   char* ccnl_prefix_to_path(struct ccnl_prefix_s *pr);
#endif
int ccnl_pkt_prependComponent(int suite, char *src, int *offset,
                    unsigned char *buf);
int ccnl_pkt2suite(unsigned char *data, int len, int *skip);
const char* ccnl_suite2str(int suite);
int ccnl_str2suite(char *str);
bool ccnl_isSuite(int suite);
int ccnl_cmp2int(unsigned char *cmp, int cmplen);


struct ccnl_buf_s *ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce);
struct ccnl_buf_s *ccnl_mkSimpleContent(struct ccnl_prefix_s *name, 
                                        unsigned char *payload, int paylen, int *payoffset);

#ifdef USE_CCNxDIGEST
#  define compute_ccnx_digest(buf) SHA256(buf->data, buf->datalen, NULL)
#else
#  define compute_ccnx_digest(b) NULL
#endif

#ifdef USE_FRAG

struct ccnl_frag_s* ccnl_frag_new(int protocol, int mtu);

void ccnl_frag_reset(struct ccnl_frag_s *e, struct ccnl_buf_s *buf,
                     int ifndx, sockunion *su);

int ccnl_frag_getfragcount(struct ccnl_frag_s *e, int origlen,
                           int *totallen);

struct ccnl_buf_s* ccnl_frag_getnext(struct ccnl_frag_s *e,
                                     int *ifndx, sockunion *su);

/*
struct ccnl_buf_s* ccnl_frag_handle_fragment(struct ccnl_relay_s *r,
                struct ccnl_face_s *f, unsigned char *data, int datalen);
*/

void ccnl_frag_destroy(struct ccnl_frag_s *e);

/*
struct ccnl_buf_s* ccnl_frag_fragment(struct ccnl_relay_s *ccnl,
                                        struct ccnl_frag_s *frag,
                                        struct ccnl_buf_s *buf);
*/

// returns >=0 if content consumed, buf and len pointers updated
typedef int (RX_datagram)(struct ccnl_relay_s*, struct ccnl_face_s*,
                          unsigned char**, int*);

int ccnl_frag_RX_frag2012(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from,
                          unsigned char **data, int *datalen);

int ccnl_frag_RX_CCNx2013(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from,
                          unsigned char **data, int *datalen);

/*
int ccnl_frag_RX_Sequenced2015(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from, int mtu,
                          unsigned int bits, unsigned int seqno,
                          unsigned char **data, int *datalen);
*/
int ccnl_frag_RX_BeginEnd2015(RX_datagram callback, struct ccnl_relay_s *relay,
                          struct ccnl_face_s *from, int mtu,
                          unsigned int bits, unsigned int seqno,
                          unsigned char **data, int *datalen);

int ccnl_is_fragment(unsigned char *data, int datalen);
#else
#ifndef ccnl_frag_new
# define ccnl_frag_new(e,u)   NULL
#endif
#ifndef ccnl_frag_destroy
# define ccnl_frag_destroy(e) do{}while(0)
#endif
#ifndef ccnl_frag_handle_fragment
# define ccnl_frag_handle_fragment(r,f,data,len)    ccnl_buf_new(data,len)
#endif
#ifndef ccnl_is_fragment
# define ccnl_is_fragment(d,l)  0
#endif
#endif // USE_FRAG

#ifdef USE_SUITE_LOCALRPC
int ccnl_localrpc_exec(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                       unsigned char **buf, int *buflen);
#endif

#ifdef USE_NACK
void ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                         struct ccnl_face_s *from, int suite);
int ccnl_nfnprefix_contentIsNACK(struct ccnl_content_s *c);
#endif // USE_NACK

#ifdef USE_TIMEOUT_KEEPALIVE
int ccnl_nfnprefix_isKeepalive(struct ccnl_prefix_s *p);
int ccnl_nfnprefix_isIntermediate(struct ccnl_prefix_s *p);
int ccnl_nfn_RX_keepalive(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                          struct ccnl_content_s *c);
#endif

#ifdef USE_NFN
void ccnl_nfn_freeKrivine(struct ccnl_relay_s *ccnl);
int ccnl_nfnprefix_isNFN(struct ccnl_prefix_s *p);
void ZAM_init();

struct ccnl_interest_s*
ccnl_nfn_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

struct ccnl_interest_s*
ccnl_nfn_interest_keepalive(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);

struct ccnl_prefix_s*
ccnl_nfn_mkKeepalivePrefix(struct ccnl_prefix_s *pfx);

struct ccnl_interest_s*
ccnl_nfn_RX_request(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
                    struct ccnl_pkt_s **p);

int
ccnl_nfn_RX_result(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_content_s *c);

struct ccnl_interest_s* ccnl_nfn_request(struct ccnl_relay_s *ccnl,
                                         struct ccnl_face_s *from, int suite, struct ccnl_buf_s *buf,
                                         struct ccnl_prefix_s *p, int minsfx, int maxsfx);

void ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid,
                                   int continue_from_remove);

void ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl,
                                     struct ccnl_buf_s *orig,
                                     struct ccnl_prefix_s *prefix,
                                     struct ccnl_face_s *from,
                                     int suite);
#endif

#ifdef USE_NFN_MONITOR
int ccnl_nfn_monitor(struct ccnl_relay_s *ccnl, struct ccnl_face_s *face,
                     struct ccnl_prefix_s *pr, unsigned char *data, int len);
#else
# define ccnl_nfn_monitor(a,b,c,d,e)    do{}while(0)
#endif // USE_NFN_MONITOR


// ----------------------------------------------------------------------

#ifdef CCNL_LINUXKERNEL

#ifdef USE_IPV4
  struct socket* ccnl_open_udpdev(int port, struct sockaddr_in *sin);
#elif defined(USE_IPV6)
  struct socket* ccnl_open_udpdev(int port, struct sockaddr_in6 *sin);
#endif
# ifdef USE_LINKLAYER
  struct net_device* ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll,
                                      int ethtype);
# endif

#elif defined(CCNL_UNIX)

#ifdef USE_IPV4
  int ccnl_open_udpdev(int port, struct sockaddr_in *si);
#endif
#ifdef USE_IPV6
  int ccnl_open_udp6dev(int port, struct sockaddr_in6 *sin);
#endif
# ifdef USE_LINKLAYER
  int ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype);
# endif

#endif // !CCNL_LINUXKERNEL

// ----------------------------------------------------------------------

#ifdef USE_MGMT

int ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *buf,
              struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);

#else
#ifndef ccnl_mgmt
# define ccnl_mgmt(r,b, p,f)  do{}while(0)
#endif
#endif

// ----------------------------------------------------------------------

#ifdef USE_SCHEDULER

void ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len,
                    void *aux1, void *aux2);
void ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);
void ccnl_sched_destroy(struct ccnl_sched_s *s);

#else
# define ccnl_sched_CTS_done(S,C,L)     do{}while(0)
#ifndef ccnl_sched_destroy
# define ccnl_sched_destroy(S)          do{}while(0)
#endif
#endif

// ----------------------------------------------------------------------

#ifdef USE_UNIXSOCKET

#if defined (CCNL_UNIX)
  int ccnl_open_unixpath(char *path, struct sockaddr_un *ux);
#elif defined(CCNL_LINUXKERNEL)

#endif

#endif // USE_UNIXSOCKET

// ----------------------------------------------------------------------

#ifdef USE_SIGNATURES
int
ccnl_crypto(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
            struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
#endif

// ----------------------------------------------------------------------
// prototypes for platform and extention independend routines:

int ccnl_is_local_addr(sockunion *su);

void ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
                sockunion *dest, struct ccnl_buf_s *buf);

#ifdef CCNL_ARDUINO
  void ccnl_close_socket(EthernetUDP *s);
#elif !defined(CCNL_LINUXKERNEL)
  int ccnl_close_socket(int s);
#endif

// eof
