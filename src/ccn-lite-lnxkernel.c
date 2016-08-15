/*
 * @f ccn-lite-lnxkernel.c
 * @b Linux kernel version of the CCN lite relay
 *
 * Copyright (C) 2012-13, Christian Tschudin, University of Basel
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
 * add GPL text here
 *
 * File history:
 * 2012-01-06 created
 */

#define CCNL_LINUXKERNEL

#define USE_DEBUG               // must select this for USE_MGMT
// #define USE_DEBUG_MALLOC
#define USE_DUP_CHECK
// #define USE_FRAG
#define USE_LINKLAYER
#define USE_LOGGING
#define USE_IPV4
#define USE_MGMT
#undef USE_NFN
#undef USE_NFN_MONITOR
// #define USE_SCHEDULER
// #define USE_SIGNATURES
#define USE_SUITE_CCNB          // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
#define USE_UNIXSOCKET

#define NEEDS_PREFIX_MATCHING

#include "ccnl-os-includes.h"
#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "ccnl-ext.h"

// ----------------------------------------------------------------------

#define assert(p) do{if(!p){DEBUGMSG(FATAL,"assertion violated %s:%d\n",__FILE__,__LINE__);}}while(0)

#define ccnl_app_RX(x,y)                do{}while(0)
#define local_producer(...)             0

static struct ccnl_relay_s theRelay;

static int ccnl_eth_RX(struct sk_buff *skb, struct net_device *indev,
                      struct packet_type *pt, struct net_device *outdev);

void ccnl_udp_data_ready(struct sock *sk);

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

// ----------------------------------------------------------------------

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

#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

static void ccnl_lnxkernel_cleanup(void);
char* ccnl_addr2ascii(sockunion *su);

void
ccnl_ll_TX(struct ccnl_relay_s *relay, struct ccnl_if_s *ifc,
            sockunion *dest, struct ccnl_buf_s *buf)
{
    DEBUGMSG(DEBUG, "ccnl_ll_TX for %d bytes ifc=%p sock=%p\n",
             buf ? buf->datalen : -1, ifc, ifc ? ifc->sock : NULL);

    if (!dest)
        return;

    if (in_interrupt()) {
        printk("** uh, must do ll_TX() during interrupt\n");
//      return;
    }

    switch (dest->sa.sa_family) {
    case AF_INET:
    {
        struct iovec iov;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
        struct iov_iter iov_iter;
#endif
        struct msghdr msg;
        int rc;
        mm_segment_t oldfs;

        iov.iov_base = buf->data;
        iov.iov_len = buf->datalen;

        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &dest->ip4;
        msg.msg_namelen = sizeof(dest->ip4);
        msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
#else
        iov_iter_init(&iov_iter, READ, &iov, 1, iov.iov_len);
        msg.msg_iter = iov_iter;
#endif

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        rc = sock_sendmsg(ifc->sock, &msg, buf->datalen);
        set_fs(oldfs);
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
        if (dev_hard_header(skb, dev, CCNL_ETH_TYPE, dest->linklayer.sll_addr,
                            ifc->addr.linklayer.sll_addr, skb->len))
            dev_queue_xmit(skb);
        else
            kfree_skb(skb);
        break;
    }
    case AF_UNIX:
    {
        struct iovec iov;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
        struct iov_iter iov_iter;
#endif
        struct msghdr msg;
        int rc;
        mm_segment_t oldfs;

        iov.iov_base = buf->data;
        iov.iov_len = buf->datalen;

        memset(&msg, 0, sizeof(msg));
        msg.msg_name = dest; //->ux.sun_path;
        msg.msg_namelen = offsetof(struct sockaddr_un, sun_path) +
            strlen(dest->ux.sun_path) + 1;
        msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
#else
        iov_iter_init(&iov_iter, READ, &iov, 1, iov.iov_len);
        msg.msg_iter = iov_iter;
#endif

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        rc = sock_sendmsg(ifc->sock, &msg, buf->datalen);
        set_fs(oldfs);

        //      printk("UNIX sendmsg returned %d\n", rc);
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

char*
ccnl_prefix_to_path(struct ccnl_prefix_s *pr)
{
    static char prefix_buf[4096];
    int len= 0, i;

    if (!pr)
        return NULL;
    for (i = 0; i < pr->compcnt; i++) {
        if(!strncmp("call", (char*)pr->comp[i], 4) && strncmp((char*)pr->comp[pr->compcnt-1], "NFN", 3))
            len += sprintf(prefix_buf + len, "%.*s", pr->complen[i], pr->comp[i]);
        else
            len += sprintf(prefix_buf + len, "/%.*s", pr->complen[i], pr->comp[i]);
    }
    prefix_buf[len] = '\0';
    return prefix_buf;
}

// ----------------------------------------------------------------------

#include "ccnl-core.c"

#include "ccnl-ext-frag.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-nfn.c"
#include "ccnl-ext-crypto.c"

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

struct ccnl_upcall_s {
    struct work_struct w;
    int ifndx;
    sockunion su;
    struct sk_buff *skb;
    void *data;
    unsigned int datalen;
};

void
ccnl_upcall_RX(struct work_struct *work)
{
    struct ccnl_upcall_s *uc = (struct ccnl_upcall_s*) work;

    DEBUGMSG(DEBUG, "ccnl_upcall_RX, ifndx=%d, %d bytes\n", uc->ifndx, uc->datalen);

    ccnl_core_RX(&theRelay, uc->ifndx, uc->data, uc->datalen,
                 &uc->su.sa, sizeof(uc->su));

    kfree_skb(uc->skb);
    ccnl_free(uc);
}

void
ccnl_schedule_upcall_RX(int ifndx, sockunion *su, struct sk_buff *skb,
                        char *data, int datalen)
{
    struct ccnl_upcall_s *uc = ccnl_malloc(sizeof(struct ccnl_upcall_s));
    if (uc) {
        INIT_WORK((struct work_struct*)uc, ccnl_upcall_RX);
        uc->ifndx = ifndx;
        memcpy(&uc->su, su, sizeof(*su));
        uc->skb = skb;
        uc->data = data;
        uc->datalen = datalen;
        schedule_work((struct work_struct*)uc);
    } else
        kfree_skb(skb);
}

// ----------------------------------------------------------------------

static int
ccnl_eth_RX(struct sk_buff *skb, struct net_device *indev,
          struct packet_type *pt, struct net_device *outdev){
    int i;
    sockunion su;

    for (i = 0; i < theRelay.ifcount; i++)
        if (theRelay.ifs[i].netdev == indev)
            break;
    if (!theRelay.halt_flag &&  i < theRelay.ifcount) {
        su.sa.sa_family = AF_PACKET;
        memcpy(su.linklayer.sll_addr, skb_mac_header(skb)+ETH_ALEN, ETH_ALEN);
        su.linklayer.sll_protocol = *((short int*)(skb_mac_header(skb)+2*ETH_ALEN));
        ccnl_schedule_upcall_RX(i, &su, skb, skb->data, skb->len);
    } else
        kfree_skb(skb);

    return 1;
}


void
ccnl_udp_data_ready(struct sock *sk)
{
    struct sk_buff *skb;
    int i, err;
    DEBUGMSG(DEBUG, "ccnl_udp_data_ready\n");

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
        DEBUGMSG(DEBUG, "ccnl_udp_data_ready2: if=%d, %d bytes, src=%s\n",
                 i, skb->len, ccnl_addr2ascii(&su));
        ccnl_schedule_upcall_RX(i, &su, skb, skb->data + sizeof(struct udphdr),
                                skb->len - sizeof(struct udphdr));
    } else
        kfree_skb(skb);
    return;
Bail:
    return;
}


void
ccnl_ux_data_ready(struct sock *sk)
{
    struct sk_buff *skb;
    int i, err;
    DEBUGMSG(DEBUG, "ccnl_ux_data_ready\n");

    if ((skb = skb_recv_datagram(sk, 0, 1, &err)) == NULL)
        goto Bail;
    for (i = 0; i < theRelay.ifcount; i++)
        if (theRelay.ifs[i].sock->sk == sk)
            break;
    if (!theRelay.halt_flag && i < theRelay.ifcount) {
        struct unix_sock *u = (struct unix_sock *)(skb->sk);
        sockunion su;
        if (u && u->addr)
            memcpy(&su, u->addr->name, u->addr->len);
        else
            su.sa.sa_family = 0;
        DEBUGMSG(DEBUG, "ccnl_ux_data_ready2: if=%d, %d bytes, src=%s\n",
                 i, skb->len, ccnl_addr2ascii(&su));

        ccnl_schedule_upcall_RX(i, &su, skb, skb->data, skb->len);
    } else
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
ccnl_open_ethdev(char *devname, struct sockaddr_ll *sll, int ethtype)
{
    struct net_device *dev;

    DEBUGMSG(DEBUG, "ccnl_open_ethdev %s\n", devname);

    dev = dev_get_by_name(&init_net, devname);
    if (!dev)
        return NULL;

    sll->sll_family = AF_PACKET;
    sll->sll_protocol = htons(ethtype);
    memcpy(sll->sll_addr, dev->dev_addr, ETH_ALEN);
    sll->sll_ifindex = dev->ifindex;

    DEBUGMSG(INFO, "access to %s with MAC=%s installed\n",
             devname, ll2ascii(sll->sll_addr, sll->sll_halen));
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
        DEBUGMSG(ERROR, "Error %d creating UDP socket\n", rc);
        return NULL;
    }

    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = htonl(INADDR_ANY);
    sin->sin_port = htons(port);
    rc = s->ops->bind(s, (struct sockaddr*) sin, sizeof(*sin));
    if (rc < 0) {
        DEBUGMSG(ERROR, "Error %d binding UDP socket (port %d)\n", rc, port);
        goto Bail;
    }
    DEBUGMSG(INFO, "UDP port %d opened, socket is %p\n", port, s);

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

    DEBUGMSG(DEBUG, "ccnl_open_unixpath %s\n", path);

    rc = sock_create(AF_UNIX, SOCK_DGRAM, 0, &s);
    if (rc < 0) {
        DEBUGMSG(ERROR, "Error %d creating UNIX socket %s\n", rc, path);
        return NULL;
    }
    DEBUGMSG(DEBUG, "UNIX socket is %p\n", (void*)s);

    ux->sun_family = AF_UNIX;
    strcpy(ux->sun_path, path);
    rc = s->ops->bind(s, (struct sockaddr*) ux,
                offsetof(struct sockaddr_un, sun_path) + strlen(path) + 1);
    if (rc < 0) {
        DEBUGMSG(ERROR, "Error %d binding UNIX socket to %s "
                    "(remove first, check access rights)\n", rc, path);
        goto Bail;
    }

    return s;

Bail:
    sock_release(s);
    return NULL;
}

// ----------------------------------------------------------------------

static char *e = NULL;
static char *x = CCNL_DEFAULT_UNIXSOCKNAME;
static int c = CCNL_DEFAULT_MAX_CACHE_ENTRIES; // no memory by default
static int suite = CCNL_SUITE_DEFAULT;
static int u = 0; // u = CCN_UDP_PORT?
static char *v = NULL;
static char *p = NULL;
static char *k = NULL;
static char *s = NULL;
static void *ageing_handler = NULL;

module_param(c, int, 0);
MODULE_PARM_DESC(c, "max number of cache entries");

module_param(e, charp, 0);
MODULE_PARM_DESC(e, "name of ethernet device to serve");

module_param(k, charp, 0);
MODULE_PARM_DESC(k, "ctrl public key path");

module_param(p, charp, 0);
MODULE_PARM_DESC(p, "private key path");

module_param(s, charp, 0);
MODULE_PARM_DESC(s, "suite (ccnb, ccnx2015, cisco2015, iot2014, ndn2013)");

module_param(u, int, 0);
MODULE_PARM_DESC(u, "UDP port (default is 6363 for ndntlv, 9695 for ccnb)");

module_param(v, charp, 0);
MODULE_PARM_DESC(v, "verbosity level (fatal, error, warning, info, debug, verbose, trace)");

module_param(x, charp, 0);
MODULE_PARM_DESC(x, "name (path) of mgmt unix socket");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("christian.tschudin@unibas.ch");

static int __init
ccnl_init(void)
{
    struct ccnl_if_s *i;

    if (v) {
        debug_level = ccnl_debug_str2level(v);
    }
    if (s) {
        suite = ccnl_str2suite(s);
    }
    if (!ccnl_isSuite(suite)) {
        suite = CCNL_SUITE_DEFAULT;
    }

    DEBUGMSG(INFO, "This is %s\n", THIS_MODULE->name);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);
    DEBUGMSG(INFO, "using suite %s\n", ccnl_suite2str(suite));

    DEBUGMSG(DEBUG, "module parameters: c=%d, e=%s, k=%s, p=%s, s=%s, "
                 "u=%d, v=%s, x=%s\n",
             c, e, k, p, s, u, v, x);

    if (u <= 0)
        u = ccnl_suite2defaultPort(suite);

    ccnl_core_init();
    theRelay.max_cache_entries = c;
    theRelay.max_pit_entries = CCNL_DEFAULT_MAX_PIT_ENTRIES;
#ifdef USE_SCHEDULER
    theRelay.defaultFaceScheduler = ccnl_lnx_defaultFaceScheduler;
#endif

    ageing_handler = ccnl_set_timer(1000000, ccnl_ageing, &theRelay, 0);

#ifdef USE_UNIXSOCKET
    if (x) {
        i = &theRelay.ifs[theRelay.ifcount];
        i->sock = ccnl_open_unixpath(x, &i->addr.ux);
        if (i->sock) {
            DEBUGMSG(DEBUG, "ccnl_open_unixpath worked\n");
//      i->frag = CCNL_DGRAM_FRAG_ETH2011;
            i->mtu = 4048;
            i->reflect = 0;
            i->fwdalli = 0;
            write_lock_bh(&i->sock->sk->sk_callback_lock);
            i->old_data_ready = i->sock->sk->sk_data_ready;
            i->sock->sk->sk_data_ready = ccnl_ux_data_ready;
//      i->sock->sk->sk_user_data = &theRelay;
            write_unlock_bh(&i->sock->sk->sk_callback_lock);
            theRelay.ifcount++;
        }
    }
#endif

    if (u > 0) {
        i = &theRelay.ifs[theRelay.ifcount];
        i->sock = ccnl_open_udpdev(u, &i->addr.ip4);
        if (i->sock != NULL) {
            i->mtu = 64000;
            i->reflect = 0;
            i->fwdalli = 0;
            write_lock_bh(&i->sock->sk->sk_callback_lock);
            i->old_data_ready = i->sock->sk->sk_data_ready;
            i->sock->sk->sk_data_ready = ccnl_udp_data_ready;
            write_unlock_bh(&i->sock->sk->sk_callback_lock);
            theRelay.ifcount++;
        }
    }

#ifdef USE_LINKLAYER
    if (e) {
        i = &theRelay.ifs[theRelay.ifcount];
        i->netdev = ccnl_open_ethdev(e, &i->addr.linklayer, CCNL_ETH_TYPE);
        if (i->netdev) {
//      i->frag = CCNL_DGRAM_FRAG_ETH2011;
            i->mtu = 1500;
            i->reflect = 1;
            i->fwdalli = 1;
            i->ccnl_packet.type = htons(CCNL_ETH_TYPE);
            i->ccnl_packet.dev = i->netdev;
            i->ccnl_packet.func = ccnl_eth_RX;
            dev_add_pack(&i->ccnl_packet);
            theRelay.ifcount++;
        }
    }
#endif
#ifdef USE_UNIXSOCKET
#ifdef USE_SIGNATURES
    if(p){
        char h[100];
        // Socket to Cryptoserver
        i = &theRelay.ifs[theRelay.ifcount];
        i->sock = ccnl_open_unixpath(p, &i->addr.ux);
        if (i->sock) {
            DEBUGMSG(DEBUG, "ccnl_open_unixpath worked\n");
//      i->frag = CCNL_DGRAM_FRAG_ETH2011;
            i->mtu = 4048;
            i->reflect = 0;
            i->fwdalli = 0;
            write_lock_bh(&i->sock->sk->sk_callback_lock);
            i->old_data_ready = i->sock->sk->sk_data_ready;
            i->sock->sk->sk_data_ready = ccnl_ux_data_ready;
//      i->sock->sk->sk_user_data = &theRelay;
            write_unlock_bh(&i->sock->sk->sk_callback_lock);
            theRelay.ifcount++;
        }
        ccnl_crypto_create_ccnl_crypto_face(&theRelay, p);
        theRelay.crypto_path = p;
        //Reply socket
        i = &theRelay.ifs[theRelay.ifcount];
        sprintf(h, "%s-2", p);
        i->sock = ccnl_open_unixpath(h, &i->addr.ux);
        if (i->sock) {
            DEBUGMSG(DEBUG, "ccnl_open_unixpath worked\n");
//      i->frag = CCNL_DGRAM_FRAG_ETH2011;
            i->mtu = 4048;
            i->reflect = 0;
            i->fwdalli = 0;
            write_lock_bh(&i->sock->sk->sk_callback_lock);
            i->old_data_ready = i->sock->sk->sk_data_ready;
            i->sock->sk->sk_data_ready = ccnl_ux_data_ready;
//      i->sock->sk->sk_user_data = &theRelay;
            write_unlock_bh(&i->sock->sk->sk_callback_lock);
            theRelay.ifcount++;
        }
        ccnl_crypto_create_ccnl_crypto_face(&theRelay, p);
        theRelay.crypto_path = p;
    }
#endif /*USE_SIGNATURES*/
#endif //USE_UNIXSOCKET

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
            if (i->addr.sa.sa_family == AF_UNIX ||
                i->addr.sa.sa_family == AF_INET) {
                write_lock_bh(&i->sock->sk->sk_callback_lock);
                i->sock->sk->sk_data_ready = i->old_data_ready;
                write_unlock_bh(&i->sock->sk->sk_callback_lock);
            }
            sock_release(i->sock);
            i->sock = 0;
        }
    }
    theRelay.ifcount = 0;

    if (x) { // also remove the UNIX socket path
        struct path p;
        int rc;

        rc = kern_path(x, 0, &p);
        if (!rc) {
            struct dentry *dir = dget_parent(p.dentry);

            mutex_lock_nested(&(dir->d_inode->i_mutex), I_MUTEX_PARENT);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
            rc = vfs_unlink(dir->d_inode, p.dentry, NULL);
#else
            rc = vfs_unlink(dir->d_inode, p.dentry);
#endif
            mutex_unlock(&dir->d_inode->i_mutex);
            dput(dir);
            path_put(&p);
        }
    }

    if (p) { // also remove the UNIX socket path
        struct path px;
        int rc;

        rc = kern_path(x, 0, &px);
        if (!rc) {
            struct dentry *dir = dget_parent(px.dentry);

            mutex_lock_nested(&(dir->d_inode->i_mutex), I_MUTEX_PARENT);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
            rc = vfs_unlink(dir->d_inode, px.dentry, NULL);
#else
            rc = vfs_unlink(dir->d_inode, px.dentry);
#endif
            mutex_unlock(&dir->d_inode->i_mutex);
            dput(dir);
            path_put(&px);
        }
    }
    x = NULL;
}

static void __exit
ccnl_exit( void )
{
    DEBUGMSG(INFO, "%s: exit\n", THIS_MODULE->name);

    if (ageing_handler)
        ccnl_rem_timer(ageing_handler);

    flush_scheduled_work();
    ccnl_lnxkernel_cleanup();

    DEBUGMSG(INFO, "%s: exit done\n", THIS_MODULE->name);
}

module_init(ccnl_init);
module_exit(ccnl_exit);

// eof
