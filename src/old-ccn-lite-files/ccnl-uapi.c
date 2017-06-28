/*
 * ccnl-uapi.c
 *
 *  Created on: Nov 4, 2014
 *      Author: manolis
 *
 *  Updated for ccn-lite > v0.1.0: Jun 4, 2015 (manolis)
 */

#include <assert.h>
#include "ccnl-uapi.h"
#include "ccnl-common.c"



/*****************************************************************************
 * Forward decls
 *****************************************************************************/

/*
 * per info struct ctors and dtors
 *
 * FIXME: Implement at least the dtors so as to make sure all the memory pointed
 * by data members is also free (atm I think I take care of everything manually, but
 * would be better to be done automatically when freeing the data/mgmt object
 * instance
 *
 * they are supposed to take care of creating and
 * initialising the memory layout of the different
 * types of info objects
 */
//ctors
void * (* info_interest_ndnTlv_ctor)(void *o) = 0;
void * (* info_content_ndnTlv_ctor)(void *o) = 0;
void * (* info_interest_ccnTlv_ctor)(void *o) = 0;
void * (* info_content_ccnTlv_ctor)(void *o) = 0;
void * (* info_interest_ccnXmlb_ctor)(void *o) = 0;
void * (* info_content_ccnXmlb_ctor)(void *o) = 0;
void * (* info_cs_data_ctor)(void *o) = 0;
void * (* info_fib_rule_ctor)(void *o) = 0;
void * (* info_iface_conf_ctor)(void *o) = 0;
//dtors
void (* info_interest_ndnTlv_dtor)(void *o) = 0;
void (* info_content_ndnTlv_dtor)(void *o) = 0;
void (* info_interest_ccnTlv_dtor)(void *o) = 0;
void (* info_content_ccnTlv_dtor)(void *o) = 0;
void (* info_interest_ccnXmlb_dtor)(void *o) = 0;
void (* info_content_ccnXmlb_dtor)(void *o) = 0;
void (* info_cs_data_dtor)(void *o) = 0;
void (* info_fib_rule_dtor)(void *o) = 0;
void (* info_iface_conf_dtor)(void *o) = 0;


#ifndef PLATFORM_SRVCS
//FIXME: for now we silence this because the ccn-lite code
//already has hard-coded defaults for different OS platforms.
//Better make this complain when not set by a platform though.
struct platform_vt_s platform_srvcs = {};
#endif


/*
 * forward declarations of the functions that will be used to populate the vtables
 * of the various type objects
 */
int ifaces_add(struct info_mgmt_s *mgmt);
int content_add(struct info_mgmt_s *mgmt);
int fwd_rule_add(struct info_mgmt_s *mgmt);
int mk_ccnb_interest(struct info_data_s *o);
int mk_ndntlv_interest(struct info_data_s *o);
int mk_ccntlv_interest(struct info_data_s *o);
int mk_ccnb_content(struct info_data_s *o);
int mk_ndntlv_content(struct info_data_s *o);
int mk_ccntlv_content(struct info_data_s *o);





/*****************************************************************************
 * vtables describing the object types 
 * (here they are initialised and chained together to form a hierarchy)
 *****************************************************************************/

/*
 * vtables of suite type objects (to be populated)
 */

//ndntlv
const struct suite_vt_s ndnTlv_vt = {
        mk_ndntlv_interest,
        mk_ndntlv_content,
        0,0
    // TODO: insert fptrs: rdInterest/rdContent
};

//ccntlv
const struct suite_vt_s ccnTlv_vt = {
        mk_ccntlv_interest,
        mk_ccntlv_content,
        0,0
        // TODO: insert fptrs: rdInterest/rdContent
};

//ccnb
const struct suite_vt_s ccnXmlb_vt = {
        mk_ccnb_interest,
        mk_ccnb_content,
        0,0
        // TODO: insert fptrs: rdInterest/rdContent
};




/*
 * vtables of I/C type objects per suite (fixed vtables with preset values)
 */

// for attaching to interest_info_ndnTlv_s
const struct info_data_vt_s   info_interest_ndnTlv_vt = {
    sizeof (struct info_interest_ndnTlv_s),
    I_NDNx_0,
    NDN_TLV,
    &ndnTlv_vt,
    info_interest_ndnTlv_ctor,
    info_interest_ndnTlv_dtor
};

// for attaching to content_info_ndnTlv_s
const struct info_data_vt_s   info_content_ndnTlv_vt = {
    sizeof (struct info_content_ndnTlv_s),
    D_NDNx_0,
    NDN_TLV,
    &ndnTlv_vt,
    info_content_ndnTlv_ctor,
    info_content_ndnTlv_dtor
};

// for attaching to interest_info_ccnTlv_s
const struct info_data_vt_s   info_interest_ccnTlv_vt = {
    sizeof (struct info_interest_ccnTlv_s),
    I_CCNx_1,
    CCN_TLV,
    &ccnTlv_vt,
    info_interest_ccnTlv_ctor,
    info_interest_ccnTlv_dtor
};

// for attaching to content_info_ccnTlv_s
const struct info_data_vt_s   info_content_ccnTlv_vt = {
    sizeof (struct info_content_ccnTlv_s),
    C_CCNx_1,
    CCN_TLV,
    &ccnTlv_vt,
    info_content_ccnTlv_ctor,
    info_content_ccnTlv_dtor
};

// for attaching to interest_info_ccnXmlb_s
const struct info_data_vt_s   info_interest_ccnXmlb_vt = {
    sizeof (struct info_interest_ccnXmlb_s),
    I_CCNx_0,
    CCN_XMLB,
    &ccnXmlb_vt,
    info_interest_ccnXmlb_ctor,
    info_interest_ccnXmlb_dtor
};

// for attaching to content_info_ccnXmlb_s
const struct info_data_vt_s   info_content_ccnXmlb_vt = {
    sizeof (struct info_content_ccnXmlb_s),
    C_CCNx_0,
    CCN_XMLB,
    &ccnXmlb_vt,
    info_content_ccnXmlb_ctor,
    info_content_ccnXmlb_dtor
};


/*
 * vtables of managment type objects (to be populated)
 */

// CS data
const struct info_mgmt_vt_s info_cs_data_vt = {
        sizeof(struct info_cs_data_s),
        MGMT_CS,
        info_cs_data_ctor,
        info_cs_data_dtor,

        // TODO: assign fptrs for rem/set/get content
        content_add,
        0,0,0
};

// FIB rules
const struct info_mgmt_vt_s info_fib_rule_vt = {
        sizeof (struct info_fib_rule_s),
        MGMT_FIB,
        info_fib_rule_ctor,
        info_fib_rule_dtor,

        // TODO: assign fptrs for rem/set/get fib rules
        fwd_rule_add,
        0,0,0
};

// IFACE conf
const struct info_mgmt_vt_s info_iface_conf_vt = {
        sizeof (struct info_iface_conf_s),
        MGMT_IFACE,
        info_iface_conf_ctor,
        info_iface_conf_dtor,

        // TODO: assign fptrs for rem/set/get interfaces
        ifaces_add,
        0,0,0
};





/*****************************************************************************
 * named type definitions for type objects
 *****************************************************************************/

/*
 * named type definitions for use in mallocSuiteInfo() and other generic
 * data functions, e.g.
 *
 * mallocSuiteInfo(info_interest_ndnTlv_s) creates a struct of type info_interest_ndnTlv_s
 */
const void * info_interest_ndnTlv_s = &info_interest_ndnTlv_vt;
const void * info_content_ndnTlv_s = &info_content_ndnTlv_vt;
const void * info_interest_ccnTlv_s = &info_interest_ccnTlv_vt;
const void * info_content_ccnTlv_s = &info_content_ccnTlv_vt;
const void * info_interest_ccnXmlb_s = &info_interest_ccnXmlb_vt;
const void * info_content_ccnXmlb_s = &info_content_ccnXmlb_vt;
/*
 * named type definitions for use in mallocMgmtInfo() and other generic
 * mgmt functions
 */
const void * info_cs_data_s = &info_cs_data_vt;
const void * info_fib_rule_s = &info_fib_rule_vt;
const void * info_iface_conf_s = &info_iface_conf_vt;





/*****************************************************************************
 * client/user side UAPI implementation
 *
 * generic function definitions for all suites
 *****************************************************************************/

/*
 * from a suite string specified as ccnb/ccntlv/ndntlv (lowercase or CAPITAL)
 * return the corresponding internal icn_suite_t id
 */
int
suiteStr2Id(char *s)
{
    assert(s);

    char *t = strdup (s);

    for (int i = 0 ; t[i] != '\0' ; i++) t[i] = tolower (t[i]);

    for (int i = 1 ; strcmp("N/A", CcnLiteRelay::icn_suite_str_by_fmt[i]) != 0; i++)
        if (strcmp(s, CcnLiteRelay::icn_suite_str_by_fmt[i]) == 0) {
            free (t);
            return i;
        };

    free (t);
    return 0;
};


/*
 * allocate an info struct for an I/C object and init memory layout
 */
struct info_data_s *
mallocSuiteInfo (const void *obj_descr)
{
    struct info_data_vt_s const *obj_vt = (struct info_data_vt_s const *) obj_descr;

    void *obj = ccnl_calloc (1, obj_vt->size);

    assert(obj);
    memset (obj, 0 , obj_vt->size);

    // attach object vt (automatically embeds the vt of the suite as well)
    * ((struct info_data_vt_s const **) obj) = obj_vt;

    // call ctor to allocate the rest of the memory layout
    if (obj_vt->ctor)
        obj = obj_vt->ctor (obj);

    return (struct info_data_s *) obj;
};



/*
 * free the info struct that describes an I/C object and destroy memory layout
 */
void
freeSuiteInfo (struct info_data_s *o)
{
    struct info_data_vt_s const * obj_vt = o->vtable;

    // erase info struct mem layout
    if (o && obj_vt && obj_vt->dtor )
        obj_vt->dtor(o);

    ccnl_free(o);

    return;
};


/*
 * hook for passing an I/C suite-formatted obj to ccn-lite
 */
int
processSuiteData (void *relay, struct info_data_s *o, sockunion * from_addr /*0 if local*/, int rx_iface /*0 if generated locally*/)
{
    int addr_len = 0;

    if (!o)         // nothing to be send
        return 0;

    // TODO: Ask Chris if this is suite dependent or generic for all suites
    // if suite dependent the correct sending function should be selected
    // based on suite info attached to the info_data_s

    if (! from_addr)     // locally generated
    {
        ccnl_core_RX((struct ccnl_relay_s *) relay, -1, (unsigned char*) o->packet_bytes, o->packet_len, 0, 0);
    }
    else            // received from some socket addr
    {
        switch (from_addr->sa.sa_family)
        {
#ifdef USE_LINKLAYER
        case AF_PACKET:
            addr_len = sizeof(from_addr->linklayer);
            break;
#endif
#ifdef USE_WPAN
        case AF_IEEE802154:
            addr_len = sizeof(from_addr->wpan);
            break;
#endif
#ifdef USE_IPV4
        case AF_INET:
            addr_len = sizeof(from_addr->ip4);
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            addr_len = sizeof(from_addr->ip6);
            break;
#endif
#ifdef USE_UNIXSOCKET
        case AF_UNIX:
            addr_len = sizeof(from_addr->ux);
            break;
#endif
        default:
            return -1;  // error: unsupported socket family
        }

        ccnl_core_RX((struct ccnl_relay_s *) relay, rx_iface, (unsigned char*) o->packet_bytes, o->packet_len, &(from_addr->sa), addr_len);
    }

    return 1;   // success: one packet processed
};


/*
 * parse an I suite-formatted obj and gen respective info struct
 * TODO: implement
 */
struct info_data_s *
parseInterest(char *pkt_buf, int pkt_len)
{
    return 0;
};


/*
 * parse a C suite-formatted obj and gen respective info struct
 * TODO: implement
 */
struct info_data_s *
parseContent(char *pkt_buf, int pkt_len)
{
    return 0;
};



/*
 * create state for a new relay
 */
void *
createRelay (const char * node_name, int cs_inodes, int cs_bytesize, char enable_stats, void *aux)
{
    struct ccnl_relay_s * r = (struct ccnl_relay_s *) ccnl_calloc (1, sizeof(struct ccnl_relay_s));

    if (! r)
        return 0;

    ccnl_core_init();

    // config
    r->max_cache_entries = cs_inodes;
    // TODO: set somewhere the cs storage size
    // TODO: save somewhere a str name for the relay
    r->aux = aux;       // optional extension to the relay control block

#ifdef USE_SCHEDULER
    // scheduling at the face level
    relay->defaultFaceScheduler = ccnl_relay_defaultFaceScheduler;
    relay->defaultInterfaceScheduler = ccnl_relay_defaultInterfaceScheduler;
#else //!USE_SCHEDULER
    // no scheduling at the face level (only at the interface level for tx pacing)
    r->defaultFaceScheduler = NULL;
#endif //USE_SCHEDULER

#ifdef USE_SIGNATURES
    // TODO: ...
#endif //USE_SIGNATURES

#ifdef USE_STATS
    if (enable_stats) {
        if (platform_srvcs.log_start)
            platform_srvcs.log_start (r, (char *) node_name);
    }
    else
    {
        DEBUGMSG(99, "ccnl_create_relay on node %s -- stats logging not activated \n", node_name);
    }
#endif //USE_STATS

    return (void *) r;
};




/*
 *  dispose relay state and free mem
 */
void
destroyRelay (void *relay)
{
    struct ccnl_relay_s *r = (struct ccnl_relay_s *) relay;

#ifdef USE_STATS
    if (r->stats) {
        if (platform_srvcs.log_stop)
            platform_srvcs.log_stop (r);

        ccnl_free(r->stats);
        r->stats = NULL;
    }
#endif //USE_STATS

    ccnl_core_cleanup(r);

#ifdef CCNL_DEBUG_MALLOC
    debug_memdump();
#endif

    ccnl_free(r);

    return;
};



/*
 * start relay op with remembered state
 */
void
startRelay (void *relay) {

    if (!relay) return;
    ccnl_do_ageing(relay, 0);
    ccnl_set_timer(1000000, ccnl_do_ageing, relay, 0);
    return;
};



/*
 * pause relay op but do not dispose state
 * TODO: implement
 */
void
stopRelay (void *relay) {return;};



/*
 * create a mgmt info struct mem layout and init
 */
struct info_mgmt_s *
mallocMgmtInfo (const void *mgmt_descr)
{
    struct info_mgmt_vt_s const *   mgmt_vt= (struct info_mgmt_vt_s const *) mgmt_descr;
    struct info_mgmt_s *            mgmt = (struct info_mgmt_s *) ccnl_calloc(1, mgmt_vt->size);

    assert(mgmt);
    memset (mgmt, 0 , mgmt_vt->size);

    mgmt->vtable = mgmt_vt;

    if (mgmt_vt->ctor)
        mgmt = (struct info_mgmt_s *) mgmt_vt->ctor((void *)mgmt);

    return mgmt;
};



/*
 * delete mem layout of ONE or MORE chained mgmt info structs
 */
void
freeMgmtInfo (struct info_mgmt_s *m)
{
    struct info_mgmt_s *current = 0;
    struct info_mgmt_s *remaining = 0;

    remaining = current = m;

    while (remaining) {
        current = remaining;
        remaining = current->next;

        if (current->vtable && current->vtable->dtor)
            current->vtable->dtor (current);

        current->vtable = 0;
        ccnl_free (current);
    };

    return;
};



/****************************************************************************
 * ccn-lite side of the UAPI
 *
 * fixed callbacks that the ccn-lite kernel uses to deliver information to
 * the host platform. They are wired through the platform_srvcs vt API
 *
 * TODO: Probably some of these need to become more chatty in the
 * reporting their progress
 ****************************************************************************/


/*
 * Packet to be delivered south (to the link layer). Create an information object
 * and pass it to the host platform through the deliverObject callback from the
 * API vtables on the platform side
 */
void
ccnl_ll_TX(
            struct ccnl_relay_s *relay,
            struct ccnl_if_s *ifc,          // fwd "phys" interface
            sockunion *dst,                 // next-hop pkt recipient
            struct ccnl_buf_s *buf)         // formatted pkt
{
    struct envelope_s *     env;
    struct info_data_s *    io;

    unsigned char *         data = (unsigned char *) buf->data;
    int                     datalen = buf->datalen;
    struct ccnl_pkt_s *     pkt;
    unsigned int            i_or_c;
    int                     aux_arg;

    assert(relay && ifc && buf && dst);

    // determine suite and extract the name information
    switch (ccnl_pkt2suite(buf->data, buf->datalen, &aux_arg))
    {

    // *** handling peeped from ccnl-core-fwd.c ***

    case CCNL_SUITE_CCNB:
        // extract I or C info
        if (ccnl_ccnb_dehead(&data, &datalen, (int *) &i_or_c, &aux_arg) || aux_arg != CCN_TT_DTAG) {
            DEBUGMSG(WARNING, "Not a valid CCN_XMLB message, although a CCN_XMLB suite was indicated \n");
            return;
        }

        // read pkt
        pkt = ccnl_ccnb_bytes2pkt (data - 2, &data, &datalen);    // -2: ??? (see ccnl-core-fwd.c)
        if (!pkt) {
            DEBUGMSG(WARNING, "Not a valid CCN_XMLB message, parsing error or no prefix\n");
            return;
        }

        switch (i_or_c) {
        case CCN_DTAG_INTEREST:
            pkt->flags |= CCNL_PKT_REQUEST;
            io = mallocSuiteInfo(info_interest_ccnXmlb_s);
            break;
        case CCN_DTAG_CONTENTOBJ:
            pkt->flags |= CCNL_PKT_REPLY;
            io = mallocSuiteInfo(info_content_ccnXmlb_s);
            break;
        default:
            DEBUGMSG(WARNING, "Neither an I nor C packet, although a CCN_XMLB suite was indicated \n");
            free_packet(pkt);
            return;
        }

        pkt->type = i_or_c; // not sure why this is needed, it is already in the flags

        break;
    case CCNL_SUITE_CCNTLV:
        // extract I or C info
        i_or_c = ((struct ccnx_tlvhdr_ccnx2015_s*) buf->data)->pkttype;
        if (i_or_c != CCNX_PT_Interest && i_or_c != CCNX_PT_Data) {
            DEBUGMSG(WARNING, "Not a valid CCN_TLV message, although a CCN_TLV suite was indicated \n");
            return;
        }

        // ??? (see ccnl-core-fwd.c)
        data += ((struct ccnx_tlvhdr_ccnx2015_s*) buf->data)->hdrlen;
        datalen -= ((struct ccnx_tlvhdr_ccnx2015_s*) buf->data)->hdrlen;

        // parse pkt
        pkt = ccnl_ccntlv_bytes2pkt(buf->data, &data, &datalen);
        if (!pkt) {
            DEBUGMSG(WARNING, "Not a valid CCN_TLV message, parsing error or no prefix\n");
            return;
        }

        switch (i_or_c) {
        case CCNX_PT_Interest:
            pkt->flags |= CCNL_PKT_REQUEST;
            io = mallocSuiteInfo(info_interest_ccnTlv_s);
            break;
        case CCNX_PT_Data:
            pkt->flags |= CCNL_PKT_REPLY;
            io = mallocSuiteInfo(info_content_ccnTlv_s);
            break;
        default:
            DEBUGMSG(WARNING, "Neither an I nor C packet, although a CCN_TLV suite was indicated \n");
            free_packet(pkt);
            return;
        }

        break;
    case CCNL_SUITE_NDNTLV:
        // extract I or C info
        if (ccnl_ndntlv_dehead(&data, &datalen, &i_or_c, (unsigned int *) &aux_arg) || aux_arg > datalen) {
            DEBUGMSG(WARNING, "Not a valid NDN_TLV message, although a NDN_TLV suite was indicated \n");
            return;
        }

        // parse pkt
        pkt = ccnl_ndntlv_bytes2pkt(i_or_c, buf->data, &data, &datalen);
        if (!pkt) {
            DEBUGMSG(WARNING, "Not a valid CCN_TLV message, parsing error or no prefix\n");
            return;
        }

        pkt->type = i_or_c;

        switch (i_or_c) {
        case NDN_TLV_Interest:
            pkt->flags |= CCNL_PKT_REQUEST;
            io = mallocSuiteInfo(info_interest_ndnTlv_s);
            break;
        case NDN_TLV_Data:
            pkt->flags |= CCNL_PKT_REPLY;
            io = mallocSuiteInfo(info_content_ndnTlv_s);
            break;
        default:
            DEBUGMSG(WARNING, "Neither an I nor C packet, although a CCN_TLV suite was indicated \n");
            free_packet(pkt);
            return;
        }

        break;
    default:
        DEBUGMSG(ERROR, "Unknown packet format \n");
        return;
    };

    io->name = ccnl_prefix_to_path(pkt->pfx);  // TODO: should be safe since io is const at the deliverObject() call
    io->chunk_seqn = (pkt->pfx->chunknum) ? ((int) *(pkt->pfx->chunknum)) : -1;
    io->packet_bytes = pkt->buf->data;
    io->packet_len = pkt->buf->datalen;

    // envelope for data to be delivered to a link
    env = (struct envelope_s *) ccnl_malloc(sizeof(struct envelope_s));
    env->to = ENV_LINK;
    memcpy (&(env->link.sa_local), &(ifc->addr), sizeof(sockunion));    // local out addr
    memcpy (&(env->link.sa_remote), dst, sizeof(sockunion));            // next hop addr

    // hand in to platform
    platform_srvcs.deliver_object(relay, io, env);

    ccnl_free (env);
    free_packet(pkt);
    freeSuiteInfo(io);

    return;
};




/*
 * Packet to be delivered north (to the local application). Create an
 * information object and pass it to the host platform through the
 * deliverObject callback
 */
void
ccnl_app_RX(
        struct ccnl_relay_s *relay,
        struct ccnl_content_s *c)
{
    struct envelope_s *     env = (struct envelope_s *) ccnl_malloc(sizeof(struct envelope_s));
    struct info_data_s *    io;

    assert (relay && c);

    // Prepare a C data object, and do minimum un-bundling for the content data only -- may
    // require per suite type checking or algorithm validation
    // The client should be able to extract more extensive attribute info from the
    // packet buffer if wished using the parsePacketC() -- (NOTE parsePacketC()
    // is not implemented yet!)
    switch(c->pkt->suite) {
    case CCNL_SUITE_CCNB:
        io = mallocSuiteInfo(info_content_ccnXmlb_s);

        ((struct info_content_ccnXmlb_s *) io)->content_bytes = c->pkt->content;
        ((struct info_content_ccnXmlb_s *) io)->content_len = c->pkt->contlen;
        break;
    case CCNL_SUITE_CCNTLV:
        io = mallocSuiteInfo(info_content_ccnTlv_s);

        ((struct info_content_ccnTlv_s *) io)->content_bytes = c->pkt->content;
        ((struct info_content_ccnTlv_s *) io)->content_len = c->pkt->contlen;
        break;
    case CCNL_SUITE_NDNTLV:
        io = mallocSuiteInfo(info_content_ndnTlv_s);

        ((struct info_content_ndnTlv_s *) io)->content_bytes = c->pkt->content;
        ((struct info_content_ndnTlv_s *) io)->content_len = c->pkt->contlen;
        break;
    default:
        DEBUGMSG(WARNING, "Unknown packet format \n");
        return;
    }

    // attach the pkt buffer to the base info object, and set name/seqn of the content
    io->packet_bytes = c->pkt->buf->data;
    io->packet_len = c->pkt->buf->datalen;
    io->name = ccnl_prefix_to_path(c->pkt->pfx); // TODO: should be safe since io is const at the deliverObject call
    io->chunk_seqn = (c->pkt->pfx->chunknum) ? ((int) *(c->pkt->pfx->chunknum)) : -1;

    // prepare envelope
    env->to = ENV_APP;

    // hand in to platform
    platform_srvcs.deliver_object(relay, io, env);

    freeSuiteInfo(io);
    ccnl_free (env);

    return;
};




/*****************************************************************************
 * function hooks of the UAPI vtables
 *****************************************************************************/


/*
 * add and configure a number of interfaces
 */
int
ifaces_add(struct info_mgmt_s *mgmt)
{
    struct info_iface_conf_s    *ifaces = (struct info_iface_conf_s *) mgmt;
    struct ccnl_relay_s         *r = (struct ccnl_relay_s *) mgmt->relay;
    struct ccnl_if_s            *i;
    int                         cnt=0;

    if (!r)
        return 0;  // no access to the relay state, nothing more to do

    int t;

    for (int k=0 ; ifaces ; k++ , ifaces = (struct info_iface_conf_s *) ifaces->base.next)
    {
        //i = & (r->ifs[k]);
        assert(ifaces->iface_id <= CCNL_MAX_INTERFACES);
        i = & (r->ifs[ifaces->iface_id]);
        t = ifaces->addr.sa.sa_family;

        switch (t)
        {
        case AF_PACKET:
            i->addr.linklayer.sll_family = AF_PACKET;
            memcpy(i->addr.linklayer.sll_addr, ifaces->addr.linklayer.sll_addr, ETH_ALEN);
            i->mtu = 1400;
            i->reflect = 1;
#ifdef  PROPAGATE_INTERESTS_SEEN_BEFORE
            i->fwdalli = 1;
#else
            i->fwdalli = 0;
#endif  // PROPAGATE_INTERESTS_SEEN_BEFORE

#ifdef  USE_SCHEDULER
            i->sched = ccnl_sched_pktrate_new (ccnl_interface_CTS, r, ifaces->tx_pace);
#endif  // USE_SCHEDULER

            cnt++;
            r->ifcount++;	// beware! the caller of ifaces_add() can decide to occupy interface
				            // structs adhocly indexed in the ifs[] of ccnl_relay_s, and therefore
				            //the ifcount var MUST NOT be used to iterate entries of ifs[]

            DEBUGMSG(INFO, "initialized ethernet interface %d for use with faces\n", ifaces->iface_id);
            break;

        // TODO: Add the other cases AF_INET AF_INET6 ...
        case AF_INET:
        case AF_INET6:
        default:
            break;
        }
    };
    return cnt;     // success: number of interfaces created
}





/*
 * add a set of content objects
 */
int
content_add (struct info_mgmt_s *mgmt)
{
    struct info_data_s *        d_obj;
    struct info_cs_data_s *     cs_data = (struct info_cs_data_s *) mgmt;
    int                         cnt=0;
    unsigned char               *data;
    int                         datalen=0, hdrlen=0;
    struct ccnl_pkt_s           *pkt;
    struct ccnl_content_s       *c = 0;


    if (!(struct ccnl_relay_s *)cs_data->base.relay)
        return 0;      // no access to the relay state, nothing more to do

    for (int k=0 ; cs_data ; k++ , cs_data = (struct info_cs_data_s *) cs_data->base.next)
    {
        // FIXME: We assume that each cs_data struct provides us with data that fit in one chunk(!).
        // We must however check and possibly segment the data into multiple content chunks (thereby
        // each cs_data struct may require the generation of a chain of content data objects!). For
        // now this is not here ...

        // pre-format content data in C pkts before storing them in  CS
        switch (cs_data->suite) {
        case CCN_XMLB:

            // prepare a content data object (BEWARE we re talking over the cs_data struct ptrs rather than making copies)
            d_obj = mallocSuiteInfo (info_content_ccnXmlb_s);
            ((struct info_content_ccnXmlb_s *) d_obj)->content_bytes = cs_data->data_bytes;   // !!!! ACHTUNG !!!! we assume data fit in one packet and we don't chunkify(!).
            ((struct info_content_ccnXmlb_s *) d_obj)->content_len = cs_data->data_len;
            ((struct info_content_ccnXmlb_s *) d_obj)->base.name = cs_data->name;
            if (cs_data->chunk_seqn >= 0)
                ((struct info_content_ccnXmlb_s *) d_obj)->base.chunk_seqn = cs_data->chunk_seqn;
            else
                ((struct info_content_ccnXmlb_s *) d_obj)->base.chunk_seqn = -1;

            // generate C packet from content object
            if (!createPacketC (d_obj))
            {
                // failed creating a C packet for the corresponding suite
                DEBUGMSG(WARNING, "Creation of CCNx (XMLB) C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            // prepare a ccnl_pkt_s struct
            // (don't understand the purpose of the next 2 lines .. see ccnl_lite_relay.c:ccnl_populate_content())
            data = d_obj->packet_bytes; data += 2;
            datalen = d_obj->packet_len; datalen -= 2;
            pkt = ccnl_ccnb_bytes2pkt(d_obj->packet_bytes, &data, &datalen);

            if (!pkt) {
                DEBUGMSG(WARNING, "Description generation of CCNx (XMLB) C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            break;
        case CCN_TLV:

            // prepare a content data object (BEWARE we re talking over the cs_data struct ptrs rather than making copies)
            d_obj  = mallocSuiteInfo (info_content_ccnTlv_s);
            ((struct info_content_ccnTlv_s *) d_obj)->content_bytes = cs_data->data_bytes;   // !!!! ACHTUNG !!!! we assume data fit in one packet and we don't chunkify(!).
            ((struct info_content_ccnTlv_s *) d_obj)->content_len = cs_data->data_len;
            ((struct info_content_ccnTlv_s *) d_obj)->base.name = cs_data->name;
            if (cs_data->chunk_seqn >= 0)
                ((struct info_content_ccnTlv_s *) d_obj)->base.chunk_seqn = cs_data->chunk_seqn;
            else
                ((struct info_content_ccnTlv_s *) d_obj)->base.chunk_seqn = -1;

            // generate C packet from content object
            if (!createPacketC(d_obj))
            {
                // failed creating a C packet for the corresponding suite
                DEBUGMSG(WARNING, "Creation of CCNx (TLV) C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            // prepare a ccnl_pkt_s struct
            // (don't understand the purpose of the next 4 lines .. see ccnl_lite_relay.c:ccnl_populate_content())
            data = d_obj->packet_bytes;
            datalen = d_obj->packet_len;
            hdrlen = ccnl_ccntlv_getHdrLen(data, datalen);
            data += hdrlen;
            datalen -= hdrlen;
            pkt = ccnl_ccntlv_bytes2pkt(d_obj->packet_bytes, &data, &datalen);

            if (!pkt) {
                DEBUGMSG(WARNING, "Description generation of CCNx (TLV) C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            break;
        case NDN_TLV:

            // prepare a content data object (BEWARE we re talking over the cs_data struct ptrs rather than making copies)
            d_obj  = mallocSuiteInfo (info_content_ndnTlv_s);
            ((struct info_content_ndnTlv_s *) d_obj)->content_bytes = cs_data->data_bytes;   // !!!! ACHTUNG !!!! we assume data fit in one packet and we don't chunkify(!).
            ((struct info_content_ndnTlv_s *) d_obj)->content_len = cs_data->data_len;
            ((struct info_content_ndnTlv_s *) d_obj)->base.name = cs_data->name;
            if (cs_data->chunk_seqn >= 0)
                ((struct info_content_ndnTlv_s *) d_obj)->base.chunk_seqn = cs_data->chunk_seqn;
            else
                ((struct info_content_ndnTlv_s *) d_obj)->base.chunk_seqn = -1;

            // generate C packet from content object
            if (!createPacketC(d_obj))
            {
                // failed creating a C packet for the corresponding suite
                DEBUGMSG(WARNING, "Creation of NDNx C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            // prepare a ccnl_pkt_s struct for ccnl_content_new()
            // (don't understand the purpose of the next 5 lines .. see ccnl_lite_relay.c:ccnl_populate_content())
            unsigned int type;
            unsigned int length;
            data = d_obj->packet_bytes;
            datalen = d_obj->packet_len;
            ccnl_ndntlv_dehead(&data, &datalen, &type, &length);
            pkt = ccnl_ndntlv_bytes2pkt(type, d_obj->packet_bytes, &data, &datalen);

            if (!pkt) {
                DEBUGMSG(WARNING, "Description generation of NDNx (TLV) C packet for adding content %s to CS, failed\n", cs_data->name);
                freeSuiteInfo(d_obj);
                continue;
            };

            break;
        default:
            DEBUGMSG(WARNING, "Invalid suite provided, will not add content %s to CS\n", cs_data->name);
            continue;
        }

        // store in CS
        if ( (c = ccnl_content_new((struct ccnl_relay_s *)cs_data->base.relay, &pkt)) )
        {
            ccnl_content_add2cache((struct ccnl_relay_s *)cs_data->base.relay, c);  // we should expect a success code here but this func does not return one.

#ifdef CONTENT_NEVER_EXPIRES
            c->flags |= CCNL_CONTENT_FLAGS_STATIC;
#endif
            cnt++;
        }
        freeSuiteInfo(d_obj);
    }
    return cnt;     // return the number of C packets added to the cs
};




/*
 * add a set of forwarding rules
 * TODO: currently based on MAC addrs only, extend for IP addresses too
 */
int
fwd_rule_add(struct info_mgmt_s *mgmt)
{
    struct info_fib_rule_s *    fib_rule = (struct info_fib_rule_s *) mgmt;
    struct ccnl_relay_s *       r = (struct ccnl_relay_s *) mgmt->relay;
    struct ccnl_forward_s *     fwd;
    int                         cnt=0;
    int                         suite_intdef;

    if (!r)
        return 0;  // no access to the relay state, nothing more to do

    for (int k=0 ; fib_rule ; k++ , fib_rule = (struct info_fib_rule_s *) fib_rule->base.next)
    {
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));

        // FIXME: need to add proper support for all available suites and harmonise the enums between UAPI and ccnl-defs.h
        switch (fib_rule->suite) {
        case CCN_XMLB:
            suite_intdef = CCNL_SUITE_CCNB;
            break;
        case CCN_TLV:
            suite_intdef = CCNL_SUITE_CCNTLV;
            break;
        case NDN_TLV:
            suite_intdef = CCNL_SUITE_NDNTLV;
            break;
        default:
            DEBUGMSG(WARNING, "Suite %s not supported at the moment through the UAPI\n", ccnl_enc2str(fib_rule->suite));
            continue;
        }


        if ( !(fwd->prefix = ccnl_URItoPrefix((char *) fib_rule->prefix, suite_intdef, NULL, NULL)))
        {
            ccnl_free(fwd);
            DEBUGMSG(WARNING, "Failed to create name prefix for %s\n", fib_rule->prefix);
            continue;
        };

        fwd->face = ccnl_get_face_or_create(r, fib_rule->dev_id, &(fib_rule->next_hop.sa), sizeof (fib_rule->next_hop.linklayer));
        if (! (fwd->face))
        {
            DEBUGMSG(WARNING, "could not allocate a new face or find an existing one for dev-id %d\n", fib_rule->dev_id);
            free_prefix(fwd->prefix);
            ccnl_free(fwd);
            continue;
        }
        fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;

        fwd->suite = suite_intdef;

    #ifdef USE_FRAG
        if ( !fwd->face->frag ) // if newly created face, no fragment policy is defined yet
            fwd->face->frag = ccnl_frag_new(CCNL_FRAG_CCNx2013, 1200);
    #endif

        // new fwd rule put at the top of the list
        fwd->next = r->fib;
        r->fib = fwd;

        cnt++;
    }
    return cnt;
};




/*
 * helper: create a ccnl_prefix_s from a name in an info_data_s object (I/C description)
 */
struct ccnl_prefix_s *
mk_prefix(struct info_data_s *d_obj)
{
    char *                  name;
    struct ccnl_prefix_s *  pfx;
    int                     suite;

    suite = (int) readSuite(d_obj);

    // FIXME: This is not needed if one harmonises suite enums in UAPI and ccnl-defs.h
    switch (suite) {
    case CCN_XMLB:
        suite = CCNL_SUITE_CCNB;
        break;
    case CCN_TLV:
        suite = CCNL_SUITE_CCNTLV;
        break;
    case NDN_TLV:
        suite = CCNL_SUITE_NDNTLV;
        break;
    default:
        DEBUGMSG(ERROR, "Suite %s not supported at the moment through the UAPI\n", ccnl_enc2str(suite));
        return NULL;
    }

    //prepare name
    if (d_obj->name)    // name provided as one string (possibly encoding non printable %-escaped chars), convert to ccnl_prefix_s
    {
        name = (char *) ccnl_malloc (sizeof(char) * (strlen(d_obj->name) +1));
        strncpy (name, d_obj->name, (strlen(d_obj->name) +1));      // because ccnl_URItoPrefix() modifies the first arg

        if (d_obj->chunk_seqn >=0)
            pfx = ccnl_URItoPrefix(name, suite, NULL, (unsigned int *) &d_obj->chunk_seqn);
        else
            pfx = ccnl_URItoPrefix(name, suite, NULL, NULL);

        ccnl_free(name);
    }
    else if(d_obj->name_components) // name provided as a set of name components adhocly organised, manually
    {
        // FIXME:
        // the rationale of name components is that some of them may contain (NFN) expressions
        // and so the assumption is that ccn-lite has a function that takes such a vector and
        // converts it to the internal ccnl_prefix_s; but currently there isn't any available.
        // Providing a Components2Prefix() here is messy cause the ccnl_prefix_s has more
        // sophisticated suite-based semnatics unlike the high level component vector of strings
        // in info_data_s, which the user cares about
        //
        // Quick patch: convert to a single string and then process as member "name"

        int namelen = 0;
        for (int i = 0; d_obj->name_components[i]!= NULL; i++)
            namelen += strlen(d_obj->name_components[i]);
        char * name = (char *) ccnl_malloc (1+ sizeof(char) * namelen);
        
        memset (name, 0, namelen+1);
        for (int i = 0; d_obj->name_components[i]!= NULL; i++)
            strcat(name, d_obj->name_components[i]);
        
        if (d_obj->chunk_seqn >=0)
            pfx = ccnl_URItoPrefix (name, suite, NULL, (unsigned int *) &d_obj->chunk_seqn);
        else
            pfx = ccnl_URItoPrefix (name, suite, NULL, NULL);
        
        ccnl_free(name);
    }
    else
        return NULL;    // nothing happened

    return pfx;
}




/*
 * create a ccnb interest packet
 */
int
mk_ccnb_interest(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_interest_ccnXmlb_s *    i_obj = (struct info_interest_ccnXmlb_s *) d_obj;
    struct ccnl_prefix_s *              pfx;
    int                                 digest_len = 0, ppkd_len = 0;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no I pkt created

    if (i_obj->content_digest) digest_len = unescape_component(i_obj->content_digest);
    if (i_obj->ppkd) ppkd_len = unescape_component(i_obj->ppkd);

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);

    d_obj->packet_len = ccnl_ccnb_mkInterest(
                                pfx,
                                i_obj->min_suffix,
                                i_obj->max_suffix,
                                (unsigned char*) i_obj->content_digest, digest_len,
                                (unsigned char*) i_obj->ppkd, ppkd_len,
                                i_obj->scope,
                                &i_obj->nonce,
                                d_obj->packet_bytes);

    free_prefix(pfx); // CHECK: make sure this is not leaving dangling pointers in the relay state?

    return 1;   // one I pkt created
};




/*
 * create a ccntlv interest packet
 */
int
mk_ccntlv_interest(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_interest_ccnTlv_s *    i_obj = (struct info_interest_ccnTlv_s *) d_obj;
    struct ccnl_prefix_s *              pfx;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no I pkt created

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);

    d_obj->packet_len = ccntlv_mkInterest(
                                    pfx,
                                    (int *) &i_obj->nonce,
                                    d_obj->packet_bytes,
                                    CCNL_MAX_PACKET_SIZE);
    
    free_prefix(pfx);   // CHECK: make sure this is not leaving dangling pointers in the relay state?
    return 1;   // one I pkt created
}




/*
 * create a ndntlv interest packet
 */
int
mk_ndntlv_interest(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_interest_ndnTlv_s *    i_obj = (struct info_interest_ndnTlv_s *) d_obj;
    struct ccnl_prefix_s *              pfx;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no I pkt created

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);

    d_obj->packet_len = ndntlv_mkInterest(pfx,
                            (int *) &i_obj->nonce,
                            d_obj->packet_bytes,
                            CCNL_MAX_PACKET_SIZE);

    free_prefix(pfx);   // CHECK: make sure this is not leaving dangling pointers in the relay state?
    return 1;   // one I pkt created
}



/*
 * create a ccnb content packet
 */
int
mk_ccnb_content(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_content_ccnXmlb_s *    c_obj = (struct info_content_ccnXmlb_s *) d_obj;
    struct ccnl_prefix_s *              pfx;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no C pkt created

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);

    // CHECK: hmm is this not too simplistic ? .. not having to provide publisher key ?
    d_obj->packet_len = ccnl_ccnb_fillContent(pfx, c_obj->content_bytes, c_obj->content_len, NULL, d_obj->packet_bytes);
    
    // did it fit in one pkt ?
    if (d_obj->packet_len > CCNL_MAX_PACKET_SIZE ) {
        DEBUGMSG(ERROR, "Content data too big; cannot fit in one CCNx (XMLB fmt) C packet\n");
        return 0;   // no C pkt created (FIXME: well ccnl_ccnb_fillContent() wrote off the buffer boundary, so we probably have a buffer overflow error)
    }

    return 1;   // 1 C pkt created
}


/*
 * create a ccnb content packet
 */
int
mk_ccntlv_content(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_content_ccnTlv_s *     c_obj = (struct info_content_ccnTlv_s *) d_obj;
    struct ccnl_prefix_s *              pfx;
    int                                 data_offset;
    int                                 r=0;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no C pkt created

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
    data_offset = CCNL_MAX_PACKET_SIZE;

    /*  TODO: *** This seems to be an incomplete feature at the moment (relies on #define USE_HMAC256 in ccnl-uapi.h) ***
    if (c_obj->keyid_bytes) {
        unsigned char keyval[64];
        unsigned char keyid[32];

        // use the key to generate signature info
        ccnl_hmac256_keyval(c_obj->keyid_bytes, c_obj->keyid_len, keyval);
        ccnl_hmac256_keyid(c_obj->keyid_bytes, c_obj->keyid_len, keyid);

        // create C pkt with signed content
        r = ccnl_ccntlv_prependSignedContentWithHdr(
                                       pfx,
                                       c_obj->content_bytes,
                                       c_obj->content_len,
                                       NULL,
                                       NULL,
                                       keyval,
                                       keyid,
                                       &data_offset,
                                       d_obj->packet_bytes);
    } else
    */
        // create C pkt with unsigned content
        r = ccnl_ccntlv_prependContentWithHdr(
                                       pfx,
                                       c_obj->content_bytes,
                                       c_obj->content_len,
                                       NULL,
                                       NULL,
                                       &data_offset,
                                       d_obj->packet_bytes);

    if (r == -1) {
        // content did not fit in one packet
        DEBUGMSG(ERROR, "Content data too big; cannot fit in one CCNx (TLV fmt) C packet\n");
        return 0;   // no C pkt created
    } else
        d_obj->packet_len = r;

    // the packet buffer was filled from the end towards the start
    // so we need move the data to the start of the buffer
    memcpy(d_obj->packet_bytes, d_obj->packet_bytes + data_offset, d_obj->packet_len);

    return 1;   // one C pkt succesfully created
}




/*
 * create a ccnb content packet
 */
int
mk_ndntlv_content(struct info_data_s *d_obj)
{
    assert (d_obj);

    struct info_content_ndnTlv_s *     c_obj = (struct info_content_ndnTlv_s *) d_obj;
    struct ccnl_prefix_s *              pfx;
    int                                 data_offset;
    int                                 r=0;

    if (! (pfx = mk_prefix(d_obj)))
        return 0;       // no C pkt created

    if (d_obj->packet_bytes && d_obj->packet_len && d_obj->packet_len < CCNL_MAX_PACKET_SIZE)
        ccnl_free (d_obj->packet_bytes);

    if (!d_obj->packet_bytes) d_obj->packet_bytes = (unsigned char *) ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
    data_offset = CCNL_MAX_PACKET_SIZE;

    /* TODO: *** This seems to be an incomplete feature at the moment (relies on #define USE_HMAC256 in ccnl-uapi.h) ***
    if (c_obj->sigkey_bytes) {
        unsigned char keyval[64];
        unsigned char keyid[32];

        // use key to generate signature info
        ccnl_hmac256_keyval(c_obj->sigkey_bytes, c_obj->sigkey_len, keyval);
        ccnl_hmac256_keyid(c_obj->sigkey_bytes, c_obj->sigkey_len, keyid);

        // create signed C pkt
        r = ccnl_ndntlv_prependSignedContent(
                                pfx,
                                c_obj->content_bytes, c_obj->content_len,
                                NULL, NULL,
                                keyval, keyid,
                                &data_offset,
                                d_obj->packet_bytes);
    } else
    */
        // create unsigned C pkt
        r = ccnl_ndntlv_prependContent(
                                pfx,
                                c_obj->content_bytes, c_obj->content_len,
                                NULL, NULL,
                                &data_offset,
                                d_obj->packet_bytes);


    if (r == -1) {
        // content did not fit in one packet
        DEBUGMSG(ERROR, "Content data too big; cannot fit in one NDNx C packet\n");
        return 0;   // no C pkt created
    } else
        d_obj->packet_len = r;

    // the packet buffer was filled from the end towards the start
    // so we need move the data to the start of the buffer
    memcpy(d_obj->packet_bytes, d_obj->packet_bytes + data_offset, d_obj->packet_len);

    return 1;   // 1 C pkt created
}


