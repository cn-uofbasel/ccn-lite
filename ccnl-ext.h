/*
 * @f ccnl-ext.h
 * @b header file for CCN lite extentions (forward declarations)
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
 * 2013-03-30 created
 */

// ----------------------------------------------------------------------

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

typedef int (RX_datagram)(struct ccnl_relay_s*, struct ccnl_face_s*,
			  unsigned char**, int*);

int ccnl_frag_RX_frag2012(RX_datagram callback, struct ccnl_relay_s *relay,
			    struct ccnl_face_s *from,
			    unsigned char **data, int *datalen);

int ccnl_frag_RX_CCNx2013(RX_datagram callback, struct ccnl_relay_s *relay,
			   struct ccnl_face_s *from,
			   unsigned char **data, int *datalen);

int ccnl_is_fragment(unsigned char *data, int datalen);
#else
# define ccnl_frag_new(e,u)   NULL
# define ccnl_frag_destroy(e) do{}while(0)
# define ccnl_frag_handle_fragment(r,f,data,len)    ccnl_buf_new(data,len)
# define ccnl_is_fragment(d,l)  0
#endif // USE_FRAG

// ----------------------------------------------------------------------

#ifdef XXX
#ifdef USE_ETHERNET

#if defined(CCNL_UNIX)
  int ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype);
  int ccnl_open_udpdev(int port, struct sockaddr_in *si);
#elif defined(CCNL_LINUXKERNEL)
  struct net_device* ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll,
				      int ethtype);
  struct socket* ccnl_open_udpdev(int port, struct sockaddr_in *sin);
#endif

#endif // USE_ETHERNET
#endif


#ifdef CCNL_LINUXKERNEL

  struct socket* ccnl_open_udpdev(int port, struct sockaddr_in *sin);
# ifdef USE_ETHERNET
  struct net_device* ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll,
				      int ethtype);
# endif

#elif defined(CCNL_UNIX)

  int ccnl_open_udpdev(int port, struct sockaddr_in *si);
# ifdef USE_ETHERNET
  int ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype);
# endif

#endif // !CCNL_LINUXKERNEL

// ----------------------------------------------------------------------

#ifdef USE_MGMT

int ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *buf,
	      struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);

#else

# define ccnl_mgmt(r,b, p,f)  0

#endif

// ----------------------------------------------------------------------

#ifdef USE_SCHEDULER

void ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len,
		    void *aux1, void *aux2);
void ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);
void ccnl_sched_destroy(struct ccnl_sched_s *s);

#else

# define ccnl_sched_CTS_done(S,C,L)	do{}while(0)
# define ccnl_sched_destroy(S)		do{}while(0)

#endif

// ----------------------------------------------------------------------

#ifdef USE_UNIXSOCKET

#if defined (CCNL_UNIX)
  int ccnl_open_unixpath(char *path, struct sockaddr_un *ux);
#elif defined(CCNL_LINUXKERNEL)

#endif

#endif // USE_UNIXSOCKET

// ----------------------------------------------------------------------

int
ccnl_crypto(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);

// ----------------------------------------------------------------------
// prototypes for platform and extention independend routines:

int ccnl_is_local_addr(sockunion *su);

void ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
		sockunion *dest, struct ccnl_buf_s *buf);

#ifndef CCNL_LINUXKERNEL
void ccnl_close_socket(int s);
#endif

// eof
