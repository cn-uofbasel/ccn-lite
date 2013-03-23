/*
 * @f ccn-lite-omnet.c
 * @b OmNet++ adaption functions
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
 * 2011-11-22 created
 */



/*****************************************************************************
 *
 * General options and configuration parameters
 *
 *****************************************************************************/


#define CCNL_OMNET

#define CCNL_DEBUG              // enable debug code for logging messages in stderr
#define CCNL_DEBUG_MALLOC       // enable recording of memory allocations for crash dumps
#define LOG_LEVEL_4_OMNET 50    // set the log level for debug info piped to Omnet's EV.
                                // (this is independent of 'log_level' in b3c-relay-debug.c
                                // which controls what is reported in stderr)


// TODO -- Use the following to set ccnl->ifs[ifndx].fwdalli = 1 in node init
#define PROPAGATE_INTERESTS_SEEN_BEFORE     // if a received interest already exists in the PIT,
                                            // it is not forwarded. Use this option to override this



/* TODO - MUST BE CHECKED FOR COMPLIANCE - -- Manolis: Ask Christian about adding the following as an option
 * (brought forward from old code)
 *
#define SERVE_EXACT_MATCH_CONTENT_ONLY  // loose match of the content name do not deliver the content
 *
 * requires the following modification in ccn-relay.c:779/ccnl_content_serve_pending() as well as
 * other places where ccnl_prefix_cmp() is used
 *
#if defined(SERVE_EXACT_MATCH_CONTENT_ONLY)
    int rc = ccnl_prefix_cmp(c->prefix, i->prefix, EXACT_MATCH);
#else
    int rc = ccnl_prefix_cmp(c->prefix, i->prefix, LOOSE_MATCH);
#endif
 *
 */

// TODO - YES - Manolis: Ask Christian if there is a distintion in the code between
// routers and application hosts, and weather this option makes sense
#define RUN_AS_APP_HOST     // configures special Face for communication with the application layer


// TODO - Default yes and controlled through the mgmt interface - Manolis: Ask Christian if we should set this as a compile time option or
// a per-relay init time option (e.g. to distinguish content sources from routers)
//
// Currently in file ccn-lite-relay.c:479/ccnl_populate_cache() it is hardcoded as an
// always-ON flag.
//
// What happens if the cache memory is full ? (Cache replacemnet strategy)
//
#define CONTENT_NEVER_EXPIRES   // content never deleted from the cache memory unless full






/*****************************************************************************
 *
 * API used by Omnet to communicate with CCN Lite (forward declarations)
 *
 * TODO: make these functions return an error code, so that I can block the
 * simulation in case of errors
 *
 *****************************************************************************/


/**
 * per relay control block and globals to keep track of the relay list in the
 * simulation
 */
struct relay_ctrl_b {
    struct relay_ctrl_b *   next;
    struct ccnl_relay_s *   state;
    void *                  opp_module;
    int                     cache_bytesize; // TODO: max memory allocated in cache for content storage (currently not used, must go in struct bbc_s)
    int                     node_id;
    char *                  node_name;
};

struct relay_ctrl_b   *all_relays=0, *last_relay=0, *active_relay;




//TODO -- check consistency of signatures with new versions (probably has changed)

void    ccnl_destroy_relay (void *ctrl_b);  /* mgmt */

void *  ccnl_create_relay (                 /* mgmt */
            void *opp_module,
            const char mac_ifs[][6],
            int num_mac_ifs,
            //const char sock_ifs[][6],
            //int num_sock_ifs,
            int node_id,
            const char *node_name,
            int cache_bytesize,
            int cache_inodes,
            int tx_pace
            );

void    ccnl_add_content (                  /* mgmt */
            void *ctrl_blk,
            const char *name,
            int seqn,
            void *data,
            int len
            );

void    ccnl_add_fwdrule(                   /* mgmt */
            void *ctrl_blk,
            const char *name,
            const char *dst,
            int netif_idx
            );

void    ccnl_app2relay(                     /* data */
            void *ctrl_blk,
            const char *name,
            int seqn
            );

void    ccnl_lnk2relay(                     /* data */
            void *ctrl_blk,
            const char *dst,
            const char *src,
            int rx_netif_idx,
            void *data,
            int len
            );






/*****************************************************************************
 *
 * Platform-related functions (using Omnet API) required by CCN Lite
 *
 * (macros and forward declarations)
 *
 * The following Omnet-to-CCNLite API functions are visible to the
 * code here as they are defined in CcnCore.c before the #include of this
 * file
 *
 *   opp_log (..)
 *   opp_relay2app(...)
 *   opp_relay2lnk(...)
 *   opp_timer (...)
 *   opp_time ()
 *
 *****************************************************************************/

// TODO -- Should the following go to the params and options section ?
#define USE_ENCAPS      // ?
#define USE_SCHEDULER   // ?
#define USE_ETHERNET    // ?

#include "ccnl-platform.h"  // generic platform defs
#include "ccnl-debug.h"     // debug macros and funcs
#include "ccnl-relay.h"     // main ccnl structs


#undef CCNL_NOW
#define CCNL_NOW() (opp_time())

#undef DEBUGMSG
#define DEBUGMSG(LVL, ...) do {   \
   if ((LVL)<=debug_level) \
   { \
       fprintf(stderr, "%s: ", timestamp());   \
       fprintf(stderr, __VA_ARGS__);   \
   } \
   if ((LVL)<=LOG_LEVEL_4_OMNET) \
   { \
       char pbuf[200]; \
       sprintf(pbuf, __VA_ARGS__);  \
       opp_log(active_relay->node_id, pbuf); \
   } \
   break; \
} while (0)



void*   ccnl_set_timer (
            int usec,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            );

void    ccnl_rem_timer (void *hdl);


void*   ccnl_set_absolute_timer (
            struct timeval abstime,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            );


void    ccnl_ll_TX(struct ccnl_relay_s *relay, struct ccnl_if_s *ifc, sockunion *dst, struct ccnl_buf_s *buf); // TODO -- this replaces network_delivery() from v0, implement it
int     ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);   // TODO: replaces client_delivery(...) from v0, import implem

void    ccnl_get_timeval (struct timeval *tv);                  // TODO: import implem of bbc_get_timeval(...) from v0
void    ccnl_print_stats(struct ccnl_relay_s *relay, int code); // TODO: implement this as in ccn-lite-simu.c (instead of node_log_handler)


// TOOD -- remove those, unless I need them locally they are not used elsewhere
//int                     relay_id(struct bbc_s *bbc);
//struct bbc_s *          relay_state(int id);






/*****************************************************************************
 *
 * CCN Lite code base inclusion
 *
 * (old style includes of .c files directly, so order matters here)
 *
 *****************************************************************************/

#include "ccnx.h"
#include "ccnl.h"

#ifdef CCNL_DEBUG_MALLOC
# include "ccnl-debug-mem.c"
#endif

#include "ccnl-debug.c"
#include "ccnl-platform.c"

/*
struct ccnl_encaps_s*   ccnl_encaps_new(int protocol, int mtu);
void                    ccnl_encaps_start(struct ccnl_encaps_s *e, struct ccnl_buf_s *buf, int ifndx, sockunion *su);
struct ccnl_buf_s*      ccnl_encaps_getnextfragment(struct ccnl_encaps_s *e, int *ifndx, sockunion *su);
int                     ccnl_encaps_getfragcount(struct ccnl_encaps_s *e, int origlen, int *totallen);
int                     ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
void                    ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);
*/

//#include "ccnl-relay.c"
#include "ccnl-relay-mgmt.c"
#include "ccnl-relay-sched.c"
#include "ccnl-relay-encaps.c"
#include "ccnl-relay.c"         // TODO -- check if this here obsoletes the commented out block of fwd decls above
#include "ccnl-packet.c"







/*****************************************************************************
 *
 * Platfrom and API function defs
 *
 *****************************************************************************/

// TODO - Probably only used in the kernel space / move to platform.c - (check and remove) -- Manolis: is there a particular reason for using these 2 instead of the string.h ones ?

static char*
mystrchr(char *list, char c)
{
    while (*list) {
    if (c == *list)
        return list;
    list++;
    }
    return NULL;
}

static char*
mystrtok(char **cp, char *delim)
{
    char *start = *cp;

    if (!cp || !*cp || !**cp)
    return NULL;
    while (*start && mystrchr(delim, *start))
    start++;
    *cp = start;
    while (**cp) {
    if (mystrchr(delim, **cp)) {
        **cp = '\0';
        (*cp)++;
        break;
    }
    (*cp)++;
    }
    return start;
}

/*****************************************************************************
 * given a content prefix as char string, gen a ccnl_prefix_s struct
 */
static struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(*pr));
    DEBUGMSG(99, "ccnl_path_to_prefix <%s>\n", path);

    if (!pr)
        return NULL;
    pr->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char**));
    pr->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    pr->path = (unsigned char*) ccnl_malloc(strlen(path)+1);
    if (!pr->comp || !pr->complen || !pr->path) {
        ccnl_free(pr->comp);
        ccnl_free(pr->complen);
        ccnl_free(pr->path);
        ccnl_free(pr);
        return NULL;
    }

    strcpy((char*) pr->path, path);
    cp = (char*) pr->path;
    for (path = mystrtok(&cp, "/");
         path && pr->compcnt < CCNL_MAX_NAME_COMP;
         path = mystrtok(&cp, "/")) {
        pr->comp[pr->compcnt] = (unsigned char*) path;
        pr->complen[pr->compcnt] = strlen(path);
        pr->compcnt++;
    }
    return pr;
}


/*****************************************************************************
 * system local time to struct timeval
 */
inline void
ccnl_get_timeval(struct timeval *tv)
{
    double now = CCNL_NOW();
    tv->tv_sec = now;
    tv->tv_usec = 1000000 * (now - tv->tv_sec);
}


// TODO - Yes - Manolis: Ask Christian
// 1. why the signature for set/rem timer is different in omnet in platform.c.
// 2. Also about the need to make the definition of struct ccnl_timer_s visible to all but kernel ver in ccnl-platform.c
// 3. should set/rem_timer always return a ptr to such a struct ?
// 4. adopt this in v0

struct ccnl_timer_s {
    struct ccnl_timer_s *next;
    struct timeval timeout;
    void (*fct)(char,int);
    void (*fct2)(void*,void*);
    char node;
    int intarg;
    void *aux1;
    void *aux2;
    int handler;
};


/*****************************************************************************
 * generic timer callback for CCN Lite (uses information from a struct
 * ccnl_timer_s to invoke the right event function
 */
// TODO -- Adopt the following wrapper in v0 too
void
ccnl_gen_callback(void * relay, void * timer_info){
    struct ccnl_timer_s     *ti;

    active_relay = (struct relay_ctrl_b *) relay;

    ti = (struct ccnl_timer_s *) timer_info;

    ti->fct2(ti->aux1, ti->aux2);

    ccnl_free (ti);
    return;
};


/*****************************************************************************
 * set a timer and register a callback event with omnet
 */
// TODO -- adapt like this the respective code in v0
void*
ccnl_set_timer (
            int usec,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            )
{
    struct ccnl_timer_s *timer_info;

    /* event info for timer
     */
    timer_info = (struct ccnl_timer_s *) ccnl_calloc(1, sizeof(struct ccnl_timer_s));
    if (!timer_info) return 0;
    timer_info->fct2 = fct;
    gettimeofday(&timer_info->timeout, NULL);
    usec += timer_info->timeout.tv_usec;
    timer_info->timeout.tv_sec += usec / 1000000;
    timer_info->timeout.tv_usec = usec % 1000000;
    timer_info->aux1 = aux1;
    timer_info->aux2 = aux2;

    /* pass to omnet to schedule event
     */
    timer_info->handler = opp_timer (
            active_relay->opp_module,
            1,
            usec,
            ccnl_gen_callback,
            (void *) active_relay,
            (void *) timer_info
            );

    if (timer_info->handler != 0)
        return timer_info;

    ccnl_free (timer_info);

    return NULL;
};


/*****************************************************************************
 * delete timer in omnet and dispose timer info
 */
// TODO -- adapt like this the respective code in v0
void
ccnl_rem_timer (void *timer_info)
{
    struct ccnl_timer_s *ti = (struct ccnl_timer_s *) timer_info;

    if (ti) {
        opp_timer (active_relay->opp_module, 0, ti->handler, 0, 0, 0);
        ccnl_free(ti);
    }

    return;
};


/*****************************************************************************
 * set timer and register callback event in omnet, using abs time
 * (used with chemflow)
 */
void*
ccnl_set_absolute_timer (
            struct timeval abstime,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            )
{
    struct timeval cur_time;
    int usec = 0;


    /* calculate usec time offset from now
     */
    ccnl_get_timeval(&cur_time);

    if ((abstime.tv_sec < cur_time.tv_sec) ||
            ((abstime.tv_sec == cur_time.tv_sec) && (abstime.tv_sec < cur_time.tv_sec)))
        return NULL;

    //usec += (abstime.tv_usec * 1000000 + abstime.tv_usec) - (cur_time.tv_usec * 1000000 + cur_time.tv_usec);
    usec = (abstime.tv_sec - cur_time.tv_sec) * 1000000;
    if ( abstime.tv_usec >= cur_time.tv_usec)
        usec += abstime.tv_usec - cur_time.tv_usec;
    else
        usec -= cur_time.tv_usec - abstime.tv_usec;

    /* use ccnl_set_timer() to sched the event
     */
    return ccnl_set_timer (usec, fct, aux1, aux2);
};


/*****************************************************************************
 * recursive callback for time progression
 */
void
ccnl_ageing(void *relay, void *aux) // TODO -- this replaces the do_ageing callback in v0, adapt the v0 to this
{
    DEBUGMSG(99, "do_ageing for node %s(%c)\n", active_relay->node_name, active_relay->node_id);

    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
};


// HERE

/*****************************************************************************
 * shutd relay node
 */
void
ccnl_destroy_relay (void *relay)
{
    active_relay = (struct relay_ctrl_b *) relay;
    struct ccnl_relay_s *relay_stat = active_relay->state;

    DEBUGMSG(99, "ccnl_destroy_relay %s (%d) \n", active_relay->node_name, active_relay->node_id);

    if (relay_st->stats) {
        end_log(relay_stat->stats->ofp_s);
        end_log(relay_stat->stats->ofp_r);
        end_log(relay_stat->stats->ofp_q);
        ccnl_free(relay_stat->stats);
        relay_stat->stats = NULL;
    }

    ccnl_relay_cleanup(relay_stat);

    // TODO - no chemflow for now - Manolis: Ask Christian, is chemflow controlling the one global system queue only ?
    // in that case it does not make sense to use it in the omnet.... (chemflow sched is not part
    // of the per node state block.
    //ccnl_sched_cleanup();

#ifdef CCNL_DEBUG_MALLOC
    debug_memdump();
#endif

    return;
};



/*****************************************************************************
 * create new relay node
 */
// TODO -- signature changed update according to this the v0 code, and the omnet code
// TODO -- replace the params with a ptr to a relay_config struct
void *  ccnl_create_relay (
            void *opp_module,
            const char mac_ifs[][6],
            int num_mac_ifs,
            //const char sock_ifs[][6],
            //int num_sock_ifs,
            int node_id,
            const char *node_name,
            int cache_bytesize,
            int cache_inodes,
            int tx_pace
            )
{
    struct relay_ctrl_b     *new_relay;
    struct ccnl_if_s        *i;

    active_relay = new_relay = (struct relay_ctrl_b *) ccnl_calloc (1, sizeof(struct relay_ctrl_b));


    /* init control block
     */
    new_relay->next = 0;
    new_relay->state = (struct ccnl_relay_s *) ccnl_calloc (1, sizeof(struct ccnl_relay_s));
    new_relay->node_id = node_id;
    new_relay->opp_module = opp_module;
    new_relay->cache_bytesize = cache_bytesize;
    new_relay->node_name = (char *) ccnl_calloc (1, strlen (node_name) + 1);
    strcpy (new_relay->node_name, node_name);
    new_relay->state->max_cache_entries = cache_inodes;


    /* append to the end of all_relays list
     */
    if (!all_relays)
        all_relays = last_relay = new_relay;
    else {
        last_relay->next = new_relay;
        last_relay = new_relay;
    }


    /* init eth[i] interfaces (one for each MAC addr we have)
     */
    for (int k=0 ; k < num_mac_ifs ; k++ )
    {
        i = &new_relay->state->ifs[k];
        i->addr.eth.sll_family = AF_PACKET;
        memcpy(i->addr.eth.sll_addr, mac_ifs[k], ETH_ALEN);
        i->mtu = 1400;
        i->reflect = 1;
        i->fwdalli = 0;
        i->sched = ccnl_sched_pktrate_new(ccnl_interface_CTS, new_relay->state, tx_pace);

        i->encaps = CCNL_ENCAPS_SEQUENCED2012;  // TODO -- Check what this does !

#ifdef PROPAGATE_INTERESTS_SEEN_BEFORE
        i->fwdalli = 1;
#endif
        new_relay->state->ifcount++;
    };


    /* TODO -- init sock[i] interfaces (one for each socket addr we have)
     */
    // ...


#ifdef USE_SCHEDULER
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
#endif

    // TODO -- think if i need this, since omnet already keeps stat logs ?
    //relay->stats = ccnl_calloc(1, sizeof(struct ccnl_stats_s));
    //ccnl_simu_node_log_start(relay, node);

    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);


    DEBUGMSG(99, "ccnl_create_relay\n");

    return (void *) new_relay;
};





// HERE: Do changes according to discussion with CT on Fri for the parts that
// I ve marked and adopt his updated code ver




void    ccnl_add_content (
            void *ctrl_blk,
            const char *name,
            int seqn,
            void *data,
            int len
            ) {};

void    ccnl_add_fwdrule(
            void *ctrl_blk,
            const char *name,
            const char *dst,
            int netif_idx
            ) {};

void    ccnl_app2relay(
            void *ctrl_blk,
            const char *name,
            int seqn
            ) {};

void    ccnl_lnk2relay(
            void *ctrl_blk,
            const char *dst,
            const char *src,
            int rx_netif_idx,
            void *data,
            int len
            ) {};



void    ccnl_ll_TX(struct ccnl_relay_s *relay, struct ccnl_if_s *ifc, sockunion *dst, struct ccnl_buf_s *buf) {}; // TODO -- this replaces network_delivery() from v0, implement it
int     ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c) {};   // TODO: replaces client_delivery(...) from v0, import implem

void    ccnl_print_stats(struct ccnl_relay_s *relay, int code) {}; // TODO: implement this as in ccn-lite-simu.c (instead of node_log_handler)


// -----------------------------------------------------------------



#define WOWMOM
#define B3C_DEBUG
#define B3C_DEBUG_MALLOC

#include "b3c.h"
#include "b3c-debug.h"
#include "b3c-relay-ccn.h"
#include "wowmom.h"

// #define b3c_print_stats(x,y)		do{}while(0)
void wowmom_phase_two(char a, int b);

#ifdef CCNB3CNET_H
# define B3C_NOW()			omnet_current_time()
# define wowmom_set_timer(T,F,N,A)	omnet_set_timer(T,F,N,A)
# define wowmom_rem_timer(H)		omnet_rem_timer(H)
#else
# define B3C_NOW()			current_time()
# include "omnet-replace.h"
#endif

#ifdef B3C_DEBUG_MALLOC
# include "wowmom-memdebug.c"
#endif

#include "b3c-relay-debug.c"
#include "b3c-relay-ccn.c"
#include "b3c-relay-support.c"
#include "b3c-relay-encaps.c"
#include "b3c-relay-sched.c"

struct bbc_s relays[5];

#ifndef CCNB3CNET_H
extern double omnet_current_time();
extern void b3c_wowmom_client_RX(char node, char *name, int seqn,
				 char *data, int len);
extern void b3c_wowmom_ll_TX(char node, unsigned char *dst, unsigned char *src,
			     unsigned char *data, int len);
extern void b3c_wowmom_LOG(char node, char *s);

// The WOWMOM clients also sit in OmNet's side, here we have a dummy version:
void b3c_wowmom_client(char node); // sending side
void b3c_wowmom_client_RX(char node, char *name,
			  int seqn, char *data, int len); // receiving side

// exists on OmNet's side already, here we reimplement the functionlity:
void omnet_eventloop(void); // juggles all events
void wowmom_ethernet(char dummy, int dummy2); // implements the media

// # include "omnet-replace.c"
# include "wowmom-client.c"
#endif

#include "wowmom-common.c"

// ----------------------------------------------------------------------

#define relayChars "ABC12"

static struct bbc_s*
char2relay(char node)
{
    unsigned int i;

    for (i = 0; i < strlen(relayChars); i++) {
	if (node == relayChars[i])
	    return relays + i;
    }
    fprintf(stderr, "unknown node %c\n", node);
    return 0;
}

static char
relay2char(struct bbc_s *relay)
{
    unsigned int i;

    for (i = 0; i < strlen(relayChars); i++) {
	if (relay == (relays + i))
	    return relayChars[i];
    }
    fprintf(stderr, "unknown node %p\n", (void *) relay);
    return '?';
}

void
b3c_wowmom_add2cache(char node, const char *name, int seqn, void *data, int len)
{
    b3c_wowmom_add2cache_(char2relay(node), name, seqn, data, len);
}


void
b3c_wowmom_init_node(char node, const char *addr,
		     int max_cache_entries,
		     int inter_packet_gap)
{
    struct bbc_if_s *i;
    struct bbc_s *relay = char2relay(node);
    if (!relay)	return;

    DEBUGMSG(99, "b3c_wowmom_init_node\n");

    relay->max_cache_entries = node == 'C' ? -1 : max_cache_entries;

    bbc_relay_encaps_init(relay);
    bbc_sched_init(relay, inter_packet_gap);

    // add (fake) eth0 interface with index 0:
    i = &relay->ifs[0];
    i->addr.eth.sll_family = AF_PACKET;
    memcpy(i->addr.eth.sll_addr, addr, ETH_ALEN);
    i->encaps = B3C_DGRAM_ENCAPS_ETH2011;
    i->mtu = 1400;
    i->reflect = 1;
    i->fwdalli = 1;
    relay->ifcount++;

#ifndef CCNB3CNET_H
    if (node == 'A' || node == 'B') {
	relay->wowmom = bbc_calloc(1, sizeof(struct wowmom_client_s));
	relay->wowmom->lastseq = WOWMOM_NUMBER_OF_CHUNKS-1;
	relay->wowmom->last_received = -1;
	relay->wowmom->name = node == 'A' ?
	    "/b3c/wowmom/movie1" : "/b3c/wowmom/movie2";
	relay->stats = bbc_calloc(1, sizeof(struct bbc_stats_s));
    }
#endif

    wowmom_set_timer(1000000, wowmom_do_ageing, node, 0);
    wowmom_node_log_start(relay, node);
}


void
b3c_wowmom_add_fwd_(char node, const char *name, char dstnode)
{
    struct bbc_s *dst;

    dst = char2relay(dstnode);
    b3c_wowmom_add_fwd(node, name, (char*) (dst->ifs[0].addr.eth.sll_addr));
}

// ----------------------------------------------------------------------
// function called by the B3C's encaps module:

void
b3c_ll_TX(struct bbc_s *bbc, unsigned char *dst, unsigned char *src,
		 unsigned char *data, int len)
{
    b3c_wowmom_ll_TX(relay2char(bbc), dst, src, data, len);
}

// ----------------------------------------------------------------------
// exported functions (called by OmNet):

int
wowmom_init(int max_cache_entries, int inter_packet_gap)
{
    static char dat[WOWMOM_CHUNK_SIZE];
    static char init_was_visited;
    int i;

    if (init_was_visited)
	return 0;
    init_was_visited = 1;

    DEBUGMSG(99, "b3c_wowmom_wowmom_init\n");

    // define each node's eth address:
    b3c_wowmom_init_node('A', "\x00\x00\x00\x00\x00\x0a",
			 max_cache_entries, inter_packet_gap);
    b3c_wowmom_init_node('B', "\x00\x00\x00\x00\x00\x0b",
			 max_cache_entries, inter_packet_gap);
    b3c_wowmom_init_node('C', "\x00\x00\x00\x00\x00\x0c",
			 max_cache_entries, inter_packet_gap);
    b3c_wowmom_init_node('1', "\x00\x00\x00\x00\x00\x01",
			 max_cache_entries, inter_packet_gap);
    b3c_wowmom_init_node('2', "\x00\x00\x00\x00\x00\x02",
			 max_cache_entries, inter_packet_gap);

    // install the system's forwarding pointers:
    b3c_wowmom_add_fwd_('A', "/b3c/wowmom", '2');
    b3c_wowmom_add_fwd_('B', "/b3c/wowmom", '2');
    b3c_wowmom_add_fwd_('2', "/b3c/wowmom", '1');
    b3c_wowmom_add_fwd_('1', "/b3c/wowmom", 'C');

    // turn node 'C' into a repository for three movies
    for (i = 0; i < WOWMOM_NUMBER_OF_CHUNKS; i++) {
	b3c_wowmom_add2cache('C', "/b3c/wowmom/movie1", i, dat, sizeof(dat));
	b3c_wowmom_add2cache('C', "/b3c/wowmom/movie2", i, dat, sizeof(dat));
	b3c_wowmom_add2cache('C', "/b3c/wowmom/movie3", i, dat, sizeof(dat));
    }

#ifndef CCNB3CNET_H
    wowmom_set_timer(5000, wowmom_ethernet, 'e', 0);

    // start clients:
    wowmom_set_timer( 500000, wowmom_client_start, 'A', 0);
    wowmom_set_timer(1000000, wowmom_client_start, 'B', 0);
    phaseOne = 1;
    //    pending_client_tasks = 2;
#endif

//    OMNETDEBUGMSG(5, '*', "simulation starts %s", "now");

    return 0;
}

void
wowmom_phase_two(char a, int b)
{
    phaseOne = 0;
    wowmom_set_timer(0, wowmom_client_start, 'A', 0);
    wowmom_set_timer(500000, wowmom_client_start, 'B', 0);
}


// ----------------------------------------------------------------------

#ifndef CCNB3CNET_H

void
wowmom_eventloop()
{ 
    static struct timeval now;
    long usec;

    //    printf("starting omnet eventloop now %g\n", B3C_NOW());

    while (events) {
	struct wowmom_timer_s *t = events;
	//    printf("  looping now %g\n", B3C_NOW());
	gettimeofday(&now, 0);
	usec = timevaldelta(&(t->timeout), &now);
	if (usec >= 0) {
	    // usleep(usec);
	    struct timespec ts;
	    ts.tv_sec = usec / 1000000;
	    ts.tv_nsec = 1000 * (usec % 1000000);
	    nanosleep(&ts, NULL);
	}
	t = events;
	events = t->next;
	if (t->fct) {
	    void (*fct)(char,int) = t->fct;
	    char c = t->node;
	    int i = t->aux;
	    bbc_free(t);
	    (fct)(c, i);
	} else if (t->fct2) {
	    void (*fct2)(void*,int) = t->fct2;
	    void *ptr = t->ptr;
	    int i = t->aux;
	    bbc_free(t);
	    (fct2)(ptr, i);
	}
    }
    DEBUGMSG(0, "wowmom event loop: no more events to handle\n");
}


void
wowmom_fini(char node, int aux)
{
    int i;
    for (i = 0; i < 5; i++)
	wowmom_fini_relay(relays+i);

    while(events)
	wowmom_rem_timer(events->handler);

    while(etherbuf) {
	struct bbc_ethernet_s *e = etherbuf->next;
	bbc_free(etherbuf);
	etherbuf = e;
    }

#ifdef B3C_DEBUG_MALLOC
    debug_memdump();
#endif
}


int
main(int argc, char **argv)
{
    int opt;
    int max_cache_entries = B3C_DEFAULT_MAX_CACHE_ENTRIES;
    int inter_packet_gap = 0;

    //    srand(time(NULL));
    srandom(time(NULL));

    while ((opt = getopt(argc, argv, "hc:g:v:")) != -1) {
        switch (opt) {
        case 'c':
            max_cache_entries = atoi(optarg);
            break;
        case 'g':
            inter_packet_gap = atoi(optarg);
            break;
        case 'v':
            debug_level = atoi(optarg);
            break;
        case 'h':
        default:
            fprintf(stderr, "usage: %s [-h] [-c MAX_CONTENT_ENTRIES] [-g MIN_INTER_PACKET_GAP] [-v DEBUG_LEVEL]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    wowmom_init(max_cache_entries, inter_packet_gap);

    DEBUGMSG(1, "simulation starts\n");
    wowmom_eventloop();
    DEBUGMSG(1, "simulation ends\n");

    return -1;
}

#endif


// eof
