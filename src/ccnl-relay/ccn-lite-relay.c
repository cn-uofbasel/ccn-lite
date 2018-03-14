/*
 * @f ccn-lite-relay.c
 * @b user space CCN relay
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
 * 2011-11-22 created
 */


#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#ifdef USE_HTTP_STATUS
#include "ccnl-http-status.h"
#endif
#include "ccnl-os-includes.h"

#include "ccnl-core.h"

#include "ccnl-dispatch.h"

/*

#define CCNL_UNIX

#define USE_CCNxDIGEST
// #define USE_DEBUG                      // must select this for USE_MGMT
// #define USE_DEBUG_MALLOC
#define USE_DUP_CHECK
#define USE_ECHO
#define USE_LINKLAYER
//#define USE_FRAG
#define USE_HMAC256
#define USE_HTTP_STATUS
#define USE_IPV4
#define USE_IPV6
#define USE_MGMT
// #define USE_NACK
// #define USE_NFN
#define USE_NFN_NSTRANS
// #define USE_NFN_MONITOR
//#define USE_NFN_KEEPALIVE
//#define USE_NFN_PULL
// #define USE_SCHEDULER
#define USE_STATS
#define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_NDNTLV
#define USE_SUITE_LOCALRPC
#define USE_UNIXSOCKET
// #define USE_SIGNATURES

#define NEEDS_PREFIX_MATCHING
*/

#include "ccn-lite-relay.h"
#include "ccnl-unix.h"

static int lasthour = -1;
static int inter_ccn_interval = 0; // in usec
static int inter_pkt_interval = 0; // in usec

#ifdef CCNL_ARDUINO
const char compile_string[] PROGMEM = ""
#else
const char *compile_string = ""
#endif

#ifdef USE_CCNxDIGEST
        "CCNxDIGEST, "
#endif
#ifdef USE_DEBUG
        "DEBUG, "
#endif
#ifdef USE_DEBUG_MALLOC
        "DEBUG_MALLOC, "
#endif
#ifdef USE_ECHO
        "ECHO, "
#endif
#ifdef USE_LINKLAYER
        "ETHERNET, "
#endif
#ifdef USE_WPAN
        "WPAN, "
#endif
#ifdef USE_FRAG
        "FRAG, "
#endif
#ifdef USE_HMAC256
        "HMAC256, "
#endif
#ifdef USE_HTTP_STATUS
        "HTTP_STATUS, "
#endif
#ifdef USE_KITE
        "KITE, "
#endif
#ifdef USE_LOGGING
        "LOGGING, "
#endif
#ifdef USE_MGMT
        "MGMT, "
#endif
#ifdef USE_NACK
        "NACK, "
#endif
#ifdef USE_NFN
        "NFN, "
#endif
#ifdef USE_NFN_MONITOR
        "NFN_MONITOR, "
#endif
#ifdef USE_NFN_NSTRANS
        "NFN_NSTRANS, "
#endif
#ifdef USE_SCHEDULER
        "SCHEDULER, "
#endif
#ifdef USE_SIGNATURES
        "SIGNATURES, "
#endif
#ifdef USE_SUITE_CCNB
        "SUITE_CCNB, "
#endif
#ifdef USE_SUITE_CCNTLV
        "SUITE_CCNTLV, "
#endif
#ifdef USE_SUITE_CISTLV
        "SUITE_CISTLV, "
#endif
#ifdef USE_SUITE_LOCALRPC
        "SUITE_LOCALRPC, "
#endif
#ifdef USE_SUITE_NDNTLV
        "SUITE_NDNTLV, "
#endif
#ifdef USE_UNIXSOCKET
        "UNIXSOCKET, "
#endif
        ;

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int opt, max_cache_entries = -1, httpport = -1;
    int udpport1 = -1, udpport2 = -1;
    int udp6port1 = -1, udp6port2 = -1;
    char *datadir = NULL, *ethdev = NULL, *crypto_sock_path = NULL;
    char *wpandev = NULL;
    int suite = CCNL_SUITE_DEFAULT;
    struct ccnl_relay_s *theRelay = ccnl_malloc(sizeof(struct ccnl_relay_s));
#ifdef USE_UNIXSOCKET
    char *uxpath = CCNL_DEFAULT_UNIXSOCKNAME;
#else
    char *uxpath = NULL;
#endif
#ifdef USE_ECHO
    char *echopfx = NULL;
#endif

    time(&theRelay->startup_time);
    unsigned int seed = time(NULL) * getpid();
#ifdef __linux__
    srand(seed);
#else
    srandom(seed);
#endif

    while ((opt = getopt(argc, argv, "hc:d:e:g:i:o:p:s:t:u:6:v:w:x:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'd':
            datadir = optarg;
            break;
        case 'e':
            ethdev = optarg;
            break;
        case 'g':
            inter_pkt_interval = atoi(optarg);
            break;
        case 'i':
            inter_ccn_interval = atoi(optarg);
            break;
#ifdef USE_ECHO
        case 'o':
            echopfx = optarg;
            break;
#endif
        case 'p':
            crypto_sock_path = optarg;
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite))
                goto usage;
            break;
        case 't':
            httpport = atoi(optarg);
            break;
        case 'u':
            if (udpport1 == -1)
                udpport1 = atoi(optarg);
            else
                udpport2 = atoi(optarg);
            break;
        case '6':
            if (udp6port1 == -1)
                udp6port1 = atoi(optarg);
            else
                udp6port2 = atoi(optarg);
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
#ifdef USE_WPAN
        case 'w':
            wpandev = optarg;
            break;
#endif
        case 'x':
            uxpath = optarg;
            break;
        case 'h':
        default:
usage:
            fprintf(stderr,
                    "usage: %s [options]\n"
                    "  -c MAX_CONTENT_ENTRIES\n"
                    "  -d databasedir\n"
                    "  -e ethdev\n"
                    "  -g MIN_INTER_PACKET_INTERVAL\n"
                    "  -h\n"
                    "  -i MIN_INTER_CCNMSG_INTERVAL\n"
#ifdef USE_ECHO
                    "  -o echo_prefix\n"
#endif
                    "  -p crypto_face_ux_socket\n"
                    "  -s SUITE (ccnb, ccnx2015, cisco2015, ndn2013)\n"
                    "  -t tcpport (for HTML status page)\n"
                    "  -u udpport (can be specified twice)\n"
                    "  -6 udp6port (can be specified twice)\n"

#ifdef USE_LOGGING
                    "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
#ifdef USE_WPAN
                    "  -w wpandev\n"
#endif
#ifdef USE_UNIXSOCKET
                    "  -x unixpath\n"
#endif
                    , argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    opt = ccnl_suite2defaultPort(suite);
    if (udpport1 < 0)
        udpport1 = opt;
    if (httpport < 0)
        httpport = opt;

    ccnl_core_init();

    DEBUGMSG(INFO, "This is ccn-lite-relay, starting at %s",
             ctime(&theRelay->startup_time) + 4);
    DEBUGMSG(INFO, "  ccnl-core: %s\n", CCNL_VERSION);
    DEBUGMSG(INFO, "  compile time: %s %s\n", __DATE__, __TIME__);
    DEBUGMSG(INFO, "  compile options: %s\n", compile_string);
    DEBUGMSG(INFO, "  seed: %u\n", seed);
//    DEBUGMSG(INFO, "using suite %s\n", ccnl_suite2str(suite));

    ccnl_relay_config(theRelay, ethdev, wpandev, udpport1, udpport2,
		      udp6port1, udp6port2, httpport,
                      uxpath, suite, max_cache_entries, crypto_sock_path);
    if (datadir)
        ccnl_populate_cache(theRelay, datadir);

#ifdef USE_ECHO
    if (echopfx) {
        struct ccnl_prefix_s *pfx;
        char *dup = ccnl_strdup(echopfx);

        pfx = ccnl_URItoPrefix(dup, suite, NULL, NULL);
        if (pfx)
            ccnl_echo_add(theRelay, pfx);
        ccnl_free(dup);
    }
#endif

    ccnl_io_loop(theRelay);

    while (eventqueue)
        ccnl_rem_timer(eventqueue);

    ccnl_core_cleanup(theRelay);
#ifdef USE_HTTP_STATUS
    theRelay->http = ccnl_http_cleanup(theRelay->http);
#endif
#ifdef USE_DEBUG_MALLOC
    debug_memdump();
#endif
    ccnl_free(theRelay);
    return 0;
}

int 
ccnl_static_fields1(){
    return lasthour + inter_ccn_interval + inter_pkt_interval;
}

// eof
