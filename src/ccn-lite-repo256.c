/*
 * @f ccn-lite-repo256.c
 * @b user space repo, access via SHA256 digest as well as exact name match
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-11-26 created
 */


/*

OKset = hash table (set) of verified file content
NMmap = hash table (map) of verified names, pointing to the hash value
ERset = hash table (set) of files know to have wrong name (for the hash val)
HSset = hash table (set) of learned content hashes

File system structure:

  <dirpath>
  <dirpath>/UV/XY/<60 hex digits>
           where UVXY are the four topmost hex digits of the file's digest

Todo:

a) put the loading of file content as back ground task
   in order to increase startup time.

b) longest prefix match (and use of balanced tree?)

c) refactor with ccn-lite-relay.c code

d) enable content store caching, or rely on OS paging?

*/

#ifndef _BSD_SOURCE
# define _BSD_SOURCE // because of DT_DIR etc
#endif

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#define CCNL_UNIX

// #define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
// #define USE_SUITE_CISTLV
// #define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
// #define USE_SUITE_LOCALRPC

#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
// #define USE_ETHERNET
#define USE_FLIC
#define USE_HMAC256
#define USE_IPV4
//#define USE_IPV6
#define USE_NAMELESS
#define USE_SHA256
#define USE_UNIXSOCKET

#undef USE_NFN

#define NEEDS_PREFIX_MATCHING

#include "ccnl-os-includes.h"
#include "ccnl-defs.h"
#include "ccnl-core.h"

#include "lib-khash.h"
#include "lib-sha256.c"

#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-os-time.c"
#include "ccnl-ext-logging.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define local_producer(...)             0

#include "ccnl-core.c"

// ----------------------------------------------------------------------

char fnbuf[512];
struct ccnl_relay_s theRepo;
char *theRepoDir;
#ifdef USE_LOGGING
  char prefixBuf[CCNL_PREFIX_BUFSIZE];
#endif
unsigned char iobuf[64*2014];

enum {
    MODE_LAZY,    // for each interest, visit directly the file system
    MODE_INDEX     // build an internal index, check there before read()
};
int repo_mode = MODE_LAZY;

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
    khint_t *ip = (khint_t*) (md+1), val = *md ^ *ip++;
    int i;
    for (i = SHA256_DIGEST_LENGTH / sizeof(khint_t); i > 1; i--)
        val ^= *ip++;

    return val;
}

#define sha256equal(a, b) (!memcmp(a, b, SHA256_DIGEST_LENGTH+1))
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

KHASH_INIT(256set, kh256_t, char, 0, sha256toInt, sha256equal)
khash_t(256set) *OKset;
khash_t(256set) *ERset;
//khash_t(256set) *NOset;

//KHASH_INIT(256map, kh256_t, unsigned char*, 1, sha256toInt, sha256equal)
khash_t(256set) *HSset;

KHASH_INIT(PFX, khPFX_t, unsigned char*, 1, khPFXtoInt, khPFXequal)
khash_t(PFX) *NMmap;

// ----------------------------------------------------------------------

void
assertDir(char *dirpath, char *lev1, char *lev2)
{
    snprintf(fnbuf, sizeof(fnbuf), "%s/%c%c", dirpath, lev1[0], lev1[1]);
    if (mkdir(fnbuf, 0777) && errno != EEXIST) {
        DEBUGMSG(FATAL, "could not create directory %s\n", fnbuf);
        exit(-1);
    }

    if (!lev2)
        return;
    snprintf(fnbuf, sizeof(fnbuf), "%s/%c%c/%c%c",
             dirpath, lev1[0], lev1[1], lev2[0], lev2[1]);
    if (mkdir(fnbuf, 0777) && errno != EEXIST) {
        DEBUGMSG(FATAL, "could not create directory %s\n", fnbuf);
        exit(-1);
    }
}

static char*
digest2str(unsigned char *md)
{
    static char tmp[80];
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(tmp + 2*i, "%02x", md[i]);
    return tmp;
}

void
str2digest(unsigned char *md, char *s)
{
    int i;
    for (i = 0; i < 32; i++, s += 2)
        *md++ = hex2int(s[0]) * 16 + hex2int(s[1]);
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
    char *hex = digest2str(md);

    snprintf(fnbuf, sizeof(fnbuf), "%s/%02x/%02x/%s", dirpath,
             md[0], md[1], hex+4);

    return fnbuf;
}

unsigned char*
digest2key(int suite, unsigned char *digest)
{
    static unsigned char md[SHA256_DIGEST_LENGTH + 1];
    md[0] = suite;
    memcpy(md + 1, digest, SHA256_DIGEST_LENGTH);
    return md;
}

unsigned char*
addTo256set(khash_t(256set) *table, int suite, unsigned char *md,
            unsigned char *existingKey)
{
    khint_t k;
    int absent;
    unsigned char *key;

    key = digest2key(suite, md);
    k  = kh_put(256set, table, key, &absent);
    if (absent) {
        if (!existingKey) {
            existingKey = ccnl_malloc(SHA256_DIGEST_LENGTH+1);
            memcpy(existingKey, key, SHA256_DIGEST_LENGTH+1);
        }
        kh_key(table, k) = existingKey;
    }
    return kh_key(table, k);
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

int
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
    return close(s);
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

    DEBUGMSG(INFO, "configuring repo in '%s mode'\n",
             repo_mode == MODE_LAZY ? "lazy" : "index");

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

struct ccnl_pkt_s*
array2packet(unsigned char *buf, int datalen)
{
    int skip, suite;
    struct ccnl_pkt_s *pkt = NULL;
    unsigned char *data;

    suite = ccnl_pkt2suite(buf, datalen, &skip);
    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB: {
        unsigned char *start;

        data = start = buf + skip;
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

        data = start = buf + skip;
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

        data = start = buf + skip;
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

        data = olddata = buf + skip;
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

        data = olddata = buf + skip;
        datalen -= skip;
        pkt = ccnl_ndntlv_bytes2pkt(olddata, &data, &datalen);
        break;
    }
#endif
    default:
        break;
    }

    if (!pkt) {
        DEBUGMSG(DEBUG, "  parsing error\n");
    }
    return pkt;
}

struct ccnl_pkt_s*
contentFile2packet(char *path)
{
    int datalen;

    DEBUGMSG(DEBUG, "loading(%s)\n", path);

    datalen = file2iobuf(path);
    if (datalen <= 0)
        return NULL;

    return array2packet(iobuf, datalen);
}

// ----------------------------------------------------------------------

int
ccnl_repo_replyWithFile(struct ccnl_relay_s *repo, struct ccnl_face_s *from,
                        int suite, char *fname, unsigned char *cmpDigest)
{
    int f, len;
    struct ccnl_buf_s *buf;
    struct stat s;

    if (stat(fname, &s))
        goto AddToER;
    f = open(fname, O_RDONLY);
    if (f < 0)
        goto AddToER;

    buf = ccnl_buf_new(NULL, s.st_size);
    buf->datalen = s.st_size;
    len = read(f, buf->data, buf->datalen);
    close(f);

    if (len != s.st_size) {
        ccnl_free(buf);
        goto AddToER;
    }

    if (cmpDigest) { // verify packet digest
        struct ccnl_pkt_s *pkt = array2packet(buf->data, buf->datalen);

        if (!pkt || memcmp(pkt->md, cmpDigest, SHA256_DIGEST_LENGTH)) {
            ccnl_free(buf);
            buf = NULL;
        }
        free_packet(pkt);
        if (!buf)
            goto AddToER;

        addTo256set(OKset, pkt->suite, pkt->md, NULL);
    }

    ccnl_face_enqueue(repo, from, buf);

    return 0;

AddToER:
    {
        int absent = 0;
        unsigned char *key = digest2key(suite, cmpDigest);
        khint_t k  = kh_put(256set, ERset, key, &absent);
        if (absent) {
            unsigned char *key2 = ccnl_malloc(SHA256_DIGEST_LENGTH+1);
            memcpy(key2, key, SHA256_DIGEST_LENGTH+1);
            kh_key(ERset, k) = key2;
        }
        k = kh_get(256set, OKset, key);
        if (k != kh_end(OKset)) {
            ccnl_free(kh_key(OKset, k));
            kh_del(256set, OKset, k);
        }
    }
    return 0;
}

int
ccnl_repo256(struct ccnl_relay_s *repo, struct ccnl_face_s *from,
             int suite, int skip, unsigned char **data, int *datalen)
{
    int rc = -1;
    unsigned char *start, *requestByDigest = NULL, *digest = NULL;
    struct ccnl_pkt_s *pkt = NULL;
    khint_t k;
    char *fname;

    DEBUGMSG(DEBUG, "ccnl_repo (suite=%s, skip=%d, %d bytes left)\n",
             ccnl_suite2str(suite), skip, *datalen);

    *data += skip;
    *datalen -=  skip;
    start = *data;

    switch(suite) {
    case CCNL_SUITE_CCNTLV: {
        int hdrlen = ccnl_ccntlv_getHdrLen(*data, *datalen);
        *data += hdrlen;
        *datalen -= hdrlen;
        pkt = ccnl_ccntlv_bytes2pkt(start, data, datalen);
        if (!pkt || pkt->contentType != CCNX_TLV_TL_Interest)
            goto Done;
        requestByDigest = pkt->s.ccntlv.objHashRestr;
        break;
    }
    case CCNL_SUITE_NDNTLV:
        pkt = ccnl_ndntlv_bytes2pkt(start, data, datalen);
        if (!pkt || pkt->packetType != NDN_TLV_Interest)
            goto Done;
        requestByDigest = pkt->s.ndntlv.dataHashRestr;
        break;
    default:
        goto Done;
    }

    if (!pkt) {
        DEBUGMSG(INFO, "  packet decoding problem\n");
        goto Done;
    }

    if (!requestByDigest) {
        khPFX_t n;
        int i;

        // try first content-addressed lookup
        i = pkt->pfx->compcnt - 1;
        if (i >= 0 && pkt->pfx->complen[i] == 71 &&
                                !memcmp(pkt->pfx->comp[i], "sha256;", 7)) {
            unsigned char key[SHA256_DIGEST_LENGTH + 1];

            DEBUGMSG(DEBUG, "lookup by content hash name [%s]%s\n",
                     ccnl_prefix2path(prefixBuf,
                                      CCNL_ARRAY_SIZE(prefixBuf),
                                      pkt->pfx),
                     ccnl_suite2str(suite));
            key[0] = suite;
            str2digest(key + 1, (char*) pkt->pfx->comp[i]);
            if (repo_mode == MODE_INDEX) {
                k = kh_get(256set, HSset, key);
                if (k == kh_end(HSset)) {
                    DEBUGMSG(DEBUG, "  not found in HSset\n");
                    goto Done;
                }
            }

            snprintf(fnbuf, sizeof(fnbuf), "%s/hs/%s",
                     theRepoDir, digest2str(key+1));
            rc = ccnl_repo_replyWithFile(repo, from, suite, fnbuf, NULL);
            goto Done;
        }

        DEBUGMSG(DEBUG, "lookup by name [%s]%s, %p/%d\n",
                 ccnl_prefix2path(prefixBuf,
                                  CCNL_ARRAY_SIZE(prefixBuf),
                                  pkt->pfx),
                 ccnl_suite2str(suite),
                 pkt->pfx->nameptr, (int)(pkt->pfx->namelen));

        n = ccnl_malloc(sizeof(struct khPFX_s) + pkt->pfx->namelen);
        n->len = 1 + pkt->pfx->namelen;
        n->mem[0] = pkt->pfx->suite;
        memcpy(n->mem + 1, pkt->pfx->nameptr, pkt->pfx->namelen);
        k = kh_get(PFX, NMmap, n);
        ccnl_free(n);
        if (k == kh_end(NMmap)) {
            DEBUGMSG(DEBUG, "  no NMmap entry\n");
            goto Done;
        }
        DEBUGMSG(DEBUG, "  found NMmap entry at position %d\n", k);
        digest = kh_val(NMmap, k) + 1;
    } else
        digest = requestByDigest;
    
    DEBUGMSG(DEBUG, "lookup %s\n", digest2str(digest));

    k = kh_get(256set, ERset, digest2key(suite, digest));
    if (k != kh_end(ERset)) { // bad hash value in file
        DEBUGMSG(DEBUG, "  ERset hit - request discarded\n");
        goto Done;
    }
    
    k = kh_get(256set, OKset, digest2key(suite, digest));
    if (k != kh_end(OKset)) {
        DEBUGMSG(DEBUG, "  found OKset entry at position %d\n", k);

        fname = digest2fname(theRepoDir, digest);
        rc = ccnl_repo_replyWithFile(repo, from, suite, fname, NULL);
        goto Done;
    }
    if (repo_mode == MODE_INDEX) // if not in OKset then it does not exist
        goto Done;

/*
            k = kh_get(256set, NOset, key);
            if (k != kh_end(NOset)) { // known to be absent
                DEBUGMSG(DEBUG, "  NOset hit - request discarded\n");
                goto Done;
            }
*/
    fname = digest2fname(theRepoDir, digest);
    rc = ccnl_repo_replyWithFile(repo, from, suite, fname, digest);

Done:
    free_packet(pkt);
    return rc;
}

void
ccnl_repo_RX(struct ccnl_relay_s *repo, int ifndx, unsigned char *data,
             int datalen, struct sockaddr *sa, int addrlen)
{
    unsigned char *base = data;
    struct ccnl_face_s *from;
    int enc, suite = -1, skip;

    (void) base; // silence compiler warning (if USE_DEBUG is not set)

    DEBUGMSG(DEBUG, "ccnl_repo_RX ifndx=%d, %d bytes\n", ifndx, datalen);
    //    DEBUGMSG_ON(DEBUG, "ccnl_core_RX ifndx=%d, %d bytes\n", ifndx, datalen);

#ifdef USE_STATS
    if (ifndx >= 0)
        repo->ifs[ifndx].rx_cnt++;
#endif

    from = ccnl_get_face_or_create(repo, ifndx, sa, addrlen);
    if (!from) {
        DEBUGMSG(DEBUG, "  no face\n");
        return;
    } else {
        DEBUGMSG(DEBUG, "  face %d, peer=%s\n", from->faceid,
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
            DEBUGMSG(WARNING, "?unknown packet format? ccnl_core_RX ifndx=%d, %d bytes starting with 0x%02x at offset %zd\n",
                     ifndx, datalen, *data, data - base);
            return;
        } else if (ccnl_repo256(repo, from, suite, skip, &data, &datalen) < 0)
            break;
        if (datalen > 0) {
            DEBUGMSG(WARNING, "ccnl_core_RX: %d bytes left\n", datalen);
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

void
addNamed(char *fname, struct ccnl_pkt_s *pkt) // , unsigned char *key)
{
    khPFX_t n;
    int absent = 0;
    khint_t k;
    unsigned char *key;

    DEBUGMSG(DEBUG, "addNamed(%s)\n", fname);
    key = addTo256set(OKset, pkt->suite, pkt->md, NULL);

    if (pkt->content) {
        unsigned char md[SHA256_DIGEST_LENGTH];
        ccnl_SHA256(pkt->content, pkt->contlen, md);
        addTo256set(HSset, pkt->suite, md, NULL);
    }

    if (!pkt->pfx) {
        DEBUGMSG(DEBUG, "  no name\n");
        return;
    }

    n = ccnl_malloc(sizeof(struct khPFX_s) + pkt->pfx->namelen);
    n->len = 1 + pkt->pfx->namelen;
    n->mem[0] = pkt->pfx->suite;
    memcpy(n->mem + 1, pkt->pfx->nameptr, pkt->pfx->namelen);
    absent = 0;
    k = kh_put(PFX, NMmap, n, &absent);
    if (absent) {
        DEBUGMSG(INFO, "  adding [%s]%s\n",
                 ccnl_prefix2path(prefixBuf,
                              CCNL_ARRAY_SIZE(prefixBuf), pkt->pfx),
                 ccnl_suite2str(pkt->pfx->suite));
        DEBUGMSG(VERBOSE, "  %s\n", digest2str(pkt->md));
        kh_key(NMmap, k) = n;
        kh_val(NMmap, k) = key;
    } else {
        DEBUGMSG(DEBUG, "  [%s]%s already seen, skipping\n",
                 ccnl_prefix2path(prefixBuf,
                              CCNL_ARRAY_SIZE(prefixBuf), pkt->pfx),
                 ccnl_suite2str(pkt->pfx->suite));
        ccnl_free(n);
    }
}

void
addHashed(char *fname, struct ccnl_pkt_s *pkt) // , unsigned char *key)
{
    unsigned char contentDigest[SHA256_DIGEST_LENGTH];

    DEBUGMSG(DEBUG, "addHashed(%s)\n", fname);
    addTo256set(OKset, pkt->suite, pkt->md, NULL);

    if (pkt->content) {
        ccnl_SHA256(pkt->content, pkt->contlen, contentDigest);
        DEBUGMSG(VERBOSE, "  %s\n", digest2str(contentDigest));
        addTo256set(HSset, pkt->suite, contentDigest, NULL);
    }

    str2digest(contentDigest, fname + strlen(fname) - 64);
    DEBUGMSG(VERBOSE, "  %s\n", digest2str(contentDigest));
    addTo256set(HSset, pkt->suite, contentDigest, NULL);
}

void
addVerified(char *fname, struct ccnl_pkt_s *pkt) // , unsigned char *key)
{
    char *path;

    DEBUGMSG(DEBUG, "addVerified\n");

    path = digest2fname(theRepoDir, pkt->md);
    if (strcmp(fname, path))
        return;
    
    addNamed(fname, pkt);
}

typedef void (reg_f)(char *, struct ccnl_pkt_s*); // , unsigned char*);

void
add_content(char *dirpath, char *fname, reg_f addToTables)
{
    struct ccnl_pkt_s *pkt = NULL;

    snprintf(fnbuf, sizeof(fnbuf), "%s/%s", dirpath, fname);
    DEBUGMSG(DEBUG, "add_content(%s)\n", fnbuf);
    pkt = contentFile2packet(fnbuf);

    if (!pkt) {
        DEBUGMSG(DEBUG, "no packet\n");
        return;
    }

    addToTables(fnbuf, pkt); // , digest2key(_suite, pkt->md));

    free_packet(pkt);
}

void
walk_fs(char *path, reg_f addToTables)
{
    DIR *dp;
    struct dirent *de;

    dp = opendir(path);
    if (!dp)
        return;
    while ((de = readdir(dp))) {
        switch(de->d_type) {
        case DT_REG:
            add_content(path, de->d_name, addToTables);
            break;
        case DT_LNK:
            if (repo_mode == MODE_LAZY)
                add_content(path, de->d_name, addToTables);
            break;
        case DT_DIR:
            if (de->d_name[0] != '.') {
                char *path2;
                snprintf(fnbuf, sizeof(fnbuf), "%s/%s", path, de->d_name);
                path2 = ccnl_strdup(fnbuf);
                walk_fs(path2, addToTables);
                ccnl_free(path2);
            }
            break;
        default:
            break;
        }
    }
    closedir(dp);
}

// ----------------------------------------------------------------------

void
flic_link(char *dirpath, char *linkdir,
             unsigned char *packetDigest, unsigned char *linkDigest)
{
    char *hex, lnktarget[100];

    assertDir(dirpath, linkdir, NULL);

    hex = digest2str(packetDigest);
    snprintf(lnktarget, sizeof(lnktarget), "../%02x/%02x/%s",
             packetDigest[0], packetDigest[1], hex+4);

    hex = digest2str(linkDigest);
    snprintf(fnbuf, sizeof(fnbuf), "%s/%s/%s", dirpath, linkdir, hex);
    
    symlink(lnktarget, fnbuf);
}
    
int
ccnl_repo256_import(char *dir)
{
    DIR *dp;
    struct dirent *de;

    dp = opendir(dir);
    if (!dp)
        return 0;
    while ((de = readdir(dp))) {
        char *path, *hashName;
        struct ccnl_pkt_s *pkt;

        snprintf(fnbuf, sizeof(fnbuf), "%s/%s", dir, de->d_name);
        path = ccnl_strdup(fnbuf);
        switch(de->d_type) {
        case DT_LNK:
        case DT_REG:
            pkt = contentFile2packet(path);
            if (!pkt) {
                DEBUGMSG(DEBUG, "  no packet?\n");
                break;
            }
            hashName = digest2fname(theRepoDir, pkt->md);
            if (access(hashName, F_OK)) { // no such file, create it
                int f;
                char *cp = digest2str(pkt->md);
                DEBUGMSG(DEBUG, "  creating %s\n", hashName);
                assertDir(theRepoDir, cp, cp + 2);
                f = open(hashName, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                write(f, pkt->buf->data, pkt->buf->datalen);
                close(f);
            }
            if (pkt->pfx) { // has name: add a symlink to the nm directory
                flic_link(theRepoDir, "nm", pkt->md, pkt->md);
            }
            if (pkt->content) { // has payload: add a symlink to the hs dir
                unsigned char md[SHA256_DIGEST_LENGTH];
                ccnl_SHA256(pkt->content, pkt->contlen, md);
                flic_link(theRepoDir, "hs", pkt->md, md);
            }
            free_packet(pkt);
            break;
        case DT_DIR:
            if (de->d_name[0] != '.')
                ccnl_repo256_import(path);
            break;
        default:
            break;
        }
        ccnl_free(path);
    }
    closedir(dp);

    return 0;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int opt, max_cache_entries = 0, udpport = 7777;
    char *ethdev = NULL, *uxpath = NULL, *import_path = NULL, *fname;

    while ((opt = getopt(argc, argv, "hc:d:e:i:m:u:v:x:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'e':
            ethdev = optarg;
            break;
        case 'i':
            import_path = optarg;
            break;
        case 'm':
            repo_mode = !strcmp(optarg, "index") ? MODE_INDEX : MODE_LAZY;
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
                    "usage: %s [options]  REPO_DIR                (server)\n"
                    "       %s -i IMPORT_DIR [options] REPO_DIR   (importing)\n"
                    "options:\n"
//                  "  -c MAX_CONTENT_ENTRIES  (dflt: 0)\n"
                    "  -e ETHDEV\n"
                    "  -h              this text\n"
                    "  -m MODE         ('lazy'=read through (dflt), 'index'=internal map\n"
#ifdef USE_IPV4
                    "  -u UDPPORT      (default: 7777)\n"
#endif

#ifdef USE_LOGGING
                    "  -v DEBUG_LEVEL  (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
#ifdef USE_UNIXSOCKET
                    "  -x UNIXPATH\n"
#endif
                    , argv[0], argv[0]);
            exit(-1);
        }
    }

    if (optind >= argc)
        goto usage;
    theRepoDir = argv[optind++];
    if (optind != argc)
        goto usage;
    while (strlen(theRepoDir) > 1 && theRepoDir[strlen(theRepoDir) - 1] == '/')
        theRepoDir[strlen(theRepoDir) - 1] = '\0';

    if (import_path)
        return ccnl_repo256_import(import_path);

    // ------------------------------------------------------------

    ccnl_core_init();

    DEBUGMSG(INFO, "This is ccn-lite-repo256, starting at %s",
             ctime(&theRepo.startup_time) + 4);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);

    ccnl_repo256_config(&theRepo, ethdev, udpport, uxpath, max_cache_entries);

    ERset = kh_init(256set);  // set of hashes for which the file is wrong
/*
    NOset = kh_init(256set);  // set of pkt+data hashes known to be absent
*/
    OKset = kh_init(256set);  // set of verified pkt hashes (from files)
    NMmap = kh_init(PFX);     // map of verified names (from files)
    HSset = kh_init(256set);  // set of data hashes (from files)

    snprintf(fnbuf, sizeof(fnbuf), "%s/hs", theRepoDir);
    fname = ccnl_strdup(fnbuf);
    DEBUGMSG(INFO, "loading hashed content from <%s>\n", fname);
    walk_fs(fname, addHashed);
    ccnl_free(fname);
    if (repo_mode == MODE_INDEX) {
        DEBUGMSG(INFO, "loading files from <%s>\n", theRepoDir);
        walk_fs(theRepoDir, addVerified);
    } else {
        snprintf(fnbuf, sizeof(fnbuf), "%s/nm", theRepoDir);
        fname = ccnl_strdup(fnbuf);
        DEBUGMSG(INFO, "loading named content from <%s>\n", fname);
        walk_fs(fname, addNamed);
        ccnl_free(fname);
    }

    DEBUGMSG(INFO, "Summary: %d files included\n", kh_size(OKset));
    DEBUGMSG(INFO, "  name entries        : %d\n", kh_size(NMmap));
    DEBUGMSG(INFO, "  content hash entries: %d\n", kh_size(HSset));
    DEBUGMSG(INFO, "  files with errors   : %d\n", kh_size(ERset));

    DEBUGMSG(DEBUG, "allocated memory: total %ld bytes in %d chunks\n",
             ccnl_total_alloc_bytes, ccnl_total_alloc_chunks);

    ccnl_io_loop(&theRepo);

/*
    {
        khiter_t k;

        for (k = kh_begin(OKset); k != kh_end(OKset); k++)
            if (kh_exist(OKset, k))
                ccnl_free(kh_key(OKset, k);

        for (k = kh_begin(NMmap); k != kh_end(NMmap); k++)
            if (kh_exist(NMmap, k))
                ccnl_free(kh_key(NMmap, k);
    }
*/
}

// eof
