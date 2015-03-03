/*
 * ccnl-uapi.c
 *
 *  Created on: Nov 4, 2014
 *      Author: manolis
 */

#include <assert.h>
#include "ccnl-uapi.h"



/*****************************************************************************
 * Forward decls
 *****************************************************************************/

/*
 * per info struct ctors and dtors
 *
 * they are supposed to take care of creating and
 * initialising the memory layout of the different
 * types of info objects
 *
 * (TBC in time, if they are needed or left NULL)
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
void * (* info_face_conf_ctor)(void *o) = 0;


//dtors
void (* info_interest_ndnTlv_dtor)(void *o) = 0;
void (* info_content_ndnTlv_dtor)(void *o) = 0;
void (* info_interest_ccnTlv_dtor)(void *o) = 0;
void (* info_content_ccnTlv_dtor)(void *o) = 0;
void (* info_interest_ccnXmlb_dtor)(void *o) = 0;
void (* info_content_ccnXmlb_dtor)(void *o) = 0;
void (* info_cs_data_dtor)(void *o) = 0;
void (* info_fib_rule_dtor)(void *o) = 0;
void (* info_face_conf_dtor)(void *o) = 0;


#ifndef PLATFORM_SRVCS
//FIXME: make this complain when not set by a platform
struct platform_vt_s platform_srvcs = {};
#endif




/*
 * TODO: Local function decls used to populate the vtables. Typically these will be
 * provided by include directives of the headers where these funcs are declared
 */
int faces_add(struct info_mgmt_s *mgmt);
int content_add(struct info_mgmt_s *mgmt);
int fwd_rule_add(struct info_mgmt_s *mgmt);
int mk_ccnb_interest(struct info_data_s *o);







/*****************************************************************************
 * vtables that need to be filled for the API to work
 *****************************************************************************/

/*
 * vtables to be populated for the different suite structs
 */

//ndntlv
const struct suite_vt_s ndnTlv_vt = {
        mk_ndntlv_interest,0,0,0
    // TODO: insert fptrs: mkInterest/mkContent/rdInterest/rdContent
};

//ccntlv
const struct suite_vt_s ccnTlv_vt = {
        mk_ccntlv_interest,0,0,0
        // TODO: insert fptrs: mkInterest/mkContent/rdInterest/rdContent
};

//ccnb
const struct suite_vt_s ccnXmlb_vt = {
        mk_ccnb_interest,
        0,0,0
        // TODO: insert fptrs: mkInterest/mkContent/rdInterest/rdContent
};




/*
 * vtables to be populated for the different management structs
 */

// CS data
const struct info_mgmt_vt_s info_cs_data_vt = {
        sizeof(struct info_cs_data_s),
        MGMT_CS,
        info_cs_data_ctor,
        info_cs_data_dtor,

        // TODO: assign fptrs for add/rem/set/get
        content_add,
        0,0,0
};

// FIB rules
const struct info_mgmt_vt_s info_fib_rule_vt = {
        sizeof (struct info_fib_rule_s),
        MGMT_FIB,
        info_fib_rule_ctor,
        info_fib_rule_dtor,

        // TODO: assign fptrs for add/rem/set/get
        fwd_rule_add,
        0,0,0
};

// FACE conf
const struct info_mgmt_vt_s info_face_conf_vt = {
        sizeof (struct info_face_conf_s),
        MGMT_FACE,
        info_face_conf_ctor,
        info_face_conf_dtor,

        // TODO: assign fptrs for add/rem/set/get
        faces_add,
        0,0,0
};




/*****************************************************************************
 * vtables that are pre-filled
 *****************************************************************************/

/*
 * info object type descriptions (fixed vtables)
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



/*****************************************************************************
 * type definitions for allocations of management and data objects
 *****************************************************************************/

/*
 * type definitions for use as in mallocSuiteInfo(), par example
 *
 * mallocSuiteInfo(info_interest_ndnTlv_s)
 */
const void * info_interest_ndnTlv_s = &info_interest_ndnTlv_vt;
const void * info_content_ndnTlv_s = &info_content_ndnTlv_vt;
const void * info_interest_ccnTlv_s = &info_interest_ccnTlv_vt;
const void * info_content_ccnTlv_s = &info_content_ccnTlv_vt;
const void * info_interest_ccnXmlb_s = &info_interest_ccnXmlb_vt;
const void * info_content_ccnXmlb_s = &info_content_ccnXmlb_vt;

/*
 * type definitions for use in mallocMgmtInfo()
 */
const void * info_cs_data_s = &info_cs_data_vt;
const void * info_fib_rule_s = &info_fib_rule_vt;
const void * info_face_conf_s = &info_face_conf_vt;



/*****************************************************************************
 * exported api generic function definitions
 *****************************************************************************/

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
processObject (void *relay, struct info_data_s *o, sockunion * from)
{
    int addr_len = 0;

    if (!o)         // nothing to be send
        return 0;

    // TODO: Ask Chris if this is suite dependent or generic for all suites
    // if suite dependent the correct sending function should be selected
    // based on suite info attached to the info_data_s

    if (! from)     // locally generated
    {
        ccnl_core_RX((struct ccnl_relay_s *) relay, -1, (unsigned char*) o->pkt_buf, o->pkt_len, 0, 0);
    }
    else            // received from some socket addr
    {
        switch (from->sa.sa_family)
        {
#ifdef USE_ETHERNET
        case AF_PACKET:
            addr_len = sizeof(from->eth);
            break;
#endif
        case AF_INET:
            addr_len = sizeof(from->ip4);
            break;
#ifdef USE_UNIXSOCKET
        case AF_UNIX:
            addr_len = sizeof(from->ux);
            break;
#endif
        default:
            return -1;  // error: unsupported socket family
        }

        ccnl_core_RX((struct ccnl_relay_s *) relay, 0, (unsigned char*) o->pkt_buf, o->pkt_len, &(from->sa), addr_len);
        //ccnl_core_RX((struct ccnl_relay_s *) relay, -1, (unsigned char*) o->pkt_buf, o->pkt_len, &(from->sa), addr_len);
    }

    return 1;   // success: one packet processed
};


// parse an I suite-formatted obj and gen respective info struct
struct info_data_s *
parseInterest(char *pkt_buf, int pkt_len)
{
    return 0;
};


// parse a C suite-formatted obj and gen respective info struct
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

    r->max_cache_entries = cs_inodes;

    // TODO: set somewhere the cs storage size
    // TODO: save somewhere a str name for the relay

    if (enable_stats) {
        r->stats = (struct ccnl_stats_s *) ccnl_calloc (1, sizeof(struct ccnl_stats_s));
        if (platform_srvcs.log_start)
            platform_srvcs.log_start (r, (char *) node_name);
    }
    else
    {
        r->stats = 0;
        DEBUGMSG(99, "ccnl_create_relay on node %s -- stats logging not activated \n", node_name);
    }

    /* no scheduling at the face level (only at the interface level for tx pacing)
     */
    r->defaultFaceScheduler = NULL;

    r->aux = aux;

    return (void *) r;
};




// dispose relay state and free
void
destroyRelay (void *relay)
{
    struct ccnl_relay_s *r = (struct ccnl_relay_s *) relay;

    if (r->stats) {
        if (platform_srvcs.log_stop)
            platform_srvcs.log_stop (r);

        ccnl_free(r->stats);
        r->stats = NULL;
    }

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

    //DEBUGMSG(99, "aging epoch completed for relay %c\n", ((struct ccnl_relay_s *)relay)->id);

    return;
};



/*
 * pause relay op but do not dispose state
 */
void
stopRelay (void *relay) {return;};


/*
 * create a mgmt info struct mem layout and init
 */
struct info_mgmt_s *
mallocMgmtInfo (const void *mgmt_descr)
{
    struct info_mgmt_vt_s const * mgmt_vt= (struct info_mgmt_vt_s const *) mgmt_descr;

    struct info_mgmt_s * mgmt = (struct info_mgmt_s *) ccnl_calloc(1, mgmt_vt->size);

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
 * The following are fixed callbacks that the ccn-lite kernel calls to deliver
 * information to the platform through the generic API
 *
 * TODO: Probably these need to become more chatty in the information they
 * provide
 ****************************************************************************/

/* helper: free a struct ccnl_prefix_s */
static inline
void
_ccnl_prefix_struct_free(struct ccnl_prefix_s *p)
{
    ccnl_free(p->comp);
    ccnl_free(p->complen);
    ccnl_free(p->path);                // TODO: OLD
    //ccnl_free(pr->bytes);             // TODO: NEW
    ccnl_free(p);

    return;
};


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
            struct ccnl_buf_s *buf)
{
    struct envelope_s *     env = (struct envelope_s *) ccnl_malloc(sizeof(struct envelope_s));
    struct info_data_s *     io;

    struct ccnl_buf_s       *tmp_buf = 0;
    struct ccnl_prefix_s    *p = 0;
    int                     scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    unsigned char           *content = 0;

    assert(relay && ifc && buf && dst);

    // the following is a hack practically emulating what the
    // ccnl_core_RX_i_or_c() does in ccnl-core.c, in order to
    // access the object type and prefix information
    //
    // TODO: normally this information should be provided as part
    // of the signature of this function
    //
    unsigned char * tmp_data = buf->data+2;
    int tmp_datalen = buf->datalen - 2;
    tmp_buf = ccnl_extract_prefix_nonce_ppkd(
                &tmp_data,
                &tmp_datalen,
                &scope,
                &aok,
                &minsfx,
                &maxsfx,
                &p,
                0,
                0,
                &content,
                &contlen);

    if (tmp_buf->data[0] == 0x01 && tmp_buf->data[1] == 0xd2)
    // interest pkt
        io = mallocSuiteInfo(info_interest_ccnXmlb_s);
    else
    // content pkt
        io = mallocSuiteInfo(info_content_ccnXmlb_s);

    //io->name = strdup(ccnl_prefix_to_path(p));
    io->name = ccnl_prefix_to_path(p);  // TODO: should be safe since io is const at the deliverObject call
    io->pkt_buf = tmp_buf->data;
    io->pkt_len = tmp_buf->datalen;

    // envelope for data to be delivered to a link
    env->to = ENV_LINK;
    memcpy (&(env->link.sa_local), &(ifc->addr), sizeof(sockunion));    // local out addr
    memcpy (&(env->link.sa_remote), dst, sizeof(sockunion));            // next hop addr

    // hand in to platform
    platform_srvcs.deliver_object(relay, io, env);

    _ccnl_prefix_struct_free(p);
    ccnl_free (tmp_buf);
    freeSuiteInfo(io);
    ccnl_free (env);

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
    struct info_data_s *     io;
    char                    tmp[10];
    struct ccnl_prefix_s *  p;

    assert (relay && c);

    // this is a content pkt so the name should contain a chunk seq num;
    // separate it in tmp2
    p = c->name;
    memcpy(tmp, p->comp[p->compcnt-1], p->complen[p->compcnt-1]);
    tmp[p->complen[p->compcnt-1]] = '\0';

    // prepare info object
    io = mallocSuiteInfo(info_content_ccnXmlb_s);
    io->pkt_buf = c->pkt->data;
    io->pkt_len = c->pkt->datalen;
    io->name = ccnl_prefix_to_path(c->name); // TODO: should be safe since io is const at the deliverObject call
    io->chunk_seqn = atoi(tmp+1);            // FIXME: here we asume that the chunk seq num is prefixed by a letter
                                             // but this is not a safe assumption, since this is not in standard spec

    // prepare envelope
    env->to = ENV_APP;

    // hand in to platform
    platform_srvcs.deliver_object(relay, io, env);

    freeSuiteInfo(io);
    ccnl_free (env);

    return;
};




/*****************************************************************************
 * non-generic function definitions that are hooked to the API vtables
 *
 * These should be confined to ones that are not to be found in other
 * per-suite source files. They are plugged to the vtables on the ccnlite
 * side.
 *
 * TODO: Clean up and move code where it belongs
 * TODO: Chris must replace these with the correct ones
 *****************************************************************************/

#define _TMP_CONTENT_OBJ_MAXSIZE    8192
#define _TMP_INTEREST_OBJ_MAXSIZE   4096

/*
 * helper: break a string name to name components in ccnl_prefix_s struct
 * TODO: Chris does one already exist somewhere ?
 */
static
struct ccnl_prefix_s*
ccnl_path_to_prefix(const char *path)
{
    char *cp;
    struct ccnl_prefix_s *pr = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(*pr));

    if (!pr)
        return NULL;

    // +1 is for letting a last position to be set to null (see before the return stmt)
    pr->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP+1 * sizeof(unsigned char**));
    pr->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));

    pr->path = (unsigned char*) ccnl_malloc(strlen(path)+1);        // OLD
    //pr->bytes = (unsigned char*) ccnl_malloc(strlen(path)+1);     // NEW

    if (!pr->comp || !pr->complen || !pr->path) {       // OLD
    //if (!pr->comp || !pr->complen || !pr->bytes) {    // NEW

        ccnl_free(pr->comp);
        ccnl_free(pr->complen);
        ccnl_free(pr->path);                // OLD
        //ccnl_free(pr->bytes);             // NEW
        ccnl_free(pr);
        return NULL;
    }

    strcpy((char*) pr->path, path);     // OLD
    //strcpy((char*) pr->bytes, path);  // NEW

    cp = (char*) pr->path;      // OLD
    //cp = (char*) pr->bytes;   // NEW

    for (path = strtok(cp, "/");
         path && pr->compcnt < CCNL_MAX_NAME_COMP;
         path = strtok(NULL, "/"))
    {
        pr->comp[pr->compcnt] = (unsigned char*) path;
        pr->complen[pr->compcnt] = strlen(path);
        pr->compcnt++;
    }

    // this compensates for the fact that some functions of ccnl-core.c use the
    // compcnt value for finding the end of components, while others rely on a
    // null last component.
    pr->comp[pr->compcnt] = 0;

    return pr;
}






/*
 * add and configure a number of faces
 */
int
faces_add(struct info_mgmt_s *mgmt)
{
    struct info_face_conf_s     *faces = (struct info_face_conf_s *) mgmt;
    struct ccnl_relay_s         *r = (struct ccnl_relay_s *) mgmt->relay;
    struct ccnl_if_s            *i;
    int                         cnt=0;

    if (!r)
        return 0;  // no access to the relay state, nothing more to do

    int t;

    for (int k=0 ; faces ; k++ , faces = (struct info_face_conf_s *) faces->base.next)
    {
        i = & (r->ifs[k]);

        t = faces->addr.sa.sa_family;

        switch (t)
        {
        case AF_PACKET:
            i->addr.eth.sll_family = AF_PACKET;
            memcpy(i->addr.eth.sll_addr, faces->addr.eth.sll_addr, ETH_ALEN);
            i->mtu = 1400;
            i->reflect = 1;
#ifdef  PROPAGATE_INTERESTS_SEEN_BEFORE
            i->fwdalli = 1;
#else
            i->fwdalli = 0;
#endif  // PROPAGATE_INTERESTS_SEEN_BEFORE

#ifdef  USE_SCHEDULER
            i->sched = ccnl_sched_pktrate_new (ccnl_interface_CTS, r, faces->tx_pace);
#endif  // USE_SCHEDULER

            r->ifcount++;
            cnt++;

            break;

        // TODO: Add the other cases AF_INET AF_INET6 ...
        case AF_INET:
        case AF_INET6:
        default:
            break;
        }
    };

    return cnt;     // success: number of faces created
}


/*
 * add a set of content objects
 */
int
content_add (struct info_mgmt_s *mgmt)
{
    struct info_cs_data_s       *cs_data = (struct info_cs_data_s *) mgmt;
    struct ccnl_relay_s         *r = (struct ccnl_relay_s *) mgmt->relay;
    int                         cnt=0;
    struct ccnl_buf_s           *bp;
    struct ccnl_prefix_s        *pp;
    unsigned char               tmp2[_TMP_CONTENT_OBJ_MAXSIZE];
    int                         len2 = 0;
    struct ccnl_content_s       *c = 0;


    if (!r)
        return 0;      // no access to the relay state, nothing more to do

    for (int k=0 ; cs_data ; k++ , cs_data = (struct info_cs_data_s *) cs_data->base.next)
    {
        if ( ! (pp = ccnl_path_to_prefix((const char*) (cs_data->name))) )
            continue;

        // create a content object
        //
        assert (cs_data->data_size < _TMP_CONTENT_OBJ_MAXSIZE);
        memset (tmp2, 0, _TMP_CONTENT_OBJ_MAXSIZE);

        // FIXME: this is the old API func needs fixing
        len2 = mkContent((char **) pp->comp, (char *) cs_data->data_buf, cs_data->data_size, tmp2);

        // store data to internal buffer structure
        //
        if ( ! (bp = ccnl_buf_new(tmp2, len2)))
        {
            _ccnl_prefix_struct_free(pp);
            continue;
        }

        // prep cs entry and add to cs_db
        //
        // FIXME: Figure out what are the last args needed for, since the info is
        // provided in the bp ptr.
        c = ccnl_content_new((struct ccnl_relay_s *)cs_data->base.relay, &bp, &pp, NULL, 0, 0);

        if (c)
        {
#ifdef CONTENT_NEVER_EXPIRES
            c->flags |= CCNL_CONTENT_FLAGS_STATIC;
#endif
            ccnl_content_add2cache((struct ccnl_relay_s *)cs_data->base.relay, c);  // we should expect a success code here but this func does not return one.

            cnt++;
        }

        // these are not needed because control of the pointed data is taken over
        // at ccnl_content_new. FIXME: This is a bit of "unexpected" practice/convention.
        //
        //_ccnl_prefix_struct_free(pp);
        //ccnl_free(bp);
    }

    return cnt; //success: return the number of content objects added to the cs
};



/*
 * add a set of forwarding rules based on MAC addrs
 */
int
fwd_rule_add(struct info_mgmt_s *mgmt)
{
    struct info_fib_rule_s *    fib_rule = (struct info_fib_rule_s *) mgmt;
    struct ccnl_relay_s *       r = (struct ccnl_relay_s *) mgmt->relay;
    struct ccnl_forward_s *     fwd;
    int                         cnt=0;

    if (!r)
        return 0;  // no access to the relay state, nothing more to do

    for (int k=0 ; fib_rule ; k++ , fib_rule = (struct info_fib_rule_s *) fib_rule->base.next)
    {
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));

        if ( !(fwd->prefix = ccnl_path_to_prefix(fib_rule->prefix)))
        {
            ccnl_free(fwd);
            //return -1;      // error: failed to extract a set of name components from supplied string
            continue;
        };

        fwd->face = ccnl_get_face_or_create(r, fib_rule->dev_id, &(fib_rule->next_hop.sa), sizeof (fib_rule->next_hop.eth));

    #ifdef USE_FRAG
        if ( !fwd->face->frag ) // if newly created face, no fragment policy is defined yet
            fwd->face->frag = ccnl_frag_new(CCNL_FRAG_CCNx2013, 1200);
    #endif

        if (! (fwd->face))
        {
            _ccnl_prefix_struct_free(fwd->prefix);
            ccnl_free(fwd);
            //return -1;          // error: we could not allocate a new face or find an existing one
            continue;
        }

        fwd->face->flags |= CCNL_FACE_FLAGS_STATIC;
        fwd->next = r->fib;
        r->fib = fwd;

        //
        cnt++;
    }

    //return 1;       // success: 1 FIB entry added
    return cnt;
};



/*
 * create a ccnb interest packet
 */
int
mk_ccnb_interest(struct info_data_s *o)
{
    struct info_interest_ccnXmlb_s *    i_info = (struct info_interest_ccnXmlb_s *) o;
    struct ccnl_prefix_s *              pp = 0;
    unsigned char                       tmp2[_TMP_INTEREST_OBJ_MAXSIZE];
    int                                 nonce = rand();
    int                                 len = 0;

    assert (i_info);

    /*
     * FIXME: NOTE that as things are here, to create an interest we do not look at the
     * chunk_seqn field of the info_interest_ccnXmlb_s struct passed to us to determine
     * the last component as a chunk num. In principle we SHOULD(!) because in this way
     * we can differentiate prefixes from exact names, and because this MAY make easier
     * the juggling with conventions on how chunk naming should be formed at the last
     * component
     */

    // name must be supplied
    if (i_info->base.name)      // name supplied as a string
    {
        pp = ccnl_path_to_prefix(i_info->base.name);
        assert (pp);

        /* create actual ccnb Interest pkt
         */
        memset (tmp2, 0, _TMP_INTEREST_OBJ_MAXSIZE);
        len = mkInterest((char **) pp->comp, (unsigned int *) &nonce, (unsigned char *) tmp2);

        _ccnl_prefix_struct_free(pp);

        if (!len) return -1;                        // error: could not create interest
    }
    else if ( *(i_info->base.name_components))        // name supplied as a set of components
    {
        // check that 1 past the last name component is a null component (sanity)
        for (int i = 0 ; i<=CCNL_MAX_NAME_COMP ; i++ ) {
            if (i_info->base.name_components[i] == 0) break;
            if (i == CCNL_MAX_NAME_COMP) return 0;  // nothing to be done the components set supplied is not correct
        }

        /* create actual ccnb Interest pkt
         */
        memset (tmp2, 0, _TMP_INTEREST_OBJ_MAXSIZE);
        len = mkInterest((char **) i_info->base.name_components, (unsigned int *) &nonce, (unsigned char *) tmp2);

        if (!len) return -1;       // error: could not create interest
    }
    else                // name has not been supplied neither as string nor as list of components
        return 0;       // nothing to be done on our end


    i_info->base.pkt_buf = (unsigned char *) ccnl_malloc (len);
    memcpy (i_info->base.pkt_buf, tmp2, len);
    i_info->base.pkt_len = len;
    i_info->nonce = nonce;

    return 1;       // success: 1 interest pkt created
};


/*
 * create a ccntlv interest packet
 */
int
mk_ccntlv_interest(struct info_data_s *o)
{
    struct ccnl_prefix_s * pref:
    //prepare name
    if (o->name) {
        pref = ccnl_URItoPrefix(o->name, CCNL_SUITE_CCNTLV, NULL, o->chunk_seqn)
    }
    else if(o->name_components){
        
    }
    

    o->pkt_buf = ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
    o->pkt_len = ccntlv_mkInterest(struct ccnl_prefix_s *name, int *dummy,
                                   o->pkt_buf, CCNL_MAX_PACKET_SIZE);

}

/*
 * create a ndntlv interest packet
 */
int
mk_ndntlv_interest(struct info_data_s *o)
{
    
}


