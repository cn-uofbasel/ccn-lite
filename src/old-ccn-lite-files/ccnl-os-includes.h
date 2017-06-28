/*
 * @f ccnl-os-includes.h
 * @b does the #include of all (system) header file
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

#ifndef CCNL_LINUXKERNEL

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CCNL_UNIX

# include <time.h>
# include <getopt.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/un.h>
# include <sys/utsname.h>

#ifndef _DEFAULT_SOURCE
#  define __USE_MISC
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h> // IFNAMSIZE, if_nametoindex

#ifdef _DEFAULT_SOURCE
  int inet_aton(const char *cp, struct in_addr *inp);
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#  include <sys/types.h>
#  undef USE_LINKLAYER
   // ethernet support in FreeBSD is work in progress ...
#elif defined(linux)
#  include <endian.h>
#  include <linux/if_ether.h>  // ETH_ALEN
#  include <linux/if_packet.h> // sockaddr_ll
#endif

#ifdef USE_CCNxDIGEST
#  include <openssl/sha.h>
#endif
#endif // CCNL_UNIX

#else // else we are compiling for the Linux kernel

#include <stddef.h>

#include <linux/ctype.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/socket.h>
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
#include <linux/string.h>
#include <linux/un.h>

#include <net/inet_sock.h>
#include <net/ip.h>
#include <net/af_unix.h>

#define strtol(s,p,b)	simple_strtol(s,p,b)
#define inet_aton(s,p)	(p)->s_addr = in_aton(s)
#define USE_LINKLAYER

#endif // CCNL_LINUXKERNEL

// eof
