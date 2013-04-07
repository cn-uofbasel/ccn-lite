/*
 * @f ccn-lite-lnxkernel.c
 * @b Linux kernel version of the CCN lite relay
 *
 * Copyright (C) 2012, Christian Tschudin, University of Basel
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
 * 2012-01-06 created
 */

#define CCNL_LINUXKERNEL

#define USE_DEBUG
// #define USE_DEBUG_MALLOC
// #define USE_ENCAPS
#define USE_ETHERNET
// #define USE_SCHEDULER
// #define USE_MGMT

#include "ccnl-includes.h"
#include "ccnl.h"
#include "ccnx.h"
#include "ccnl-core.h"

// ----------------------------------------------------------------------

#define ccnl_print_stats(x,y)		do{}while(0)
#define ccnl_app_RX(x,y)		do{}while(0)

static struct ccnl_relay_s theRelay;

static int ccnl_eth_RX(struct sk_buff *skb, struct net_device *indev, 
		       struct packet_type *pt, struct net_device *outdev);

void ccnl_udp_data_ready(struct sock *sk, int len);

// ----------------------------------------------------------------------

/*
static int
random(void)
{
    int rand;
    get_random_bytes(&rand, sizeof(rand));
    return rand;
}
*/

static char*
inet_ntoa(struct in_addr in)
{
    static char buf[16];
    unsigned int i, len = 0, a = ntohl(in.s_addr);
    for (i = 0; i < 4; i++) {
	len += sprintf(buf+len, "%s%d", i ? "." : "", 0xff & (a >> 8*(3-i)));
    }
    return buf;
}

static inline void*
ccnl_malloc(int s)
{
    return kmalloc(s, GFP_ATOMIC);
}

static inline void*
ccnl_calloc(int n, int s)
{
    return kcalloc(n, s, GFP_ATOMIC);
}

static inline void
ccnl_free(void *ptr)
{
    kfree(ptr);
}

static void ccnl_lnxkernel_cleanup(void);
char* ccnl_addr2ascii(sockunion *su);

struct udp_send_s {
    struct work_struct w;
    struct ccnl_if_s *iface;
//    struct socket *sock;
    struct sockaddr_in ip4;
    void *data;
    unsigned int datalen;
};

void do_udpsend(struct work_struct *work)
{
    struct udp_send_s *u = (struct udp_send_s*) work;
    struct iovec iov;
    struct msghdr msg;
    int rc;
    mm_segment_t oldfs;

    iov.iov_base = u->data;
    iov.iov_len = u->datalen;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &u->ip4;
    msg.msg_namelen = sizeof(u->ip4);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    printk("before sock_sendmsg: sock %p, dest <%s>, buf %p %d\n",
	   u->iface->sock, ccnl_addr2ascii((sockunion*)&u->ip4), u->data, u->datalen);

    rc = sock_sendmsg(u->iface->sock, &msg, u->datalen);
    set_fs(oldfs);

    printk("UDP sendmsg returned %d\n", rc);
    ccnl_free(u->data);
    ccnl_free(u);
}


void
ccnl_ll_TX(struct ccnl_relay_s *relay, struct ccnl_if_s *ifc,
	    sockunion *dest, struct ccnl_buf_s *buf)
{
    if (!dest)
	return;

    printk("in_interrupt=%ld\n", in_interrupt());

    switch (dest->sa.sa_family) {
    case AF_INET:
    {
	struct udp_send_s *u = ccnl_malloc(sizeof(struct udp_send_s));

	INIT_WORK((struct work_struct*)u, do_udpsend);

	u->data = ccnl_malloc(buf->datalen);
	memcpy(u->data, buf->data, buf->datalen);
	u->datalen = buf->datalen;
	memcpy(&u->ip4, &dest->ip4, sizeof(dest->ip4));
	u->iface = ifc;

	queue_work(ifc->wq, (struct work_struct*)u);

	break;
    }
    case AF_PACKET:
    {
	struct net_device *dev = ifc->netdev;
	struct sk_buff *skb;

	if (!dev)
	    return;
	skb = alloc_skb(dev->hard_header_len + buf->datalen, GFP_ATOMIC);
	if (!skb)
	    return;
	skb->dev = dev;
	skb->protocol = htons(CCNL_ETH_TYPE);
	skb_reserve(skb, dev->hard_header_len);
	skb_reset_network_header(skb);
	skb->len = buf->datalen;
	memcpy(skb->data, buf->data, skb->len);
	if (dev_hard_header(skb, dev, CCNL_ETH_TYPE, dest->eth.sll_addr,
			    ifc->addr.eth.sll_addr, skb->len))
	    dev_queue_xmit(skb);
	else
	    kfree_skb(skb);
	break;
    }
    case AF_UNIX:
    {
	struct iovec iov;
	struct msghdr msg;
	int rc;
	mm_segment_t oldfs;

	iov.iov_base =  buf->data;
	iov.iov_len = buf->datalen;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = dest; //->ux.sun_path;
	msg.msg_namelen = sizeof(sa_family_t) + strlen(dest->ux.sun_path) + 1;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	rc = sock_sendmsg(ifc->sock, &msg, buf->datalen);
	set_fs(oldfs);

	//	printk("UNIX sendmsg returned %d\n", rc);
	break;
    }
    default:
	break;
    }
}

void
ccnl_close_socket(struct socket *s)
{
    // socket will be released in the cleanup routine
}

// ----------------------------------------------------------------------

#include "ccnl-ext-debug.c"
#include "ccnl-platform.c"
#include "ccnl-ext.h"

#include "ccnl-core.c"

#ifdef USE_ENCAPS
#  include "ccnl-pdu.c"
#  include "ccnl-ext-encaps.c"
#endif

#include "ccnl-ext-mgmt.c"

#ifdef USE_SCHEDULER
#  include "ccnl-ext-sched.c"

int inter_ccn_interval = 100;

struct ccnl_sched_s*
ccnl_lnx_defaultFaceScheduler(struct ccnl_relay_s *ccnl,
				    void(*cb)(void*,void*))
{
    return ccnl_sched_pktrate_new(cb, ccnl, inter_ccn_interval);
}

#endif

// ----------------------------------------------------------------------

static int
ccnl_eth_RX(struct sk_buff *skb, struct net_device *indev, 
	  struct packet_type *pt, struct net_device *outdev){
    int i;
    sockunion so;

    for (i = 0; i < theRelay.ifcount; i++)
	if (theRelay.ifs[i].netdev == indev)
	    break;
    if (!theRelay.halt_flag &&  i < theRelay.ifcount) {
	so.sa.sa_family = AF_PACKET;
	memcpy(so.eth.sll_addr, skb->data, ETH_ALEN);
	ccnl_core_RX(&theRelay, i, skb->data, skb->len,
		      &so.sa, sizeof(so.eth));
    }
    kfree_skb(skb);

    return 1;
}


void
ccnl_udp_data_ready(struct sock *sk, int len)
{
    struct sk_buff *skb;
    int i, err;
    DEBUGMSG(99, "ccnl_udp_data_ready %d bytes\n", len);

    if ((skb = skb_recv_datagram(sk, 0, 1, &err)) == NULL)
	goto Bail;
    for (i = 0; i < theRelay.ifcount; i++)
	if (theRelay.ifs[i].sock->sk == sk)
	    break;
    if (!theRelay.halt_flag && i < theRelay.ifcount) {
	sockunion su;
	su.sa.sa_family = AF_INET;
	su.ip4.sin_addr.s_addr = ((struct iphdr *)skb_network_header(skb))->saddr;
	su.ip4.sin_port = udp_hdr(skb)->source;
	DEBUGMSG(99, "ccnl_udp_data_ready2: if=%d, %d bytes, src=%s\n",
		 i, skb->len, ccnl_addr2ascii(&su));
	ccnl_core_RX(&theRelay, i, skb->data + sizeof(struct udphdr),
		      skb->len - sizeof(struct udphdr), &su.sa, sizeof(su.ip4));
    }
    kfree_skb(skb);

    return;
Bail:
    return;
}


void
ccnl_ux_data_ready(struct sock *sk, int len)
{
/*
    struct ccnl_relay_s *ccnl = sk->sk_user_data;
    struct kvec vec[1];
    struct msghdr msg;
    struct ccnl_buf_s *buf;
    sockunion from;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = from.ux.sun_path;
    msg-msg_namelen = sizeof(from);
    buf = ccnl_buf_new(NULL, len);
    if (!buf)
	len = 0;
    else
	vec. = buf->data;
    vec. = len;
    len = kernel_recvmsg(sk, &msg, vec, 1, len);
    if (len > 0) {
    }
*/
    struct sk_buff *skb;
    int i, err;
    DEBUGMSG(99, "ccnl_ux_data_ready %d bytes\n", len);

    if ((skb = skb_recv_datagram(sk, 0, 1, &err)) == NULL)
	goto Bail;
    for (i = 0; i < theRelay.ifcount; i++)
	if (theRelay.ifs[i].sock->sk == sk)
	    break;
    if (!theRelay.halt_flag && i < theRelay.ifcount) {
	struct unix_sock *u = (struct unix_sock *)(skb->sk);
	sockunion sux;
	if (u && u->addr)
	    memcpy(&sux, u->addr->name, u->addr->len);
	else
	    sux.sa.sa_family = 0;
	DEBUGMSG(99, "ccnl_ux_data_ready2: if=%d, %d bytes, src=%s\n",
		 i, skb->len, ccnl_addr2ascii(&sux));

	ccnl_core_RX(&theRelay, i, skb->data, skb->len,
		      &sux.sa, sizeof(sux.ux));
    }
    kfree_skb(skb);

    return;
Bail:
    return;
}

// ----------------------------------------------------------------------

static void
ccnl_ageing(void *ptr, void *dummy)
{
    struct ccnl_relay_s *relay = (struct ccnl_relay_s*) ptr;

    ccnl_do_ageing(relay, dummy);
    ccnl_set_timer(1000000, ccnl_ageing, relay, dummy);
}

// ----------------------------------------------------------------------

struct net_device*
ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll)
{
    struct net_device *dev;

    dev = dev_get_by_name(&init_net, devname);
    if (!dev)
	return NULL;

    sll->sll_family = AF_PACKET;
    sll->sll_protocol = htons(CCNL_ETH_TYPE);
    memcpy(sll->sll_addr, dev->dev_addr, ETH_ALEN);
    sll->sll_ifindex = dev->ifindex;

    printk("%s: access to %s with MAC=%s\n", THIS_MODULE->name,
	   devname, eth2ascii(sll->sll_addr));
    //    dev_put(dev);
    return dev;
}


struct socket*
ccnl_open_udpdev(int port, struct sockaddr_in *sin)
{
    struct socket *s;
    int rc;

    rc = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &s);
    if (rc < 0) {
	DEBUGMSG(1, "Error %d creating UDP socket\n", rc);
	return NULL;
    }

    DEBUGMSG(1, "UDP socket is %p\n", s);

    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = htonl(INADDR_ANY);
    sin->sin_port = htons(port);
    rc = s->ops->bind(s, (struct sockaddr*) sin, sizeof(*sin));
    if (rc < 0) {
	DEBUGMSG(1, "Error %d binding UDP socket\n", rc);
	goto Bail;
    }

    return s;
Bail:
    sock_release(s);
    return NULL;
}


struct socket*
ccnl_open_unixpath(char *path, struct sockaddr_un *ux)
{
    struct socket *s;
    int rc;

    rc = sock_create(AF_UNIX, SOCK_DGRAM, 0, &s);
    if (rc < 0) {
	DEBUGMSG(1, "Error %d creating UNIX socket %s\n", rc, path);
	return NULL;
    }
    DEBUGMSG(1, "UNIX socket is %p\n", s);

    ux->sun_family = AF_UNIX;
    strcpy(ux->sun_path, path);
    rc = s->ops->bind(s, (struct sockaddr*) ux,
		      sizeof(ux->sun_family) + strlen(path) + 1); // sizeof(ux));
    if (rc < 0) {
	DEBUGMSG(1, "Error %d binding UNIX socket to %s "
		    "(remove first, check access rights)\n", rc, path);
	goto Bail;
    }

    return s;
Bail:
    sock_release(s);
    return NULL;
}

// ----------------------------------------------------------------------

// static char *ethdevname = 0;
static char *ux_sock_path = 0;
static int c = CCNL_DEFAULT_MAX_CACHE_ENTRIES; // no memory by default
static int v = 0;
static void *ageing_handler = NULL;

/*
module_param(ethdevname, charp, 0);
MODULE_PARM_DESC(ethdevname, "name of ethernet device to serve");
 */
module_param(c, int, 0);
MODULE_PARM_DESC(c, "max number of cache entries");

module_param(v, int, 0);
MODULE_PARM_DESC(v, "verbosity level");

module_param(ux_sock_path, charp, 0);
MODULE_PARM_DESC(uk_sock_path, "name (path) of mgmt unix socket");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("christian.tschudin@unibas.ch");

static int __init
ccnl_init(void)
{
    struct ccnl_if_s *i;

    printk("%s: init u=%s c=%d, v=%d\n", THIS_MODULE->name, ux_sock_path, c, v);
    if (v >= 0)
	debug_level = v;
    theRelay.max_cache_entries = c;
#ifdef USE_SCHEDULER
    theRelay.defaultFaceScheduler = ccnl_lnx_defaultFaceScheduler;
#endif

    ageing_handler = ccnl_set_timer(1000000, ccnl_ageing, &theRelay, 0);

    i = &theRelay.ifs[0];
    i->sock = ccnl_open_unixpath(ux_sock_path, &i->addr.ux);
    if (i->sock) {
	DEBUGMSG(99, "ccnl_open_unixpath worked\n");
//	i->encaps = CCNL_DGRAM_ENCAPS_ETH2011;
	i->mtu = 4048;
	i->reflect = 0;
	i->fwdalli = 0;
	write_lock_bh(&i->sock->sk->sk_callback_lock);
	i->old_data_ready = i->sock->sk->sk_data_ready;
	i->sock->sk->sk_data_ready = ccnl_ux_data_ready;
//	i->sock->sk->sk_user_data = &theRelay;
	write_unlock_bh(&i->sock->sk->sk_callback_lock);

	theRelay.ifcount++;
    }

/*
    i = &theRelay.ifs[0];
    i->netdev = ccnl_open_ethdev(ethdevname, &i->addr.eth);
    if (i->netdev) {
//	i->encaps = CCNL_DGRAM_ENCAPS_ETH2011;
	i->mtu = 1500;
	i->reflect = 1;
	i->fwdalli = 1;
	i->ccnl_packet.type = htons(CCNL_ETH_TYPE);
	i->ccnl_packet.dev = i->netdev;
	i->ccnl_packet.func = ccnl_eth_RX;
	dev_add_pack(&i->ccnl_packet);
	theRelay.ifcount++;
    }
*/

    /*
    ccnl_proc = create_proc_entry(PROCFS_NAME, 0644, NULL);
    if (ccnl_proc) {
	ccnl_proc->read_proc = ccnl_proc_read;
	ccnl_proc->write_proc = ccnl_proc_write;
	ccnl_proc_buflen = 0;
    }
    */
    return 0;
}

static void
ccnl_lnxkernel_cleanup(void)
{
    int j;

    printk("%s: ccnl_lnxkernel_cleanup\n", THIS_MODULE->name);

    if (ageing_handler) {
	ccnl_rem_timer(ageing_handler);
	ageing_handler = 0;
    }

    ccnl_core_cleanup(&theRelay);

    for (j = 0; j < theRelay.ifcount; j++) {
	struct ccnl_if_s *i = &theRelay.ifs[j];

	if (i->netdev) { // unbind from the ETH packet driver
	    dev_remove_pack(&i->ccnl_packet);
	    dev_put(i->netdev);
	    i->netdev = NULL;
	}
	
	if (i->sock) {
	    if (i->addr.sa.sa_family == AF_UNIX) {
		write_lock_bh(&i->sock->sk->sk_callback_lock);
		i->sock->sk->sk_data_ready = i->old_data_ready;
		write_unlock_bh(&i->sock->sk->sk_callback_lock);
	    }
	    sock_release(i->sock);
	    i->sock = 0;
	}
    }
    theRelay.ifcount = 0;

    if (ux_sock_path) { // also remove the path
	struct path p;
	int rc;

	rc = kern_path(ux_sock_path, 0, &p);
	if (!rc) {
	    struct dentry *dir = dget_parent(p.dentry);

	    mutex_lock_nested(&(dir->d_inode->i_mutex), I_MUTEX_PARENT);
	    rc = vfs_unlink(dir->d_inode, p.dentry);
	    mutex_unlock(&dir->d_inode->i_mutex);
	    dput(dir);
	    path_put(&p);
	}
    }
    ux_sock_path = NULL;
}

static void __exit
ccnl_exit( void )
{
    int j;

    printk("%s: exit\n", THIS_MODULE->name);

    if (ageing_handler)
	ccnl_rem_timer(ageing_handler);

    /*
    if (ccnl_proc)
	remove_proc_entry(PROCFS_NAME, NULL);
    */

    ccnl_lnxkernel_cleanup();

    for (j = 0; j < theRelay.ifcount; j++) {
	struct ccnl_if_s *i = &theRelay.ifs[j];

	if (i->wq) { // hmm, the work entries contain refs to i->sock?
	    flush_workqueue(i->wq);
	    destroy_workqueue(i->wq);
	    i->wq = NULL;
	}
    }
    theRelay.ifcount = 0;

    printk("%s: exit done\n", THIS_MODULE->name);
}

module_init(ccnl_init);
module_exit(ccnl_exit);

// eof
