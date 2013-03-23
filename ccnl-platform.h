/*
 * @f ccnl-platform.h
 * @b header file for CCN lite (CCNL), uniform platform
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
 * 2011-03-30 created
 */

/*
  The CCNL code currently supports four platforms, defined in the
  corresponding main source files.

  Choose there one of the following DEFINES for your platform,
  do not mix them:

  CCNL_UNIX       UNIX user space relay (ccn-lite-relay.c), or
  CCNL_SIMULATION UNIX user space simulator (ccn-lite-simu.c), or
  CCNL_KERNEL     Linux kernel module (ccn-lite-lnxkernel.c), or
  CCNL_OMNET      OmNet++ integration (ccn-lite-omnet.c)

  Note that "ccn-lite-minimalrelay.c" is also a CCNL_UNIX version
  (UDP only), but that it contains all support code by itself and
  does not include the ccnl-platform.c functionality or definitions,
  hence the DEFINE does not matter for that platform.

 */

#ifndef CCNL_KERNEL

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h> // IFNAMSIZE, if_nametoindex

#if defined(__FreeBSD__)
#  include <sys/types.h>
#  undef USE_ETHERNET
   // ethernet support in FreeBSD is work in progress ...
#elif defined(linux)
#  include <endian.h>
#  include <linux/if_ether.h>  // ETH_ALEN
#  include <linux/if_packet.h> // sockaddr_ll
#  define USE_ETHERNET
#endif

#else // else we are compiling for the Linux kernel

#include <linux/ctype.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/inet.h>  // in_aton
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/un.h>

#include <net/inet_sock.h>
#include <net/ip.h>
#include <net/af_unix.h>

#define strtol(s,p,b)	simple_strtol(s,p,b)
#define inet_aton(s,p)	(p)->s_addr = in_aton(s)
#define USE_ETHERNET

#endif // !CCNL_KERNEL


#ifdef USE_SCHEDULER

struct ccnl_scheduler_s* ccnl_scheduler_new();
int ccnl_scheduler_register(struct ccnl_scheduler_s *s,
			    struct ccnl_buf_s* (*getPacket)(void *aux1, void *aux2),
			    void *aux1, void *aux2);
  ;
int ccnl_scheduler_unregister(struct ccnl_scheduler_s *s);
int ccnl_scheduler_rts(struct ccnl_face_s *f, struct ccnl_scheduler_s *s, int pktcnt, int size);
void ccnl_scheduler_destroy(struct ccnl_scheduler_s *s);

#else

#define ccnl_scheduler_new()			NULL
#define ccnl_scheduler_register(S,CB,A1,A2)	do{}while(-1)
#define ccnl_scheduler_unregister(S)		do{}while(-1)
#define ccnl_scheduler_rts(S,C,L)		do{}while(-1)
#define ccnl_scheduler_destroy(S)		do{}while(-1)

#endif

// eof
