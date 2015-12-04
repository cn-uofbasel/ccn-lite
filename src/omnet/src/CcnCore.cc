/*
 * CcnCore.cc
 *
 * C++ encapsulation of the C codebase of CCN Lite
 *
 *  Created on: Aug 11, 2012
 *      Author: manolis sifalakis
 */



#include "CcnCore.h"
#include "Ccn.h"
#include "CcnAppMessage_m.h"

#define EMBED_CHUNKS_IN_NAMES
#define _CHUNK_POSTFIX_MAXCHAR 20







/*****************************************************************************
 * Omnet's version of a CCN-lite control block (wraps a ccnl_relay_s)
 *****************************************************************************/

struct ccnl_omnet_s {       // extension of ccnl_relay_s (aux field) for keeping omnet state
     struct ccnl_omnet_s *  next;
     void *                 state;         // struct ccnl_relay_s of ccn-lite goes here
     void *                 opp_module;    // omnet module goes here
     int                    cs_max_entries;
     int                    cs_bytesize;
     int                    node_id;
     std::string            node_name;
};
struct ccnl_omnet_s     *all_relays=0, *last_relay=0, *active_relay;



/*****************************************************************************
 * CCN Lite code base integration/configuration
 *****************************************************************************/

/*
 * Configure the compilation of CCN-lite
 */
#define CCNL_OMNET
#define PLATFORM_SRVCS
#define USE_DEBUG               // enable debug code for logging messages in stderr
#define PLATFORM_LOG_THRESHOLD 50    // set the log level for debug info piped to Omnet's EV (this is independent of 'log_level' in b3c-relay-debug.c which controls what is reported in stderr)
//#define USE_FRAG              // pkt fragmentation
#define USE_LINKLAYER
//#define USE_IPV4
#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV
//#define USE_SCHEDULER         // enable relay scheduling capabilities (e.g. for inter-packet pacing)
#define CONTENT_NEVER_EXPIRES   // content never deleted from the cache memory unless full
#define PROPAGATE_INTERESTS_SEEN_BEFORE     // if a received interest already exists in the PIT, it is not forwarded. Use this option to override this
#define CCNL_OS_TIME_C          // prevent loading of the header with the system time functions (omnet provides its own)



/*
 * Hide the ccn-lite code base in a distinct namespace
 */
namespace CcnLiteRelay {

#include "ccnl-uapi.h" // the complete code base

extern "C" {    // host platform service functions (omnet side provides)
inline double   opp_time () { return simTime().dbl(); };
inline char *   opp_timestamp () {return (char *) simTime().str().c_str();};
void            opp_get_timeval(struct timeval *tv);
void            opp_log (char *msg );
void *          opp_set_timer (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2 );
void *          opp_set_absolute_timer (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2);
void            opp_rem_timer (void *timer_info);
void            opp_deliver (void *relay, struct CcnLiteRelay::info_data_s const * io, struct CcnLiteRelay::envelope_s * env);
inline void     opp_print_stats (struct ccnl_relay_s *relay, int code) {};
}

struct platform_vt_s platform_srvcs = {     // populate platform_srvcs vtable so that ccn-lite can access them
        opp_deliver,
        0,
        0,
        opp_set_timer,
        opp_set_absolute_timer,
        opp_rem_timer,
        opp_time,
        opp_timestamp,
        opp_get_timeval,
        opp_log,
        opp_print_stats,
        0
};

struct ccnl_timer_s { // timer container for ccn-lite callbacks (used by the platform service functions above)
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

};  // end of namespace CcnLiteRelay def





/*****************************************************************************
 * implementation of host platform service functions -- they are in CCN Lite's
 * namespace so that they are accessible in C
 *****************************************************************************/

/** generic callback for timers (uses ccnl_timer_s struct to invoke the right event function) **/
extern "C"
void
ccnl_gen_callback(void * relay, void * timer_info)
{
    struct CcnLiteRelay::ccnl_timer_s     *ti;

    if (!relay) return;
    active_relay = (struct ccnl_omnet_s *) relay;

    ti = (struct CcnLiteRelay::ccnl_timer_s *) timer_info;
    if (!ti) return;

    ti->fct2(ti->aux1, ti->aux2);

    free (ti);
    return;
};

/** construct a timer and register a callback event with omnet **/
extern "C"
void *
CcnLiteRelay::opp_set_timer (
            int usec,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            )
{
    struct CcnLiteRelay::ccnl_timer_s *timer_info;
    CcnCore *mdl = static_cast <CcnCore *>(active_relay->opp_module);

    /* event info for timer */
    timer_info = (struct CcnLiteRelay::ccnl_timer_s *) calloc (1, sizeof(struct CcnLiteRelay::ccnl_timer_s));
    if (!timer_info) return 0;
    timer_info->fct2 = fct;
    CcnLiteRelay::opp_get_timeval(&timer_info->timeout);    // get current time
    usec += timer_info->timeout.tv_usec;
    timer_info->timeout.tv_sec += usec / 1000000;
    timer_info->timeout.tv_usec = usec % 1000000;
    timer_info->aux1 = aux1;
    timer_info->aux2 = aux2;

    /* pass to omnet to schedule event */
    mdl->manageTimerEvent (1, usec, ccnl_gen_callback, (void *) active_relay, (void *) timer_info);

    if (timer_info->handler != 0)
        return timer_info;
    else {
        free (timer_info);
        return NULL;
    }
};


/** set timer and register callback event in omnet, using abs time (used with chemflow) **/
extern "C"
void*
CcnLiteRelay::opp_set_absolute_timer (
            struct timeval abstime,
            void (*fct)(void *aux1, void *aux2),
            void *aux1,
            void *aux2
            )
{
    struct timeval cur_time;
    int usec = 0;

    /* calculate usec time offset from now */
    CcnLiteRelay::opp_get_timeval(&cur_time);

    if ((abstime.tv_sec < cur_time.tv_sec) ||
            ((abstime.tv_sec == cur_time.tv_sec) && (abstime.tv_sec < cur_time.tv_sec)))
        return NULL;

    usec = (abstime.tv_sec - cur_time.tv_sec) * 1000000;
    if ( abstime.tv_usec >= cur_time.tv_usec)
        usec += abstime.tv_usec - cur_time.tv_usec;
    else
        usec -= cur_time.tv_usec - abstime.tv_usec;

    /* use ccnl_set_timer() to sched the event */
    return CcnLiteRelay::ccnl_set_timer (usec, fct, aux1, aux2);
};


/** delete timer previously registered in omnet and dispose timer info **/
extern "C"
void
CcnLiteRelay::opp_rem_timer (void *timer_info)
{
    struct CcnLiteRelay::ccnl_timer_s *ti = (struct CcnLiteRelay::ccnl_timer_s *) timer_info;
    CcnCore *mdl = static_cast <CcnCore *>(active_relay->opp_module);

    if (ti) {
        mdl->manageTimerEvent (0, ti->handler, 0, 0, 0);
        ccnl_free(ti);
    }

    return;
};

extern "C"
void
CcnLiteRelay::opp_get_timeval(struct timeval *tv)
{
    double now = CcnLiteRelay::opp_time();
    tv->tv_sec = now;
    tv->tv_usec = 1000000 * (now - tv->tv_sec);
}


extern "C"
void
CcnLiteRelay::opp_log (char *log )
{
    int debugLevel = defaultDebugLevel;    // used in the DBGPRT() macro

    // messages from the ccn-lite kernel will only be reported at "Detail" level logging
    // and on the aux channel. To enable/see them set the defaulDebugInfo at the scenario
    // ini file to 4

    DBGPRT(AUX, Detail, active_relay->node_name ) << "-- ccn-lite kernel --: " << log ; // << std::endl;
};


extern "C"
void
CcnLiteRelay::opp_deliver (void *relay, struct CcnLiteRelay::info_data_s const * io, struct CcnLiteRelay::envelope_s * env)
{
    char *          name = 0;
    unsigned int    name_len=0;
    char            i_or_c = 100;   // random num >10 to make it by default invalid
    char            suite = 0;    // default invalid suite

    active_relay = (struct ccnl_omnet_s *) ((struct CcnLiteRelay::ccnl_relay_s *) relay)->aux;
    if (!active_relay) return;

    CcnCore *mdl = static_cast <CcnCore *>(active_relay->opp_module);

    if (env->to == CcnLiteRelay::ENV_APP)         // destined to the application layer
    {
        if (io->name) {
            mdl->deliverContent((char *) io->name, io->chunk_seqn, (char*) io->packet_bytes, io->packet_len);
        }
        else    // we receive a list of name components
        {
            for (int i=0 ; io->name_components[i] ; i++)
                name_len += strlen (io->name_components[i]);

            name = (char *) malloc (name_len * sizeof (char) + 1);
            name[0] = '\0';

            for (int i=0 ; io->name_components[i] ; i++)
                strcat (name, io->name_components[i]);

            mdl->deliverContent(name, io->chunk_seqn, (char*) io->packet_bytes, io->packet_len);
        }
    }
    else if (env->to == CcnLiteRelay::ENV_LINK)   // destined to the wire
    {
        // translate between the ccn lite api enums and our local ones in omnet
        i_or_c = (readTypeIorC(io) % 2) ? MSG_TYPE_INTEREST : MSG_TYPE_CONTENT;
        switch (readSuite(io)) {
        case CCNx_0: suite = SUITE_CCNx0; break;
        case CCNx_1: suite = SUITE_CCNx1; break;
        case NDNx_0: suite = SUITE_NDNx0; break;
        default: break;
        };

        mdl->toMACFace(
                (char *) env->link.sa_remote.linklayer.sll_addr,  // next hop addr,
                (char *) env->link.sa_local.linklayer.sll_addr,   // local addr of outgoing interface
                i_or_c,
                suite,
                io->name,
                io->chunk_seqn,
                io->packet_bytes,
                io->packet_len);
    }
    else {
        opp_error("CCN-Lite kernel on node %s tried to deliver data %s to the host platform, with an envelope I cannot parse yet (not APP or ENV_LINK)", active_relay->node_name.c_str(), io->name);
    }

    return;
}







/*****************************************************************************
 * Class CcnCore wraps in a OO-context the access to ccn-lite for omnet modules
 *****************************************************************************/

/*
 * Ctor
 */
CcnCore::CcnCore (Ccn *owner, const char *nodeName, const char *coreVer):
    relayName(nodeName),
    coreVersion(coreVer),
    ccnModule(owner),
    ctrlBlock(0)
{
    int     added_ifaces=0;

    /* Set debug level first so that we can provide debug output */
    this->debugLevel = ::defaultDebugLevel;

    int numMacIds = ccnModule->numMacIds;

    // create omnet state control block
    ctrlBlock = new ccnl_omnet_s();
    active_relay = (struct ccnl_omnet_s *) ctrlBlock;
    if (!ctrlBlock)
        opp_error(" Initialisation of CCN-Lite core for %s failed: ERROR in control block allocation!", nodeName);

    // create ccn-lite kernel state
#ifdef USE_STATS
    active_relay->state = CcnLiteRelay::createRelay(ccnModule->getFullPath().c_str(), ccnModule->maxCacheSlots, ccnModule->maxCacheBytes, 0, active_relay);
#else
    active_relay->state = CcnLiteRelay::createRelay(ccnModule->getFullPath().c_str(), ccnModule->maxCacheSlots, ccnModule->maxCacheBytes, 1, active_relay);
#endif

    if (! active_relay->state) {
        delete active_relay;
        opp_error(" Initialisation of CCN lite core for %s failed: ERROR while creating relay!", nodeName);
    }

    /* init omnet relay control block */
    active_relay->next = 0;
    active_relay->node_id = ccnModule->getId();
    active_relay->opp_module = this;
    active_relay->cs_max_entries = ccnModule->maxCacheSlots;
    active_relay->cs_bytesize = ccnModule->maxCacheBytes;        // TODO: rm this ?? Is it used somewhere ?
    active_relay->node_name = ccnModule->getFullPath();

    /* configure relay ifaces */
    struct CcnLiteRelay::info_iface_conf_s *iface_cfg = 0;
    struct CcnLiteRelay::info_mgmt_s *all_iface_cfg = 0;

    for (int k=0 ; k < numMacIds ; k++ )
    {
        if (iface_cfg) {
            iface_cfg->base.next = CcnLiteRelay::mallocMgmtInfo(CcnLiteRelay::info_iface_conf_s);
            iface_cfg = (struct CcnLiteRelay::info_iface_conf_s *) iface_cfg->base.next;
        }
        else {
            iface_cfg = (struct CcnLiteRelay::info_iface_conf_s *) CcnLiteRelay::mallocMgmtInfo(CcnLiteRelay::info_iface_conf_s);
            all_iface_cfg = (struct CcnLiteRelay::info_mgmt_s *) iface_cfg;
        }

        iface_cfg->base.relay = active_relay->state;
        iface_cfg->addr.linklayer.sll_family = AF_PACKET;
        iface_cfg->tx_pace = ccnModule->minInterTxTime;
        iface_cfg->status_up = 1;
        iface_cfg->base.next = 0;
        iface_cfg->iface_id = k;         // we ask that the indexing of the interfaces in ccn-lite follows the indexing of the macIds array (for tractability)
        ccnModule->macIds[k].getAddressBytes(iface_cfg->addr.linklayer.sll_addr);
    }

    added_ifaces = CcnLiteRelay::addMgmt(all_iface_cfg);

    if (added_ifaces == -1) {
        DBGPRT(EVAUX, Err, active_relay->node_name ) << "FAILED initializing required interfaces, while instantiating CCN-lite relay with id. Tearing down relay .. " << active_relay->node_id << std::endl;

        CcnLiteRelay::destroyRelay(active_relay->state);
        delete active_relay;
        CcnLiteRelay::freeMgmtInfo (all_iface_cfg);

        opp_error("FAILED initializing required interfaces, while instantiating CCN-lite relay with id %d. Tore down relay %s ", active_relay->node_id, active_relay->node_name.c_str());
    }

    DBGPRT(EVAUX, Info, active_relay->node_name ) << "Initialized "<< added_ifaces << " interfaces, for CCN-lite relay with id " << active_relay->node_id << std::endl;

    /* chain up the control block to the end of all_relays list */
    if (!all_relays) {
        all_relays = last_relay = active_relay;
    }
    else
    {
        last_relay->next = active_relay;
        last_relay = active_relay;
    }

    // start relay aging loop
    CcnLiteRelay::startRelay (active_relay->state);

    CcnLiteRelay::freeMgmtInfo (all_iface_cfg);

    return;
};



/*
 * Dtor
 */
CcnCore::~CcnCore ()
{
    active_relay = (struct ccnl_omnet_s *) (ctrlBlock);

    CcnLiteRelay::destroyRelay(active_relay->state);
    DBGPRT(EVAUX, Info, active_relay->node_name ) << "Torn down CCN-lite relay with id " << active_relay->node_id << std::endl;

    // remove relay cb from list of all relays
    struct ccnl_omnet_s *r, *prev_r;
    r = prev_r = all_relays;
    while (r)
    {
        if (r == active_relay) {
            if (r == all_relays)
                all_relays = active_relay->next;
            else
                prev_r->next = active_relay->next;

            break;
        }
        prev_r = r;
        r = r->next;
    }

    delete active_relay;
    return;
}



/*
 * text description of the object
 */
std::string
CcnCore::info ()
{
    std::string  text(relayName);
    text += " (" + coreVersion + ")";
    return text;
}




/*
 * add a range of chunks for a content name to the cache
 */
int
CcnCore::addToCacheFixedSizeChunks(const char *contentName, const int startChunk, const int numChunks, const char *chunkPtrs[], const int chunkSize)
{
    struct CcnLiteRelay::info_cs_data_s *   cs_data = 0;
    struct CcnLiteRelay::info_mgmt_s *      mgmt_cs = 0, *all_mgmt_cs = 0;
    int                                     chunks_added = 0;
    char                                    *name_buf, *name;
    int                                     i = 0, suite_id;
    int                                     name_len;

    assert (ctrlBlock);
    active_relay = (struct ccnl_omnet_s *) ctrlBlock;

    assert (contentName);
    name = name_buf = strdup(contentName);

    // parse suite info possibly present in the name
    suite_id = extractSuiteFromName (&name, NULL);
    name_len = strlen(name);            // name now points to the beginning of the name without the suite prefix

    if (suite_id == -1) {
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "In adding content " << contentName << " to cache"
            << ", encountered a problem: UNKNOWN SUITE specified!"
            << std::endl;

        free (name_buf);    // strduped contentName
        return 0;
    } else if (suite_id == 0) // no suite provided, default to ccnb
        suite_id = CcnLiteRelay::CCN_XMLB;
    else                    // suite prefix found,
        memcpy(name_buf, name, name_len + 1);   // overwrite the suite prefix by shifting the name string to the beginning of the buffer

#ifdef EMBED_CHUNKS_IN_NAMES
    char * listof_chunk_names = (char *) malloc (numChunks * (name_len + _CHUNK_POSTFIX_MAXCHAR + 1));
#endif

    for (i = 0 ; i < numChunks ; i++)
    {
        // prep the mgmt info
        if (!mgmt_cs) {
            all_mgmt_cs = mgmt_cs = CcnLiteRelay::mallocMgmtInfo(CcnLiteRelay::info_cs_data_s);
        } else {
            mgmt_cs = (mgmt_cs->next = CcnLiteRelay::mallocMgmtInfo(CcnLiteRelay::info_cs_data_s));
        }

        cs_data = (struct CcnLiteRelay::info_cs_data_s *) mgmt_cs;
        cs_data->data_bytes = (unsigned char *) chunkPtrs[i];
        cs_data->data_len = chunkSize;
        cs_data->suite = (CcnLiteRelay::icn_suite_t) suite_id;
        cs_data->version_seqn = 0;

        // two options here (and in requestContent(), either embed the chunk id in the
        // name (under some app specific convention) and ccn-lite will be oblivious to
        // the presence of a chunk id, or explicitly specify a chunk id and ccn-lite
        // will know about it (and maybe do intelligent things).
        //
        // here is the former approach (enable with macrodef EMBED_CHUNKS_IN_NAMES)
#ifdef EMBED_CHUNKS_IN_NAMES
        char * name_with_chunk_buf = &listof_chunk_names[(name_len+_CHUNK_POSTFIX_MAXCHAR+1) * i];
        strcpy(name_with_chunk_buf, name_buf);  // copy name at an integer offset of name_len+_CHUNK_POSTFIX_MAXCHAR+1
        sprintf(name_with_chunk_buf + name_len, "/c%d", i+startChunk); // append postfix with chunk num (embedded in the same string)
        cs_data->name = name_with_chunk_buf;
        cs_data->chunk_seqn = -1;
#else
        // and here is the latter approach
        /*
        cs_data->name = name_buf;
        cs_data->chunk_seqn = i+startChunk;
        */
#endif //EMBED_CHUNKS_IN_NAMES


        mgmt_cs->relay = active_relay->state;
        mgmt_cs->next =0;
    }

    // add content object to CS
    chunks_added = CcnLiteRelay::addMgmt(all_mgmt_cs);

    if (chunks_added == -1) {
        opp_error(" Adding content for node %s failed: ERROR reported by addMgmt()!", active_relay->node_name.c_str());
    }

    // free (named_data);  // uapi frees only what it has itself allocated
    CcnLiteRelay::freeMgmtInfo(all_mgmt_cs);
#ifdef EMBED_CHUNKS_IN_NAMES
    free (listof_chunk_names);
#endif

    if ((numChunks - chunks_added) > 0)
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << chunks_added << " of " << numChunks << " were added to relay cache!"
            << std::endl;

    return chunks_added;
};




/*
 * add a forwarding rule based on a L2 ID (MAC) to relay's FIB
 */
bool
CcnCore::addL2FwdRule (const char *contentName, const char *peerAddr, int localNetifIndex)
{
    struct CcnLiteRelay::info_fib_rule_s    *fib_rule;
    struct CcnLiteRelay::info_mgmt_s        *mgmt_cs;
    int                                     res;
    char                                    peerAddrStr [20];
    char                                    *name, *name_buf;
    int                                     suite_id;

    assert (ctrlBlock);
    active_relay = (struct ccnl_omnet_s *) ctrlBlock;

    assert (contentName);
    assert (peerAddr);

    sprintf (peerAddrStr, "%2.2X-%2.2X-%2.2X-%2.2X-%2.2X-%2.2X",
            (unsigned char) peerAddr[0], (unsigned char) peerAddr[1], (unsigned char) peerAddr[2],
            (unsigned char) peerAddr[3], (unsigned char) peerAddr[4], (unsigned char) peerAddr[5]);

    name = name_buf = strdup(contentName);
    suite_id = extractSuiteFromName (&name, NULL);

    if (suite_id == -1) {
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "In adding fwd rule for " << contentName << ", encountered a problem: UNKNOWN SUITE specified!"
            << std::endl;
        free (name_buf);    // strduped contentName
        return false;
    } else if (suite_id == 0) // no suite provided, default to ccnb
        suite_id = CcnLiteRelay::CCN_XMLB;


    // prep the mgmt info
    mgmt_cs = CcnLiteRelay::mallocMgmtInfo(CcnLiteRelay::info_fib_rule_s);
    fib_rule = (struct CcnLiteRelay::info_fib_rule_s *) mgmt_cs;
    fib_rule->base.relay = active_relay->state;
    fib_rule->base.next=0;
    fib_rule->prefix = name;
    fib_rule->dev_id = localNetifIndex;
    fib_rule->next_hop.linklayer.sll_family = AF_PACKET;
    fib_rule->suite = (CcnLiteRelay::icn_suite_t) suite_id;
    memcpy(fib_rule->next_hop.linklayer.sll_addr, peerAddr, ETH_ALEN);

    // add rule to FIB
    res = CcnLiteRelay::addMgmt(mgmt_cs);

    free(name_buf);
    CcnLiteRelay::freeMgmtInfo(mgmt_cs);

    if (res == -1) {
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "Installation of fwd rule: " << contentName << "-->" << peerAddrStr
            << " at relay FIB, encountered an ERROR!"
            << std::endl;

        opp_error(" Adding FIB rule for node %s encountered an ERROR when calling addMgmt()!", active_relay->node_name.c_str());
    } else if (res == 0) {
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << "Installation of fwd rule: " << contentName << "-->" << peerAddrStr
            << " at relay FIB, FAILED! .. Rule not added"
            << std::endl;

        return false;
    }

    assert (res==1);

    DBGPRT(EVAUX, Info, this->ccnModule->getFullPath())
            << "Installation of fwd rule: " << contentName << "-->" << peerAddrStr
            << " at relay FIB, Succeeded!"
            << std::endl;
    return true;
}




/*
 * Extract the suite info (as a string) from a name. Takes a string that
 * possibly contains an suite identifier in specification of <suite>:<name>,
 * and returns the string tokenised at the beginning of the content name and
 * the beginning of the suite name. It returns a positive int identifying the
 * suite code, 0 if no suite name was parsed, and or -1 if no known suite name
 * was found.
 */
int
CcnCore::extractSuiteFromName(INOUT char ** contentName, OUT char ** suiteStr)
{
    char        *delimiter, *s;
    int         id;

    assert(contentName);

    delimiter = strchr(*contentName, ':');

    if (!delimiter)
        return 0;

    s = strtok(*contentName, ":");
    *contentName = delimiter + 1;

    if (suiteStr)
        *suiteStr = s;

    if (! (id = CcnLiteRelay::suiteStr2Id(s)))
        return -1;
    else
        return id;
}





/*
 * pass request for content to relay
 */
bool
CcnCore::requestContent(const char *contentName, const int seqNum)
{
    struct CcnLiteRelay::info_data_s   *i_obj = NULL;
    char    *tmp_name = 0, *name_buf;
    char    *suite_str = 0;
    int     suite_id;
    int     name_len = 0;
    int     status = 0;

    assert (ctrlBlock);
    active_relay = (struct ccnl_omnet_s *) ctrlBlock;


    tmp_name = name_buf = strdup(contentName);
    suite_id = extractSuiteFromName(&tmp_name, &suite_str);

    switch (suite_id) {
    case -1:
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "Execution of content request " << contentName << " - chunk " << seqNum
            << ", encountered a problem: UNKNOWN SUITE specified!"
            << std::endl;

        free (name_buf);    // for strduped contentName
        return false;

    case CcnLiteRelay::CCN_XMLB:
        i_obj = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ccnXmlb_s);
        ((struct CcnLiteRelay::info_interest_ccnXmlb_s *) i_obj)->nonce = rand();
        break;
    case CcnLiteRelay::CCN_TLV:
        i_obj = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ccnTlv_s);
        ((struct CcnLiteRelay::info_interest_ccnTlv_s *) i_obj)->nonce = rand();
        break;
    case CcnLiteRelay::NDN_TLV:
        i_obj = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ndnTlv_s);
        ((struct CcnLiteRelay::info_interest_ndnTlv_s *) i_obj)->nonce = rand();
        break;
    default:    // 0 : no suite info provided within the name
        i_obj = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ccnXmlb_s);
        ((struct CcnLiteRelay::info_interest_ccnXmlb_s *) i_obj)->nonce = rand();

        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << "Execution of content request " << contentName << " - chunk " << seqNum
            << ", has not specified an ICN suite: will assume the default suite "
            << CcnLiteRelay::printSuite(i_obj)
            << std::endl;
        break;
    }

    // move name to the beginning of buffer
    name_len = strlen(tmp_name);
    if (suite_id != 0) {
        memcpy(name_buf, tmp_name, name_len + 1);   // overwrite the suite prefix by shifting the name string to the beginning of the buffer
    }


    // two options here (and in addToCacheFixedSizeChunks()), either embed the chunk
    // id in the name (under some app specific convention) and ccn-lite will be oblivious
    // to the presence of a chunk id, or explicitly specify a chunk id and ccn-lite will
    // know about it (and maybe do intelligent things).
    //
    // here is the former approach
#ifdef EMBED_CHUNKS_IN_NAMES
    if (seqNum >= 0)
    {
        name_buf = (char *) realloc (name_buf, name_len + _CHUNK_POSTFIX_MAXCHAR + 1);
        sprintf((name_buf + name_len), "/c%d", seqNum);
        i_obj->name = name_buf;
    }
    else    // no chunk index given
    {
        i_obj->name = name_buf;
    }
    i_obj->chunk_seqn = -1;
#else
    // and here is the latter approach
    i_obj->name = name_buf;
    if (seqNum >= 0)
        i_obj->chunk_seqn = seqNum;
    else
        i_obj->chunk_seqn = -1;
#endif // EMBED_CHUNKS_IN_NAMES


    status = CcnLiteRelay::createPacketI(i_obj);
    switch (status)
    {
    case -1:    // failure
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "Call to createPacketI() in ccn-lite produced and Error when requested to create and I pkt with " << CcnLiteRelay::printSuite(i_obj) << " for " << i_obj->name << " - c:" << seqNum << std::endl;
        opp_error(" Attempt to create an Interest packet by node %s encountered an ERROR when calling createInterest()!", active_relay->node_name.c_str());
        break;
    case 0:     // abort
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
        << "Failed to create and I packet with " << CcnLiteRelay::printSuite(i_obj) << " for " << i_obj->name << " - c:" << seqNum << " (call to createPacketI() in ccn-lite)" << std::endl;
        break;
    default:    // all good

        status = CcnLiteRelay::processSuiteData(active_relay->state, i_obj, 0, 0);
        if ( status >= 0)
            DBGPRT(EVAUX, Info, this->ccnModule->getFullPath())
                << "Execution of request for content with " << CcnLiteRelay::printSuite(i_obj) << " for " << i_obj->name << " - c:" << seqNum << " successfully processed!" << std::endl;
        else
            DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
                << "Execution of request for content with " << CcnLiteRelay::printSuite(i_obj) << " for " << i_obj->name << " - c:" << seqNum << " failed at processSuiteData() in ccn-lite; with code" << status << std::endl;
        break;
    }

    free (name_buf);
    CcnLiteRelay::freeSuiteInfo (i_obj);

    if (status >=0)
        return true;
    return false;
};





/*
 * pass CCN packet received from the MAC layer to the relay
 */
bool
CcnCore::fromMACFace(CcnContext *ccnCtx, int arrNetIf, CcnPacket *ccnPkt)
{
    CcnLiteRelay::sockunion             sun;
    struct CcnLiteRelay::info_data_s    *obj_info = NULL;
    std::string                         rprtInfo;
    int                                 rxIntfIndex;
    int                                 buffSize    = ccnPkt->getByteArray().getDataArraySize();
    char                                *data       = new char [buffSize];
    int                                 len         = ccnPkt->copyDataToBuffer(data, buffSize);
    int                                 typeIorC    = ccnPkt->getMsgPayloadType();
    int                                 suite       = ccnPkt->getSuiteType();


    assert (typeIorC == MSG_TYPE_INTEREST || typeIorC == MSG_TYPE_CONTENT);
    assert (ctrlBlock);
    active_relay = (struct ccnl_omnet_s *) ctrlBlock;

    switch (suite) {
    case SUITE_CCNx0:
        rprtInfo += "[CCNx0/";

        if (typeIorC == MSG_TYPE_INTEREST) {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ccnXmlb_s);
            rprtInfo += "I]";
        }
        else {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_content_ccnXmlb_s);
            rprtInfo += "C]";
        }

        break;
    case SUITE_CCNx1:
        rprtInfo += "[CCNx1/";

        if (typeIorC == MSG_TYPE_INTEREST) {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ccnTlv_s);
            rprtInfo += "I]";
        }
        else {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_content_ccnTlv_s);
            rprtInfo += "C]";
        }

        break;
    case SUITE_NDNx0:
        rprtInfo += "[NDNx0/";

        if (typeIorC == MSG_TYPE_INTEREST) {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_interest_ndnTlv_s);
            rprtInfo += "I]";
        }
        else {
            obj_info = CcnLiteRelay::mallocSuiteInfo (CcnLiteRelay::info_content_ndnTlv_s);
            rprtInfo += "C]";
        }

        break;
    default:
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Incoming eth pkt was not delivered to CCN-lite relay: Omnet wrapper does not know the used ICN suite!"
            << std::endl;

        opp_error("Block simulation at module %s, who saw packets using an an unknown ICN suite (Omnet wrapper level).", this->ccnModule->getFullPath().c_str());
        return false;
    };

    /*
     * TODO: at the moment we consume only data coming from an ethernet interface.
     * Extend for socket interfaces too..
     */

    sun.sa.sa_family = AF_PACKET;
    ccnCtx->getSrcAddress802().getAddressBytes(sun.linklayer.sll_addr);
    for (int i=0 ; i< this->ccnModule->numMacIds ; i++) {
        if ( this->ccnModule->macIds[i] == ccnCtx->getDstAddress802() )
        {
            rxIntfIndex = i;
            break;
        }
    }
    obj_info->packet_bytes = (unsigned char *) data;
    obj_info->packet_len =len;


    DBGPRT(EVAUX, Info, this->ccnModule->getFullPath())
            << "Passing to CCN-lite core incoming CCN packet " << rprtInfo
            << " received over Ethernet at interface " << arrNetIf
            << " from " << ccnCtx->getSrcAddress802().str()
            << " for " << ccnCtx->getDstAddress802().str()
            << std::endl;

    int res = CcnLiteRelay::processSuiteData(active_relay->state, obj_info, &sun, rxIntfIndex);

    if (res == -1) {
        DBGPRT(EVAUX, Err, this->ccnModule->getFullPath())
            << "CCN-lite core reported unsupported socket family. "
            << "ERROR processing packet with processObject()!"
            << std::endl;
    } else if (res == 0) {
        DBGPRT(EVAUX, Warn, this->ccnModule->getFullPath())
            << "CCN-lite core reported Null request. "
            << "Processing packet by processObject() was not possible!"
            << std::endl;
    }

    delete data;
    return true;
};




/*
 * pass CCN packet from relay to MAC layer
 */
bool
CcnCore::toMACFace (const char *dstAddr, const char *srcAddr, const char typeIorC, const char suite, const char *name, int chunk, void *data, int len)
{
    char    chunk_str[10];

    if ( !dstAddr || !srcAddr) {
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Outgoing eth pkt dropped (src/dst endpoint not provided)!"
            << std::endl;
        return false;
    }

    if ( len && !data) {
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "Outgoing eth pkt dropped (no packet buffer provided)!"
            <<std::endl;
        return false;
    }

    CcnContext *ctx = new CcnContext(AF_PACKET);
    ctx->set802Info(srcAddr, dstAddr);

    std::string pktInfo;

    switch (suite) {
    case SUITE_CCNx0: pktInfo += "[CCN_XMLB/"; break;
    case SUITE_CCNx1: pktInfo += "[CCN_TLV/"; break;
    case SUITE_NDNx0: pktInfo += "[NDN_TLV/"; break;
    default:
        delete ctx;
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "In outgoing eth pkt from CCN-lite relay: OMNet wrapper does not know the used ICN suite!"
            << std::endl;

        opp_error("Module %s, received from CCN-lite packets of an unknown ICN suite (OMNnet wrapper level).", this->ccnModule->getFullPath().c_str());
        return false;
    };

    switch (typeIorC) {
    case MSG_TYPE_INTEREST: pktInfo += "I]"; break;
    case MSG_TYPE_CONTENT: pktInfo += "C]"; break;
    default:
        delete ctx;
        DBGPRT(AUX, Err, this->ccnModule->getFullPath())
            << "In outgoing eth pkt from CCN-lite relay: OMNet CCN-lite wrapper cannot handle messages other than Interest or Content/Data!"
            << std::endl;

        opp_error("Module %s, received from CCN-lite ICN packets other than Interest or Content/Data (Omnet wrapper level).", this->ccnModule->getFullPath().c_str());
        return false;
    }

    sprintf (chunk_str, "%d", chunk);
    pktInfo += ", From:" + ctx->getSrcAddress802().str()
            + ", To:" + ctx->getDstAddress802().str()
            + ", Naming Obj:" + std::string(name)
            + ", c:" + std::string(chunk_str);

    CcnPacket *pkt = new CcnPacket ();
    pkt->setName(pktInfo.c_str());
    pkt->setKind(PROT_CCN);
    pkt->setDataFromBuffer(data, len);
    pkt->addByteLength(len);
    pkt->setContextPointer(ctx);
    pkt->setMsgPayloadType(typeIorC);    //
    pkt->setSuiteType(suite);       //

    ccnModule->toLowerLayer(pkt);
    return true;
};




/*
 * register or delete a timer at the owning Ccn module
 */
long
CcnCore::manageTimerEvent (char setOrReset, long usecOrHandle, void(*callback)(void *, void *), void * par1, void * par2)
{
    switch (setOrReset)
    {
    case 1:
        return ccnModule->setTimerEvent (usecOrHandle, callback, par1, par2); // usecOrHandle ~ usec

    case 0:
        return ccnModule->clearTimerEvent(usecOrHandle);    // usecOrHandle ~ handle

    default:
        DBGPRT(AUX, Err, ccnModule->getFullName())
            << "Invalid timer operation (not a timer event registration or removal request)!"
            << std::endl;
        break;
    }

    return -1;
};




/*
 * deliver content from the relay to the layer above (app)
 */
bool
CcnCore::deliverContent (char *contentName, int seqNum, void *data, int len)
{
    if ( !contentName ) {
        DBGPRT(EVAUX, Warn, ccnModule->getFullPath())
                << "Dropped content data, no associated content name/chunk identifier!"
                <<std::endl;
        return false;
    }

    if ( len && !data) {
        DBGPRT(AUX, Warn, ccnModule->getFullPath())
                << "Dropped content chunk with inconsistent content data buffer!"
                << std::endl;
        return false;
    }

    CcnAppMessage *appPkt = new CcnAppMessage;
    appPkt->setType(CCN_APP_CONTENT);
    appPkt->setAction(CONTENT_DELIVER);
    appPkt->setContentName(contentName);
    appPkt->setSeqNr(seqNum);
    appPkt->setDataFromBuffer(data, len);
    appPkt->addByteLength(len);
    appPkt->setArrivalTime(simTime());

    ccnModule->toUpperLayer(appPkt);

    return true;
};




/*
 * Update relay configuration (e.g. cache store size, cache policy, tx pace etc)
 */
bool
CcnCore::updateRelayConfig(void *)
{
    // TODO: Implement relay control block update/peek interface
    DBGPRT(EVAUX, Warn, ccnModule->getFullPath())
            << "UNDER CONSTRUCTION: function not implemented! \n"
            << "Cache store size in i-nodes can currently be set only at relay instantiation \n"
            << "Cache policy is fixed to FIFO \n"
            << "Tx pace can currently be set only at relay instantiation"
            << std::endl;
    return true;
};
