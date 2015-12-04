#ifndef CCNL_UNIT_TEST_H
#define CCNL_UNIT_TEST_H


#define USE_DEBUG
#define USE_DEBUG_MALLOC
#define CCNL_UNIX
#define USE_NFN
#define USE_NFN_MONITOR

//#define ccnl_core_addToCleanup(b)	do{}while(0)

enum {STAT_RCV_I, STAT_RCV_C, STAT_SND_I, STAT_SND_C, STAT_QLEN, STAT_EOP1};


#define ccnl_frag_destroy(e)            do{}while(0)
#define ccnl_print_stats(x,y)           do{}while(0)
#define ccnl_app_RX(x,y)		do{}while(0)

#include "unit.h"

#include <dirent.h>
#include <fnmatch.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>


#define CCNL_UNIX

#define USE_CCNxDIGEST
#define USE_DEBUG                      // must select this for USE_MGMT
#define USE_DEBUG_MALLOC
#define USE_LINKLAYER
#define USE_HTTP_STATUS
#define USE_NACK
#define USE_NFN
#define USE_NFN_NSTRANS
#define USE_SUITE_CCNB                 // must select this for USE_MGMT
#define USE_SUITE_CCNTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV
#define USE_SUITE_LOCALRPC
#define USE_UNIXSOCKET

#include "../../src/ccnl-os-includes.h"
// #include "../../src/ccnl-headers.h"
#include "../../src/ccnl-ext-logging.c"

#include "../../src/ccnl-defs.h"
#include "../../src/ccnl-core.h"

#include "../../src/ccnl-ext.h"
#include "../../src/ccnl-ext-debug.c"
#include "../../src/ccnl-os-time.c"

#define ccnl_app_RX(x,y)                do{}while(0)
#define ccnl_print_stats(x,y)           do{}while(0)
#define ccnl_close_socket(a)		do{}while(0)
#define ccnl_ll_TX(a,b,c,d)		do{a=a;}while(0)

#include "../../src/ccnl-core.c"

#include "../../src/ccnl-ext-http.c"
#include "../../src/ccnl-ext-mgmt.c"
#include "../../src/ccnl-ext-localrpc.c"
#include "../../src/ccnl-ext-nfn.c"
#include "../../src/ccnl-ext-nfnmonitor.c"
#include "../../src/ccnl-ext-sched.c"
#include "../../src/ccnl-ext-frag.c"
#include "../../src/ccnl-ext-crypto.c"


#endif
