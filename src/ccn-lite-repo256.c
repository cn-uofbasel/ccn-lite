
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
#define ccnl_close_socket(...)          do{}while(0)
#define ccnl_ll_TX(x,...)               do{(x);}while(0)
#define local_producer(...)             0

#include "ccnl-core.c"

unsigned char iobuf[64*2014];

// ----------------------------------------------------------------------
// khash.h specifics

khint_t
sha256toInt(unsigned char *md)
{
    khint_t *ip = (khint_t*) md, val = *ip++;
    int i;
    for (i = SHA256_DIGEST_LENGTH / sizeof(khint_t); i > 1; i--)
        val ^= *ip++;

    return val;
}

#define sha256cmp(a, b) (!memcmp(a, b, SHA256_DIGEST_LENGTH))
typedef unsigned char* khint256_t;

#undef kcalloc
#undef kmalloc
#undef krealloc
#undef kfree
#define kcalloc(N,Z)  ccnl_calloc(N,Z)
#define kmalloc(Z)    ccnl_malloc(Z)
#define krealloc(P,Z) ccnl_realloc(P,Z)
#define kfree(P)      ccnl_free(P)

KHASH_INIT(256, khint256_t, unsigned char, 0, sha256toInt, sha256cmp)
khash_t(256) *OKlist;

struct hentry_s {
    struct ccnl_pkt_s *pkt;
};

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

int loadcnt;

void
add_content(char *dirpath, char *path)
{
    int f, datalen;

    DEBUGMSG(DEBUG, "loading %s\n", path);

    f = open(path, O_RDONLY);
    if (f < 0)
        return;
    datalen = read(f, iobuf, sizeof(iobuf));
    if (datalen > 0) {
        struct ccnl_pkt_s *pkt = NULL;
        unsigned char *data;
        int skip, _suite = ccnl_pkt2suite(iobuf, datalen, &skip);

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
            char *path2, *cp = digest2str(pkt->md);

            asprintf(&path2, "%s/%02x/%s", dirpath, (unsigned)pkt->md[0], cp+2);
            if (strcmp(path2, path)) {
                DEBUGMSG(WARNING, "wrong digest for file %s, ignored\n", path);
            } else {
                int absent;
                khint_t k = kh_put(256, OKlist, pkt->md, &absent);
                if (absent) {
                    unsigned char *md = ccnl_malloc(SHA256_DIGEST_LENGTH);
                    memcpy(md, pkt->md, SHA256_DIGEST_LENGTH);
                    kh_key(OKlist, k) = md;
                }
            }
            free(path2);
            free_packet(pkt);
        }
    }
    close(f);
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

int
main(int argc, char *argv[])
{
    int opt, max_cache_entries = 0, inter_load_gap = 10, udpport = 7777;
    char *dirpath = NULL, *ethdev = NULL, *uxpath = NULL;
    unsigned char buf[100], digest[SHA256_DIGEST_LENGTH];

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
    dirpath = argv[optind++];
    if (optind != argc)
        goto usage;

    OKlist = kh_init(256);

    while (strlen(dirpath) > 1 && dirpath[strlen(dirpath) - 1] == '/')
        dirpath[strlen(dirpath) - 1] = '\0';
    walk_fs(dirpath, dirpath);
    
    ccnl_SHA256(buf, sizeof(buf), digest);

    {
        printf("khash: size %d, buckets %d\n", kh_size(OKlist), kh_n_buckets(OKlist));

/*
        khiter_t k;
        int cnt = 0;

        for (k = kh_begin(OKlist); k != kh_end(OKlist); k++)
            if (kh_exist(OKlist, k))
                printf("%d %s\n", cnt++, digest2str(kh_key(OKlist, k)));
*/
    }

    printf("done (mem alloc: %ld bytes, %d chunks)\n", ccnl_total_alloc_bytes,
        ccnl_total_alloc_chunks);
}

// eof
