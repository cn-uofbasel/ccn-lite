
// #include <search.h>

/*

oklist
fslist

load time:

  start background process

background:
  for each directory
    for each file in the tree:
      load(hashname)
      key = hash(content)
      tsearch(key, &OKbag, cmp)

request time - search by hash:
  fname = tfind(hashval, OKbag, cmp)
  if (fname) {
    load(fname)
    return content;
  }
  load(hashname)
  if (fails)
    return NULL;
  key = hash(content)
  tsearch(key, &OKbag, cmp)
  return content


request time - search by name:
  ...
*/

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "khash.h"

#define CCNL_UNIX

#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
// #define USE_ETHERNET
// #define USE_HMAC256
#define USE_IPV4
//#define USE_IPV6
#define USE_NAMELESS

// #define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
// #define USE_SUITE_CISTLV
// #define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
// #define USE_SUITE_LOCALRPC
#define USE_UNIXSOCKET
// #define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING

#include "ccnl-os-includes.h"
#include "ccnl-defs.h"
#include "ccnl-core.h"

#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define local_producer(...)             0

#include "ccnl-core.c"

struct ccnl_relay_s theRepo;
char *theDirPath;
#ifdef USE_LOGGING
  char prefixBuf[CCNL_PREFIX_BUFSIZE];
#endif
unsigned char iobuf[64*2014];

// ----------------------------------------------------------------------
// khash.h specifics

#undef kcalloc
#undef kmalloc
#undef krealloc
#undef kfree
#define kcalloc(N,Z)  ccnl_calloc(N,Z)
#define kmalloc(Z)    ccnl_malloc(Z)
#define krealloc(P,Z) ccnl_realloc(P,Z)
#define kfree(P)      ccnl_free(P)

khint_t
sha256toInt(unsigned char *md)
{
    khint_t *ip = (khint_t*) md, val = *ip++;
    int i;
    for (i = SHA256_DIGEST_LENGTH / sizeof(khint_t); i > 1; i--)
        val ^= *ip++;

    return val;
}

#define sha256equal(a, b) (!memcmp(a, b, SHA256_DIGEST_LENGTH))
typedef unsigned char* kh256_t;


struct khPFX_s {
    unsigned short len;
    unsigned char mem[1];
};
typedef struct khPFX_s* khPFX_t;

khint_t
khPFXtoInt(khPFX_t pfx)
{
    unsigned char *s = pfx->mem;
    int i = pfx->len;
    khint_t h = *s;

    while (--i > 0)
        h = (h << 5) - h + *++s;
    return h;
}

#define khPFXequal(a, b) ((a->len == b->len) && !memcmp(a->mem, b->mem, a->len))

KHASH_INIT(256, kh256_t, char, 0, sha256toInt, sha256equal)
khash_t(256) *OKlist;

KHASH_INIT(PFX, khPFX_t, unsigned char*, 1, khPFXtoInt, khPFXequal)
khash_t(PFX) *NMlist;

// ----------------------------------------------------------------------

static char*
digest2str(unsigned char *md)
{
    static char tmp[80];
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tmp + 2*i, "%02x", md[i]);
    return tmp;
}

int
file2iobuf(char *path)
{
    int f = open(path, O_RDONLY), len;
    if (f < 0)
        return -1;
    len = read(f, iobuf, sizeof(iobuf));
    close(f);

    return len;
}

char*
digest2fname(char *dirpath, unsigned char *md)
{
    char *cp, *hex = digest2str(md);

    asprintf(&cp, "%s/%02x/%s", dirpath, (unsigned) md[0], hex+2);

    return cp;
}

// ----------------------------------------------------------------------

#ifdef USE_UNIXSOCKET
int
ccnl_open_unixpath(char *path, struct sockaddr_un *ux)
{
    int sock, bufsize;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        return -1;
    }

    unlink(path);
    ccnl_setUnixSocketPath(ux, path);

    if (bind(sock, (struct sockaddr *) ux, sizeof(struct sockaddr_un))) {
        perror("binding name to datagram socket");
        close(sock);
        return -1;
    }

    bufsize = CCNL_MAX_SOCK_SPACE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;

}
#endif // USE_UNIXSOCKET

#ifdef USE_IPV4
int
ccnl_open_udpdev(int port, struct sockaddr_in *si)
{
    int s;
    unsigned int len;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        return -1;
    }

    si->sin_addr.s_addr = INADDR_ANY;
    si->sin_port = htons(port);
    si->sin_family = PF_INET;
    if(bind(s, (struct sockaddr *)si, sizeof(*si)) < 0) {
        perror("udp sock bind");
        return -1;
    }
    len = sizeof(*si);
    getsockname(s, (struct sockaddr*) si, &len);

    return s;
}
#elif defined(USE_IPV6)
int
ccnl_open_udpdev(int port, struct sockaddr_in6 *sin)
{
    int s;
    unsigned int len;

    s = socket(PF_INET6, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        return -1;
    }

    sin->sin6_addr = in6addr_any;
    sin->sin6_port = htons(port);
    sin->sin6_family = PF_INET6;
    if(bind(s, (struct sockaddr *)sin, sizeof(*sin)) < 0) {
        perror("udp sock bind");
        return -1;
    }
    len = sizeof(*sin);
    getsockname(s, (struct sockaddr*) sin, &len);

    return s;
}
#endif

void
ccnl_ll_TX(struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc,
           sockunion *dest, struct ccnl_buf_s *buf)
{
    int rc;

    switch(dest->sa.sa_family) {
#ifdef USE_IPV4
    case AF_INET:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ip4, sizeof(struct sockaddr_in));
        DEBUGMSG(DEBUG, "udp sendto(%d Bytes) to %s/%d returned %d/%d\n",
                 (int) buf->datalen,
                 inet_ntoa(dest->ip4.sin_addr), ntohs(dest->ip4.sin_port),
                 rc, errno);
        /*
        {
            int fd = open("t.bin", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            write(fd, buf->data, buf->datalen);
            close(fd);
        }
        */

        break;
#endif
#ifdef USE_ETHERNET
    case AF_PACKET:
        rc = ccnl_eth_sendto(ifc->sock,
                             dest->eth.sll_addr,
                             ifc->addr.eth.sll_addr,
                             buf->data, buf->datalen);
        DEBUGMSG(DEBUG, "eth_sendto %s returned %d\n",
                 eth2ascii(dest->eth.sll_addr), rc);
        break;
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        rc = sendto(ifc->sock,
                    buf->data, buf->datalen, 0,
                    (struct sockaddr*) &dest->ux, sizeof(struct sockaddr_un));
        DEBUGMSG(DEBUG, "unix sendto(%d Bytes) to %s returned %d\n",
                 (int) buf->datalen, dest->ux.sun_path, rc);
        break;
#endif
    default:
        DEBUGMSG(WARNING, "unknown transport\n");
        break;
    }
    (void) rc; // just to silence a compiler warning (if USE_DEBUG is not set)
}

void
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
    close(s);
}

// ----------------------------------------------------------------------

#ifdef USE_IPV4
void
ccnl_repo256_udp(struct ccnl_relay_s *relay, int port)
{
    struct ccnl_if_s *i;

    if (port < 0)
        return;
    i = &relay->ifs[relay->ifcount];
    i->sock = ccnl_open_udpdev(port, &i->addr.ip4);
    if (i->sock <= 0) {
        DEBUGMSG(WARNING, "sorry, could not open udp device (port %d)\n",
                 port);
        return;
    }

    relay->ifcount++;
    DEBUGMSG(INFO, "UDP interface (%s) configured\n",
             ccnl_addr2ascii(&i->addr));
    if (relay->defaultInterfaceScheduler)
        i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
}
#endif

void
ccnl_repo256_config(struct ccnl_relay_s *relay, char *ethdev, int udpport,
                    char *uxpath, int max_cache_entries)
{
#if defined(USE_ETHERNET) || defined(USE_UNIXSOCKET)
    struct ccnl_if_s *i;
#endif

    DEBUGMSG(INFO, "configuring relay\n");

    relay->max_cache_entries = max_cache_entries;
#ifdef USE_SCHEDULER
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
    relay->defaultInterfaceScheduler = ccnl_relay_defaultInterfaceScheduler;
#endif

#ifdef USE_ETHERNET
    // add (real) eth0 interface with index 0:
    if (ethdev) {
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_ethdev(ethdev, &i->addr.eth, CCNL_ETH_TYPE);
        i->mtu = 1500;
        i->reflect = 1;
        i->fwdalli = 1;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "ETH interface (%s %s) configured\n",
                     ethdev, ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
        } else
            DEBUGMSG(WARNING, "sorry, could not open eth device\n");
    }
#endif // USE_ETHERNET

#ifdef USE_IPV4
    ccnl_repo256_udp(relay, udpport);
#endif

#ifdef USE_UNIXSOCKET
    if (uxpath) {
        i = &relay->ifs[relay->ifcount];
        i->sock = ccnl_open_unixpath(uxpath, &i->addr.ux);
        i->mtu = 4096;
        if (i->sock >= 0) {
            relay->ifcount++;
            DEBUGMSG(INFO, "UNIX interface (%s) configured\n",
                     ccnl_addr2ascii(&i->addr));
            if (relay->defaultInterfaceScheduler)
                i->sched = relay->defaultInterfaceScheduler(relay,
                                                        ccnl_interface_CTS);
        } else
            DEBUGMSG(WARNING, "sorry, could not open unix datagram device\n");
    }
#endif // USE_UNIXSOCKET

//    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
}

// ----------------------------------------------------------------------

void
ccnl_repo_RX(struct ccnl_relay_s *relay, int ifndx, unsigned char *data,
             int datalen, struct sockaddr *sa, int addrlen)
{
    unsigned char *base = data;
    struct ccnl_face_s *from;
    int enc, suite = -1, skip;
    dispatchFct dispatch;

    (void) base; // silence compiler warning (if USE_DEBUG is not set)

    DEBUGMSG_CORE(DEBUG, "ccnl_repo_RX ifndx=%d, %d bytes\n", ifndx, datalen);
    //    DEBUGMSG_ON(DEBUG, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

#ifdef USE_STATS
    if (ifndx >= 0)
        relay->ifs[ifndx].rx_cnt++;
#endif

    from = ccnl_get_face_or_create(relay, ifndx, sa, addrlen);
    if (!from) {
        DEBUGMSG_CORE(DEBUG, "  no face\n");
        return;
    } else {
        DEBUGMSG_CORE(DEBUG, "  face %d, peer=%s\n", from->faceid,
                    ccnl_addr2ascii(&from->peer));
    }

    // loop through all packets in the received frame (UDP, Ethernet etc)
    while (datalen > 0) {
        // work through explicit code switching
        while (!ccnl_switch_dehead(&data, &datalen, &enc))
            suite = ccnl_enc2suite(enc);
        if (suite == -1)
            suite = ccnl_pkt2suite(data, datalen, &skip);

        if (!ccnl_isSuite(suite)) {
            DEBUGMSG_CORE(WARNING, "?unknown packet format? ccnl_core_RX ifndx=%d, %d bytes starting with 0x%02x at offset %zd\n",
                     ifndx, datalen, *data, data - base);
            return;
        }
        //        dispatch = ccnl_core_RX_dispatch[suite];
        dispatch = ccnl_core_suites[suite].RX;
        if (!dispatch) {
            DEBUGMSG_CORE(ERROR, "Forwarder not initialized or dispatcher "
                     "for suite %s does not exist.\n", ccnl_suite2str(suite));
            return;
        }
        if (dispatch(relay, from, &data, &datalen) < 0)
            break;
        if (datalen > 0) {
            DEBUGMSG_CORE(WARNING, "ccnl_core_RX: %d bytes left\n", datalen);
        }
    }
}

int
ccnl_io_loop(struct ccnl_relay_s *ccnl)
{
    int i, len, maxfd = -1, rc;
    fd_set readfs, writefs;
    unsigned char buf[CCNL_MAX_PACKET_SIZE];

    if (ccnl->ifcount == 0) {
        DEBUGMSG(ERROR, "no socket to work with, not good, quitting\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < ccnl->ifcount; i++)
        if (ccnl->ifs[i].sock > maxfd)
            maxfd = ccnl->ifs[i].sock;
    maxfd++;

    DEBUGMSG(INFO, "starting main event and IO loop\n");
    while (!ccnl->halt_flag) {
        int usec;

        FD_ZERO(&readfs);
        FD_ZERO(&writefs);

        for (i = 0; i < ccnl->ifcount; i++) {
            FD_SET(ccnl->ifs[i].sock, &readfs);
            if (ccnl->ifs[i].qlen > 0)
                FD_SET(ccnl->ifs[i].sock, &writefs);
        }

        usec = ccnl_run_events();
        if (usec >= 0) {
            struct timeval deadline;
            deadline.tv_sec = usec / 1000000;
            deadline.tv_usec = usec % 1000000;
            rc = select(maxfd, &readfs, &writefs, NULL, &deadline);
        } else
            rc = select(maxfd, &readfs, &writefs, NULL, NULL);

        if (rc < 0) {
            perror("select(): ");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < ccnl->ifcount; i++) {
            if (FD_ISSET(ccnl->ifs[i].sock, &readfs)) {
                sockunion src_addr;
                socklen_t addrlen = sizeof(sockunion);
                if ((len = recvfrom(ccnl->ifs[i].sock, buf, sizeof(buf), 0,
                                (struct sockaddr*) &src_addr, &addrlen)) > 0) {
                    if (0) {}
#ifdef USE_IPV4
                    else if (src_addr.sa.sa_family == AF_INET) {
                        ccnl_repo_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ip4));
                    }
#endif
#ifdef USE_ETHERNET
                    else if (src_addr.sa.sa_family == AF_PACKET) {
                        if (len > 14)
                            ccnl_repo_RX(ccnl, i, buf+14, len-14,
                                         &src_addr.sa, sizeof(src_addr.eth));
                    }
#endif
#ifdef USE_UNIXSOCKET
                    else if (src_addr.sa.sa_family == AF_UNIX) {
                        ccnl_repo_RX(ccnl, i, buf, len,
                                     &src_addr.sa, sizeof(src_addr.ux));
                    }
#endif
                }
            }

            if (FD_ISSET(ccnl->ifs[i].sock, &writefs)) {
              ccnl_interface_CTS(ccnl, ccnl->ifs + i);
            }
        }
    }

    return 0;
}

// ----------------------------------------------------------------------

int
ccnl_repo(struct ccnl_relay_s *repo, struct ccnl_face_s *from,
          unsigned char **data, int *datalen)
{
    int rc = -1;
    unsigned char *start = *data;
    struct ccnl_pkt_s *pkt;

    DEBUGMSG(DEBUG, "ccnl_repo (%d bytes left)\n", *datalen);

    pkt = ccnl_ndntlv_bytes2pkt(start, data, datalen);
    if (!pkt) {
        DEBUGMSG(INFO, "  ndntlv packet coding problem\n");
        goto Done;
    }
    if (pkt->type == NDN_TLV_Interest) {
        khint_t k;
        unsigned char *digest = NULL;

        if (pkt->s.ndntlv.dataHashRestr) {
            DEBUGMSG(DEBUG, "lookup %s\n",
                     digest2str(pkt->s.ndntlv.dataHashRestr));
            k = kh_get(256, OKlist, pkt->s.ndntlv.dataHashRestr); // ndx lookup
            if (k != kh_end(OKlist)) {
                DEBUGMSG(DEBUG, "  found OKlist entry at position %d\n", k);
                digest = pkt->s.ndntlv.dataHashRestr;
            }
        } else {
            khPFX_t n;

            DEBUGMSG(DEBUG, "lookup by name [%s]%s, %p/%d\n",
                     ccnl_prefix2path(prefixBuf,
                                      CCNL_ARRAY_SIZE(prefixBuf),
                                      pkt->pfx),
                     ccnl_suite2str(pkt->pfx->suite),
                     pkt->pfx->nameptr, (int)(pkt->pfx->namelen));
            
            n = ccnl_malloc(sizeof(struct khPFX_s) + pkt->pfx->namelen);
            n->len = 1 + pkt->pfx->namelen;
            n->mem[0] = pkt->pfx->suite;
            memcpy(n->mem + 1, pkt->pfx->nameptr, pkt->pfx->namelen);
            k = kh_get(PFX, NMlist, n);
            if (k != kh_end(NMlist)) {
                DEBUGMSG(DEBUG, "  found NMlist entry at position %d\n", k);
                digest = kh_val(NMlist, k);
            }
            ccnl_free(n);
        }

        if (digest) {
            char *path;
            struct stat s;
            int f;

            path = digest2fname(theDirPath, digest);
            if (stat(path, &s)) {
                perror("stat");
                free(path);
                goto Done;
            }
            f = open(path, O_RDONLY);
            free(path);
            if (f >= 0) {
                struct ccnl_buf_s *buf = ccnl_buf_new(NULL, s.st_size);
                buf->datalen = s.st_size;
                read(f, buf->data, buf->datalen);
                ccnl_face_enqueue(repo, from, buf);
            }
            close(f);
        }
    }
    rc = 0;
Done:
    free_packet(pkt);
    return rc;
}

// ----------------------------------------------------------------------

int loadcnt;

void
add_content(char *dirpath, char *path)
{
    int datalen, skip, _suite;
    struct ccnl_pkt_s *pkt = NULL;
    unsigned char *data;

    DEBUGMSG(DEBUG, "loading %s\n", path);

    datalen = file2iobuf(path);
    if (datalen <= 0)
        return;

    _suite = ccnl_pkt2suite(iobuf, datalen, &skip);
    switch (_suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB: {
            unsigned char *start;

            data = start = iobuf + skip;
            datalen -= skip;

            if (data[0] != 0x04 || data[1] != 0x82)
                goto notacontent;
            data += 2;
            datalen -= 2;

            pkt = ccnl_ccnb_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
            int hdrlen;
            unsigned char *start;

            data = start = iobuf + skip;
            datalen -=  skip;

            hdrlen = ccnl_ccntlv_getHdrLen(data, datalen);
            data += hdrlen;
            datalen -= hdrlen;

            pkt = ccnl_ccntlv_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_CISTLV
        case CCNL_SUITE_CISTLV: {
            int hdrlen;
            unsigned char *start;

            data = start = iobuf + skip;
            datalen -=  skip;

            hdrlen = ccnl_cistlv_getHdrLen(data, datalen);
            data += hdrlen;
            datalen -= hdrlen;

            pkt = ccnl_cistlv_bytes2pkt(start, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV: {
            unsigned char *olddata;

            data = olddata = iobuf + skip;
            datalen -= skip;
            if (ccnl_iottlv_dehead(&data, &datalen, &typ, &len) ||
                                                       typ != IOT_TLV_Reply)
                goto notacontent;
            pkt = ccnl_iottlv_bytes2pkt(typ, olddata, &data, &datalen);
            break;
        }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV: {
            unsigned char *olddata;

            data = olddata = iobuf + skip;
            datalen -= skip;
            pkt = ccnl_ndntlv_bytes2pkt(olddata, &data, &datalen);
            break;
        }
#endif
        default:
            DEBUGMSG(WARNING, "unknown packet format (%s)\n", path);
            break;
        }

        if (!pkt) {
            DEBUGMSG(DEBUG, "  parsing error in %s\n", path);
        } else {
            char *path2;

            path2 = digest2fname(dirpath, pkt->md);
            if (strcmp(path2, path)) {
                DEBUGMSG(WARNING, "wrong digest for file %s, ignored\n", path);
            } else {
                unsigned char *md;
                int absent = 0;
                khint_t k = kh_put(256, OKlist, pkt->md, &absent);

                if (absent) {
                    md = ccnl_malloc(SHA256_DIGEST_LENGTH);
                    memcpy(md, pkt->md, SHA256_DIGEST_LENGTH);
                    kh_key(OKlist, k) = md;
                } else
                    md = kh_key(OKlist, k);

                if (pkt->pfx) {
                    khPFX_t n;
                    DEBUGMSG(DEBUG, "pkt has name [%s]%s, %p/%d\n",
                             ccnl_prefix2path(prefixBuf,
                                              CCNL_ARRAY_SIZE(prefixBuf),
                                              pkt->pfx),
                             ccnl_suite2str(pkt->pfx->suite),
                             pkt->pfx->nameptr, (int)(pkt->pfx->namelen));
                    n = ccnl_malloc(sizeof(struct khPFX_s) + pkt->pfx->namelen);
                    n->len = 1 + pkt->pfx->namelen;
                    n->mem[0] = pkt->pfx->suite;
                    memcpy(n->mem + 1, pkt->pfx->nameptr, pkt->pfx->namelen);
                    absent = 0;
                    k = kh_put(PFX, NMlist, n, &absent);
                    if (absent) {
                        kh_key(NMlist, k) = n;
                        kh_val(NMlist, k) = md;
                    } else
                        ccnl_free(n);
                }
            }
            free(path2);
            free_packet(pkt);
        }
}

void
walk_fs(char *dirpath, char *path)
{
    DIR *dp;
    struct dirent *de;

    dp = opendir(path);
    if (!dp)
        return;
    while ((de = readdir(dp))) {
        if (de->d_type == DT_REG ||
                           (de->d_type == DT_DIR && de->d_name[0] != '.')) {
            char *path2;
            asprintf(&path2, "%s/%s", path, de->d_name);
            if (de->d_type == DT_REG)
                add_content(dirpath, path2);
            else
                walk_fs(dirpath, path2);
            free(path2);
        }
    }
    closedir(dp);
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt, max_cache_entries = 0, inter_load_gap = 10, udpport = 7777;
    char *ethdev = NULL, *uxpath = NULL;

    while ((opt = getopt(argc, argv, "hc:d:e:g:m:u:v:x:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'e':
            ethdev = optarg;
            break;
        case 'g':
            inter_load_gap = atoi(optarg);
            break;
        case 'u':
            udpport = atoi(optarg);
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'x':
            uxpath = optarg;
            break;
        case 'h':
        default:
usage:
            fprintf(stderr,
                    "usage: %s [options] DIRPATH\n"
//                  "  -c MAX_CONTENT_ENTRIES  (dflt: 0)\n"
//                  "  -e ETHDEV\n"
                    "  -g GAP          (msec between loads)\n"
                    "  -h\n"
//                  "  -m MODE         (0=read through, 1=ndx from file, 2=ndx from content (dflt)\n"
#ifdef USE_IPV4
                    "  -u UDPPORT      (default: 7777)\n"
#endif

#ifdef USE_LOGGING
                    "  -v DEBUG_LEVEL  (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
#ifdef USE_UNIXSOCKET
                    "  -x UNIXPATH\n"
#endif
                    , argv[0]);
            exit(-1);
        }
    }

    if (optind >= argc)
        goto usage;
    theDirPath = argv[optind++];
    if (optind != argc)
        goto usage;

    ccnl_core_init();
    ccnl_core_suites[CCNL_SUITE_NDNTLV].RX       = ccnl_repo;

    DEBUGMSG(INFO, "This is ccn-lite-repo256, starting at %s",
             ctime(&theRepo.startup_time) + 4);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);

    ccnl_repo256_config(&theRepo, NULL, udpport, uxpath, max_cache_entries);

    OKlist = kh_init(256);
    NMlist = kh_init(PFX);

    while (strlen(theDirPath) > 1 && theDirPath[strlen(theDirPath) - 1] == '/')
        theDirPath[strlen(theDirPath) - 1] = '\0';
    walk_fs(theDirPath, theDirPath);

    ccnl_io_loop(&theRepo);

    {
        DEBUGMSG(DEBUG, "khash OKlist: size %d, buckets %d\n",
                 kh_size(OKlist), kh_n_buckets(OKlist));
        DEBUGMSG(DEBUG, "khash NMlist: size %d, buckets %d\n",
                 kh_size(NMlist), kh_n_buckets(NMlist));

/*
        khiter_t k;
        int cnt = 0;

        for (k = kh_begin(OKlist); k != kh_end(OKlist); k++)
            if (kh_exist(OKlist, k))
                printf("%d %s\n", cnt++, digest2str(kh_key(OKlist, k)));
*/
    }

    DEBUGMSG(DEBUG, "done (mem alloc: %ld bytes, %d chunks)\n",
             ccnl_total_alloc_bytes, ccnl_total_alloc_chunks);
}

// eof
