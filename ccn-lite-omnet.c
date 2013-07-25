/*
 * @f ccn-lite-omnet.c
 * @b OmNet++ adaption functions
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
 *               2013, Manolis Sifalakis, University of Basel
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
 * 2013-04-03 updated for new ver of ccn-lite code (ms)
 */



/*****************************************************************************
 *
 * General options and configuration parameters
 *
 *****************************************************************************/


#define CCNL_OMNET


#define USE_DEBUG               // enable debug code for logging messages in stderr
#define USE_DEBUG_MALLOC        // enable recording of memory allocations for crash dumps
#define LOG_LEVEL_4_OMNET 50    // set the log level for debug info piped to Omnet's EV.
                                // (this is independent of 'log_level' in b3c-relay-debug.c
                                // which controls what is reported in stderr)
#define USE_FRAG     		// for pkt fragmentation 
#define USE_ETHERNET

#define USE_SCHEDULER           // enable relay scheduling capabilities (e.g. for inter-packet pacing)
#define USE_STATS               // enable statistics logging of relay actions to ext file

#define PROPAGATE_INTERESTS_SEEN_BEFORE     // if a received interest already exists in the PIT,
                                            // it is not forwarded. Use this option to override this

#define CONTENT_NEVER_EXPIRES   // content never deleted from the cache memory unless full



/*****************************************************************************
 *
 * API used by Omnet to communicate with CCN Lite (forward declarations)
 *
 *****************************************************************************/


void    ccnl_destroy_relay (void *relay);   /* mgmt */

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

int     ccnl_add_content (                  /* mgmt */
            void *relay,
            const char *name,
            int seqn,
            void *data,
            int len
            );

int     ccnl_add_fwdrule(                   /* mgmt */
            void *relay,
            const char *name,
            const char *dst,
            int netif_idx
            );

int     ccnl_app2relay(                     /* data */
            void *relay,
            const char *name,
            int seqn
            );

int     ccnl_lnk2relay(                     /* data */
            void *relay,
            const char *dst,
            const char *src,
            int rx_netif_idx,
            void *data,
            int len
            );




/*****************************************************************************
 *
 * Platform functions -OMNet container- required by CCN Lite
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

//#include "ccnl-includes.h"  // generic platform includes - included in CcnCore.cc
#include "ccnx.h"
#include "ccnl.h"
#include "ccnl-core.h"      // core ccnl structs
#include "ccnl-ext.h"       // forward declarations
#include "ccnl-ext-debug.c" // debug funcs and some macros that I redeclare below


struct ccnl_omnet_s {       // extension of ccnl_relay_s (aux field) for omnet state
    struct ccnl_relay_s *   next;
    struct ccnl_relay_s *   state;
    void *                  opp_module;
    int                     cache_bytesize; // TODO: max memory allocated in cache for content storage (currently not used, must go in struct bbc_s)
    int                     node_id;
    char *                  node_name;
};
struct ccnl_relay_s     *all_relays=0, *last_relay=0, *active_relay;



// #undef CCNL_NOW
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
       opp_log(((struct ccnl_omnet_s *)active_relay->aux)->node_name, pbuf); \
   } \
   break; \
} while (0)



#ifdef USE_STATS
#  define END_LOG(CHANNEL) do { if (CHANNEL){ \
     fprintf(CHANNEL, "#end\n"); \
     fclose(CHANNEL); \
     CHANNEL = NULL; \
} } while(0)
#else
#  define END_LOG(CHANNEL) do {} while(0)
#endif


// TODO -- (ms 3) stats recording (enum and func) should better be moved to debug.c,
// however for the moment it clashes with other code, so I temporarily just leave it here.
enum    {STAT_RCV_I, STAT_RCV_C, STAT_SND_I, STAT_SND_C, STAT_QLEN, STAT_EOP1};
void    ccnl_print_stats(struct ccnl_relay_s *relay, int code);


void    ccnl_get_timeval (struct timeval *tv);

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

void    ccnl_ll_TX (
            struct ccnl_relay_s *relay,
            struct ccnl_if_s *ifc,
            sockunion *dst,
            struct ccnl_buf_s *buf);

void    ccnl_app_RX(
            struct ccnl_relay_s *ccnl,
            struct ccnl_content_s *c);

void    ccnl_close_socket (int s);



/*****************************************************************************
 *
 * CCN Lite code base inclusion
 *
 * (old style includes of .c files directly, so order matters here)
 *
 *****************************************************************************/

#include "ccnl-platform.c"
#include "ccnl-core.c"
//#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-sched.c"
#include "ccnl-pdu.c"
#include "ccnl-ext-frag.c"



/*****************************************************************************
 *
 * Platform code for OMNet container - required by CCN Lite
 *
 * (definitions)
 *
 *****************************************************************************/

/*****************************************************************************
 * if ccn-lite wants to shutd a socket descriptor
 */
void
ccnl_close_socket(int s)
{
    // TODO: activate when UDP sockets are enabled in omnet, just a dummy for now
    return;
}

/*****************************************************************************
 * record stats
 */
// TODO -- (ms 2) It should better be moved to debug.c ?
void
ccnl_print_stats(struct ccnl_relay_s *relay, int code)
{
    //code 1 = recv interest
    //code 2 = recv content
    //code 3 = sent interest
    //code 4 = sent content
    //code 5 = queue length
    //code 6 = end phaseOne

    struct ccnl_stats_s *st = relay->stats;
    double t = CCNL_NOW();
    double tmp;

    if (!st || !st->ofp_r || !st->ofp_s || !st->ofp_q)
        return;

    switch (code) {
    case STAT_RCV_I:
    if (st->log_recv_t_i < st->log_recv_t_c) {
        tmp = st->log_recv_t_c;
    } else {
        tmp = st->log_recv_t_i;
    }
    fprintf(st->ofp_r, "%.4g\t %f\t %c\t %f\n",
        t, 1/(t - st->log_recv_t_i), '?', 1/(t-tmp));
    fflush(st->ofp_r);
    st->log_recv_t_i = t;
    break;

    case STAT_RCV_C:
    if (st->log_recv_t_i < st->log_recv_t_c) {
        tmp = st->log_recv_t_c;
    } else {
        tmp = st->log_recv_t_i;
    }
    fprintf(st->ofp_r, "%.4g\t %c\t %f\t %f\n",
        t, '?', 1/(t - st->log_recv_t_c), 1/(t-tmp));
    fflush(st->ofp_r);
    st->log_recv_t_c = t;
    break;

    case STAT_SND_I:
    if (st->log_sent_t_i < st->log_sent_t_c) {
        tmp = st->log_sent_t_c;
    } else {
        tmp = st->log_sent_t_i;
    }
    fprintf(st->ofp_s, "%.4g\t %f\t %c\t %f\n",
        t, 1/(t - st->log_sent_t_i), '?', 1/(t-tmp));
    fflush(st->ofp_s);
    st->log_sent_t_i = t;
    break;

    case STAT_SND_C:
    if (st->log_sent_t_i < st->log_sent_t_c) {
        tmp = st->log_sent_t_c;
    } else {
        tmp = st->log_sent_t_i;
    }
    fprintf(st->ofp_s, "%.4g\t %c\t %f\t %f\n",
        t, '?', 1/(t - st->log_sent_t_c), 1/(t-tmp));
    fflush(st->ofp_s);
    st->log_sent_t_c = t;
    break;

    case STAT_QLEN:
    fprintf(st->ofp_q, "%.4g\t %i\n", t, relay->ifs[0].qlen);
    fflush(st->ofp_q);
    break;

    case STAT_EOP1: // end of phase 1
    fprintf(st->ofp_r, "%.4g\t %f\t %f\t %f\n", t, 0.0, 0.0, 0.0);
    fflush(st->ofp_r);
    break;

    default:
    break;
    }
}

/*****************************************************************************
 * open FPs for logging and init stats recording
 */
void
ccnl_node_log_start(struct ccnl_relay_s *relay, char *node)
{
#ifdef USE_STATS
    char name[100];
    struct ccnl_stats_s *st = relay->stats;

    if (!st)
    return;

    sprintf(name, "relay_logs/%s-sent.log", node);
    st->ofp_s = fopen(name, "w");
    if (st->ofp_s) {
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# sent_log Node %s\n", node);
        fprintf(st->ofp_s,"#####################\n");
        fprintf(st->ofp_s,"# time\t sent i\t\t sent c\t\t sent i+c\n");
    fflush(st->ofp_s);
    }
    sprintf(name, "relay_logs/%s-rcvd.log", node);
    st->ofp_r = fopen(name, "w");
    if (st->ofp_r) {
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# rcvd_log Node %s\n", node);
        fprintf(st->ofp_r,"#####################\n");
        fprintf(st->ofp_r,"# time\t recv i\t\t recv c\t\t recv i+c\n");
    fflush(st->ofp_r);
    }
    sprintf(name, "relay_logs/%s-qlen.log", node);
    st->ofp_q = fopen(name, "w");
    if (st->ofp_q) {
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# qlen_log Node %s (relay->ifs[0].qlen)\n", node);
        fprintf(st->ofp_q,"#####################\n");
        fprintf(st->ofp_q,"# time\t qlen\n");
    fflush(st->ofp_q);
    }
#endif // USE_STATS

    return;
}

/*****************************************************************************
 * given a content prefix as char string, gen a ccnl_prefix_s struct
 */
// TODO -- (ms 2) un-static and move to platform.c ?
static struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(*pr));

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

    for (path = strtok(cp, "/");
         path && pr->compcnt < CCNL_MAX_NAME_COMP;
         path = strtok(NULL, "/")) {
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

/*****************************************************************************
 * generic timer callback for CCN Lite (uses information in a
 * ccnl_timer_s struct to invoke the right event function
 */
void
ccnl_gen_callback(void * relay, void * timer_info)
{
    struct ccnl_timer_s     *ti;

    if (!relay) return;
    active_relay = (struct ccnl_relay_s *) relay;

    ti = (struct ccnl_timer_s *) timer_info;
    if (!ti) return;

    ti->fct2(ti->aux1, ti->aux2);

    ccnl_free (ti);
    return;
};


/*****************************************************************************
 * set a timer and register a callback event with omnet
 */
void*
ccnl_set_timer (
            int usec,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            )
{
    struct ccnl_timer_s *timer_info;
    void * opp_mdl = ((struct ccnl_omnet_s *) active_relay->aux)->opp_module;


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
            opp_mdl,
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
void
ccnl_rem_timer (void *timer_info)
{
    struct ccnl_timer_s *ti = (struct ccnl_timer_s *) timer_info;
    void * opp_mdl = ((struct ccnl_omnet_s *) active_relay->aux)->opp_module;

    if (ti) {
        opp_timer (opp_mdl, 0, ti->handler, 0, 0, 0);
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
 * callback for time progression
 */
void
ccnl_ageing(void *relay, void *aux) 
{
    if (!relay) return;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;

    DEBUGMSG(99, "do_ageing for node %s(%c)\n", opp_info->node_name, opp_info->node_id);

    ccnl_do_ageing(relay, aux);
    ccnl_set_timer(1000000, ccnl_ageing, relay, 0);
};


/*****************************************************************************
 * shutd relay
 */
void
ccnl_destroy_relay (void *relay)
{
    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;

    DEBUGMSG(99, "ccnl_destroy_relay on node %s (%d) \n", opp_info->node_name, opp_info->node_id);

    if (active_relay->stats) {
        END_LOG(active_relay->stats->ofp_s);
        END_LOG(active_relay->stats->ofp_r);
        END_LOG(active_relay->stats->ofp_q);

        ccnl_free(active_relay->stats);
        active_relay->stats = NULL;
    }

    ccnl_free(opp_info->node_name);
    ccnl_core_cleanup(active_relay);

    struct ccnl_relay_s *r = all_relays;
    while (r)
    {
        if ( ((struct ccnl_omnet_s *) r->aux)->next == active_relay) {
            ((struct ccnl_omnet_s *) r->aux)->next = opp_info->next;

            if (last_relay == active_relay) last_relay = r;
            break;
        }
        r = ((struct ccnl_omnet_s *) r->aux)->next;
    }
    ccnl_free(active_relay);

    //ccnl_sched_cleanup();

#ifdef CCNL_DEBUG_MALLOC
    debug_memdump();
#endif

    return;
};


/*****************************************************************************
 * create new relay
 */
// TODO -- replace the params with a ptr to a relay_config struct
void *
ccnl_create_relay (
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
    struct ccnl_relay_s     *new_relay;
    struct ccnl_omnet_s     *opp_info;
    struct ccnl_if_s        *i;

    active_relay = new_relay = (struct ccnl_relay_s *) ccnl_calloc (1, sizeof(struct ccnl_relay_s));
    if (!active_relay) return NULL;

    opp_info = (struct ccnl_omnet_s *) ccnl_calloc (1, sizeof(struct ccnl_omnet_s));
    if (!opp_info) {
        ccnl_free (new_relay);
        return NULL;
    } else
        new_relay->aux = (void *) opp_info;

    DEBUGMSG(99, "ccnl_create_relay on node %s (%d) \n", opp_info->node_name, opp_info->node_id);


    /* init relay control block
     */
    opp_info->next = 0;
    opp_info->state = new_relay;
    opp_info->node_id = node_id;
    opp_info->opp_module = opp_module;
    opp_info->cache_bytesize = cache_bytesize;
    opp_info->node_name = (char *) ccnl_calloc (1, strlen (node_name) + 1);
    strcpy (opp_info->node_name, node_name);

    new_relay->max_cache_entries = cache_inodes;


    /* append to the end of all_relays list
     */
    if (!all_relays)
        all_relays = last_relay = new_relay;
    else {
        ((struct ccnl_omnet_s *) last_relay->aux)->next = new_relay;
        last_relay = new_relay;
    }


    /* init struct ccnl_if_s for each eth[i] interface (one for each MAC addr we are given)
     */
    for (int k=0 ; k < num_mac_ifs ; k++ )
    {
        i = &new_relay->ifs[k];
        i->addr.eth.sll_family = AF_PACKET;
        memcpy(i->addr.eth.sll_addr, mac_ifs[k], ETH_ALEN);
        i->mtu = 1400;
        i->reflect = 1;
        i->fwdalli = 0;

#ifdef  USE_SCHEDULER
        i->sched = ccnl_sched_pktrate_new (ccnl_interface_CTS, new_relay, tx_pace);
#endif  // USE_SCHEDULER

#ifdef  PROPAGATE_INTERESTS_SEEN_BEFORE
        i->fwdalli = 1;
#endif  // PROPAGATE_INTERESTS_SEEN_BEFORE

        new_relay->ifcount++;
    };


    /* no scheduling at the face level (only at the interface level for tx pacing)
     * TODO: think if I need to enable such a possibility for simu testing
     */
    new_relay->defaultFaceScheduler = NULL;


    /* init stats logging
     */
#ifdef USE_STATS
    new_relay->stats = (struct ccnl_stats_s *) ccnl_calloc(1, sizeof(struct ccnl_stats_s));
    if (!new_relay->stats)
        DEBUGMSG(99, "ccnl_create_relay on node %s (%d) -- stats logging not activated \n", opp_info->node_name, opp_info->node_id);
    else
        ccnl_node_log_start(new_relay, (char *) node_name);

#else // USE_STATS
    new_relay->stats = NULL;
#endif // USE_STATS

    ccnl_set_timer(1000000, ccnl_ageing, new_relay, 0);

    return (void *) new_relay;
};



/*****************************************************************************
 * store a content chunk to relay's CS
 */
int
ccnl_add_content (
            void *relay,
            const char *name,
            int seqn,
            void *data,
            int len
            )
{
    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return -1;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return -1;

    struct ccnl_buf_s *bp;
    struct ccnl_prefix_s *pp;
    char *name2, tmp[10], *n;
    char *namecomp[20];
    unsigned char tmp2[8192];
    int cnt, len2;
    struct ccnl_content_s *c;

    cnt = 0;
    n = name2 = strdup(name);
    name2 = strtok(name2, "/");
    while (name2) {
        namecomp[cnt++] = name2;
        name2 = strtok(NULL, "/");
    }
    sprintf(tmp, ".%d", seqn);
    namecomp[cnt++] = tmp;
    namecomp[cnt] = 0;

    len2 = mkContent(namecomp, (char *) data, len, tmp2);
    free(n);

    // ms: failed, free and ret
    if ( ! (bp = ccnl_buf_new(tmp2, len2)))
        return -1;

    strcpy((char*) tmp2, name);
    sprintf(tmp, "/.%d", seqn);
    strcat((char*) tmp2, tmp);

    // ms: failure, free and ret
    if ( ! (pp = ccnl_path_to_prefix((const char*)tmp2)) )
    {
        ccnl_free(bp);
        return -1;
    };

    c = ccnl_content_new((struct ccnl_relay_s *) relay, &bp, &pp, NULL, 0, 0);

    if (c) {
#ifdef CONTENT_NEVER_EXPIRES
        c->flags |= CCNL_CONTENT_FLAGS_STATIC;
#endif
        ccnl_content_add2cache((struct ccnl_relay_s *) relay, c);
    }
    else {  //ms: failure, free and ret
        ccnl_free(pp);
        ccnl_free(bp);
        return -1;
    }

    DEBUGMSG(99, "ccnl_add_content %s in relay of node %s (%d) \n", tmp2, opp_info->node_name, opp_info->node_id);

    return 0;
};


/*****************************************************************************
 * add a forwarding rule in relay's FIB
 */
int
ccnl_add_fwdrule(
            void *relay,
            const char *name,
            const char *dstaddr,
            int netif_idx
            )
{
    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return -1;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return -1;

    struct ccnl_forward_s *fwd;
    sockunion sun;

    DEBUGMSG(99, "ccnl_add_fwdrule <%s --> %s> in relay of node %s (%d) \n", name, dstaddr, opp_info->node_name, opp_info->node_id);


    // TODO - currently assuming only fwd rules with MAC addresses. Fix to
    // consider also socket addresses

    sun.eth.sll_family = AF_PACKET;
    memcpy(sun.eth.sll_addr, dstaddr, ETH_ALEN);
    fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));

    if ( !(fwd->prefix = ccnl_path_to_prefix(name)))
    {
        ccnl_free(fwd);
        return -1;
    };

    fwd->face = ccnl_get_face_or_create(
                        (struct ccnl_relay_s *) relay,
                        netif_idx,
                        &sun.sa,
                        sizeof(sun.eth));
#ifdef USE_FRAG
    if ( !fwd->face->frag )	// if newly created face, no fragment policy is defined yet
        fwd->face->frag = ccnl_frag_new(CCNL_FRAG_CCNx2013, 1200);
#endif

    if (! (fwd->face))
    {
        ccnl_free(fwd->prefix);
        ccnl_free(fwd);
        return -1;
    }

    fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;     // TODO -- ?
    fwd->next = ((struct ccnl_relay_s *) relay)->fib;
    ((struct ccnl_relay_s *) relay)->fib = fwd;

    return 0;
};


/*****************************************************************************
 * higher layer (omnet) passes to relay
 */
int
ccnl_app2relay(
            void *relay,
            const char *name,
            int seqn
            )
{
    char *namecomp[20], *n, *cname;
    char tmp[512], tmp2[10];
    int len, cnt;
    int nonce = rand();     // is this ok ? Is the application only checking that or the protocol too ?

    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return -1;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return -1;

    DEBUGMSG(10, "ccnl_app2relay on node %s (%d) requests content %s (chunk %d)\n", opp_info->node_name, opp_info->node_id, name, seqn);


    /* prepare content prefix
     */
    cnt = 0;
    n = cname = strdup(name);
    cname = strtok(cname, "/");
    while (cname) {
        namecomp[cnt++] = cname;
        cname = strtok(NULL, "/");
    }
    sprintf(tmp2, ".%d", seqn);
    namecomp[cnt++] = tmp2;
    namecomp[cnt] = 0;


    /* ccnb for Interest pkt
     */
    len = mkInterest(namecomp, (unsigned int *) &nonce, (unsigned char*) tmp);
    free(n);

    /* pass it to relay
     */
    ccnl_core_RX((struct ccnl_relay_s *) relay, -1, (unsigned char*)tmp, len, 0, 0);

    return 0;
};



/*****************************************************************************
 * lower layer (omnet) passes data to relay
 */
int
ccnl_lnk2relay(
            void *relay,
            const char *dst,
            const char *src,
            int rx_netif_idx,
            void *data,
            int len
            )
{
    sockunion sun;

    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return -1;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return -1;

    DEBUGMSG(10, "ccnl_lnk2relay on node %s (%d) received %d bytes of data on netif %d from %s destined to %s \n",
            opp_info->node_name,
            opp_info->node_id,
            len, rx_netif_idx,
            eth2ascii((unsigned char*)src),
            eth2ascii((unsigned char*)dst));

    /* TODO -- at the moment we assume only data coming from an ethernet face.
     * Extend for socket faces too..
     */
    sun.sa.sa_family = AF_PACKET;
    memcpy(sun.eth.sll_addr, src, ETH_ALEN);

    ccnl_core_RX(
            (struct ccnl_relay_s *) relay,
            rx_netif_idx,
            (unsigned char*) data,
            len,
            &sun.sa,
            sizeof(sun.eth));

    return 0;
};


/*****************************************************************************
 * relay passes data to lower layer (omnet)
 */
void
ccnl_ll_TX(
            struct ccnl_relay_s *relay,
            struct ccnl_if_s *ifc,
            sockunion *dst,
            struct ccnl_buf_s *buf)
{
    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return;

    DEBUGMSG(2, "ccnl_ll_TX on node %s (%d) sending %d bytes of data from %s destined to %s \n",
            opp_info->node_name,
            opp_info->node_id,
            buf->datalen,
            ccnl_addr2ascii(&ifc->addr),
            ccnl_addr2ascii(dst));

    opp_relay2lnk(
            opp_info->opp_module,
            (char *) dst->eth.sll_addr,
            (char *) ifc->addr.eth.sll_addr,
            buf->data,
            buf->datalen);

    return;
};


/*****************************************************************************
 * relay passes data to upper layer (omnet)
 */
//int
void
ccnl_app_RX(
        struct ccnl_relay_s *relay,
        struct ccnl_content_s *c)
{
    int i;
    char tmp[200], tmp2[10];
    struct ccnl_prefix_s *p = c->name;

    active_relay = (struct ccnl_relay_s *) relay;
    if (!active_relay) return;

    struct ccnl_omnet_s *opp_info = (struct ccnl_omnet_s *) active_relay->aux;
    if (!opp_info) return;


    tmp[0] = '\0';

    for (i = 0; i < p->compcnt-1; i++) {
        strcat((char*)tmp, "/");
        tmp[strlen(tmp) + p->complen[i]] = '\0';
        memcpy(tmp + strlen(tmp), p->comp[i], p->complen[i]);
    }

    memcpy(tmp2, p->comp[p->compcnt-1], p->complen[p->compcnt-1]);
    tmp2[p->complen[p->compcnt-1]] = '\0';

    opp_relay2app(opp_info->opp_module, tmp, atoi(tmp2+1), (char*) c->content, c->contentlen);

    DEBUGMSG(2, "ccnl_app_RX on node %s (%d) delivering %d bytes of content %s/%s \n",
            opp_info->node_name,
            opp_info->node_id,
            c->contentlen,
            tmp,
            tmp2);

    return;
};


// eof
