/*
 * ccnl-uapi.h
 *
 *  Created on: Oct 24, 2014
 *      Author: manolis
 *
 *  Updated for ccn-lite > v0.1.0: Jun 4, 2015 (manolis)
 */



/*****************************************************************************
                               OVERVIEW
                               ========

  ccn-lite expects the following service API to be instantiated by the host
  platform

  struct platform_vt_s {
    void    (* deliver_object) (void *relay, struct info_data_s const * io, struct envelope_s * env);    // MUST
    void    (* log_start) (struct ccnl_relay_s *relay, char * node_name);                          // MAY
    void    (* log_stop) (struct ccnl_relay_s *relay);                           // MAY
    void *  (* set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2 );     // MUST
    void *  (* set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2); // MUST
    void    (* rem_timer) (void *hdl);      // MUST
    double  (* get_time_now) ();            // MUST
    char *  (* get_timestamp) ();           // MUST if using debugging
    void    (* get_timeval) (struct timeval *tv);   // MUST
    void    (* log) (char *logmsg);  // MUST
    void    (* print_stats) (struct ccnl_relay_s *relay, int code); // MAY
    void    (* close_socket) (int s);   // MUST only if socket based faces are used
  };

  ... it accesses this API thought the global var

  extern struct platform_vt_s platform_srvcs;

  Essentially the hosting platform code should define and initialise the
  "platform_srvcs" global vtable, with the functions providing the respective
  platform services and callbacks that ccnlite needs. Additionally it should
  set the PLATFORM_SRVCS def.

  E.g. in omnet I have something like the following

  #define PLATFORM_SRVCS

  struct platform_vt_s platform_srvcs = {
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

  ---------------------------------------------------------------------------

  The host platform on the other hand can access use the following ccn-lite
  services through UAPI

  A) generic functions for all suites, both data objects (I/C)

  int                         suiteStr2Id(char *s);                         // get a suite ID from a string
  static inline const char *  suiteId2Str(int id);                          // convert a suite ID to a string
  static inline icn_suite_t   readSuite (const struct info_data_s *o);      // report the suite ID of an I or C pkt description
  static inline const char *  printSuite (const struct info_data_s *o);     // print the suite of an I or C pkt description
  static inline info_data_t   readTypeIorC (const struct info_data_s *o);   // report a ID of an I or C pkt description
  static inline const char *  printTypeIorC (const struct info_data_s *o);  // print if an I or C pkt description
  static inline int           createPacketI (struct info_data_s *o);        // create a suite-formated C pkt from a description object
  static inline int           createPacketC (struct info_data_s *o);        // create a suite-formated C pkt from a description object
  struct info_data_s *        parsePacketI (char *pkt_buf, int pkt_len);    // parse an I suite-formatted packet and gen respective description object
  struct info_data_s *        parsePacketC (char *pkt_buf, int pkt_len);    // parse a C suite-formatted packet and gen respective description object
  struct info_data_s *        mallocSuiteInfo (const void *obj_descr);      // allocate the right data object type description
  void                        freeSuiteInfo (struct info_data_s *o);        // free mem of an data object type desctription
  int                         processSuiteData (void *relay, struct info_data_s *o, sockunion * from_addr, int rx_iface);  // pass an I/C suite-formatted pkt to ccn-lite

  B) generic functions for relay mgmt subsystems (CS/FIB/FACES)

  void *                      createRelay (const char * node_name, int cs_inodes, int cs_bytesize, char enable_stats, void *aux);  // malloc/init relay state
  void                        destroyRelay (void *relay);                   // dispose relay state and free
  void                        startRelay (void *relay);                     // start relay op with remembered state
  void                        stopRelay (void *relay);                      // pause relay op but do not dispose state
  struct info_mgmt_s *        mallocMgmtInfo (const void *mgmt_descr);      // create the right mgmt object and presets some attribs
  void                        freeMgmtInfo (struct info_mgmt_s *m);         // delete mem of a mgmt object
  static inline int           addMgmt (struct info_mgmt_s *m);              // add cs data/fib rule/iface config
  static inline int           remMgmt (struct info_mgmt_s *m);              // delete cs data/fib rule/iface config
  static inline int           getMgmt (struct info_mgmt_s *m);              // read cs data/fib rule/iface config
  static inline int           setMgmt (struct info_mgmt_s *m);              // update cs data/fib rule/iface config

  ---------------------------------------------------------------------------

  The whole information exchange between the host platform and ccn-lite is
  implemented by two types of data objects and their sub-classed hierarchies

  info_data_s : base type obect for I or C of any suite
    |
    |    (derived objects)
    +- info_interest_ndnTlv_s   // ndn tlv I
    +- info_content_ndnTlv_s    // ndn tlv C
    +- info_interest_ccnTlv_s   // ccn tlv I
    +- info_content_ccnTlv_s    // ccn tlv C
    +- info_interest_ccnXmlb_s  // ccn xmlb I
    +- info_content_ccnXmlb_s   // ccn xmlb C

  each of the derived data objects are specific to each suite and packet type (attibs,
  pkt creation and parsing, etc). E.g.

       createPacketC(obj)       // from the suite specific attribs of obj
                                // generates a respective C packet.
                                // The C pkt buffer is also attached to the
                                // description obj

       obj = parsePacketC(buf, len) // parses a pkt buffer and generates a list of
                                    // suite specific attribs; attach the buffer
                                    // to an respective type obj

  Bottom line is that irrespective of suite the same functions are used always
  for all of them. New suites can be added by simply deriving more sub-types from
  info_data_s. Similarly the list of attibs can be extended without changing the
  interface, only the code in the vtables and the members of the subtype data objects.

  Tthe derived per suite, per pkt, data type objects can be created/destroyed with
  mallocSuiteInfo()/freeSuiteInfo() and manipulated polymoprhically through their
  common base type object Eg.

       obj = mallocSuiteInfo(info_content_ccnTlv_s)     // allocate a ccn tlv I obj
       obj2 = mallocSuiteInfo(info_content_ndnTlv_s)     // allocate a ccn tlv I obj
       freeSuiteInfo (obj)                              // free a ccn tlv I obj
       processSuiteData(obj)                            // process a ccn tlv I obj
       processSuiteData(obj2)                           // process a ndn tlv I obj

  Similar philosophy is used for introspection and configuration of the relay state
  by addressing generically its various subsystems with mgmt type objects.


  info_mgmt_s : base class for exchanging info with the FIB/CS/IFACES subsys
    |
    |    (derived objects)
    +- info_cs_data_s       // inspect/config the CS
    +- info_fib_rule_s      // inspect/config the FIB
    +- info_iface_conf_s    // inspect/config the interfaces available for face mapping

 *****************************************************************************/

#ifndef CCNL_UAPI_H_
#define CCNL_UAPI_H_

#include "ccnl-defs.h"
#include "ccnl-core.h"



/*****************************************************************************
 * various enum type defs
 *****************************************************************************/

// FIXME: these should be harmonised wiith the enums at the ccnl-defs.h
// suite types and description strings
typedef enum { CCNx_0=1, CCN_XMLB=1,
               CCNx_1=2, CCN_TLV=2,
               NDNx_0=3, NDN_TLV=3,
               END_SUITE_T
} icn_suite_t;
//
const char  *icn_suite_str[] = {"N/A", "CCNx_0", "CCNx_1", "NDNx_0", "N/A"};
const char  *icn_suite_str_by_fmt[] = {"N/A", "ccnb", "ccntlv","ndntlv", "N/A"};
const char  *icn_suite_str_by_year[] = {"N/A", "CCNx2012", "CCNx2014", "NDNx2014", "N/A"};

// data object types (I/C) by suite
typedef enum { I_CCNx_0=1, C_CCNx_0,
               I_CCNx_1, C_CCNx_1,
               I_NDNx_0, D_NDNx_0,
               END_DATA_T
} info_data_t;
//
const char *pkt_type_str_by_suite[]= {"N/A", "I_CCNx_0","C_CCNx_0","I_CCNx_1","C_CCNx_1","I_NDNx_0","D_NDNx_0", "N/A"};

// mgmt subsystem type
typedef enum{
    MGMT_CS,
    MGMT_FIB,
    MGMT_IFACE,
    END_MGMT_T
} mgmt_subsys_t;

// mgmt command type
typedef enum{
    MGMT_C_ADD,
    MGMT_C_REM,
    MGMT_C_GET,
    MGMT_C_SET,
    END_MGMT_CMD_T
} mgmt_cmd_t;

// TODO: not implemented yet
// identifiers for logging different types of statistics (used by ccn-lite, not the UAPI)
enum {
    STAT_RCV_I,
    STAT_RCV_C,
    STAT_SND_I,
    STAT_SND_C,
    STAT_QLEN,
    STAT_EOP1,
    END_LOG_T
};




/*****************************************************************************
 * ccn-lite passes i/c objects to the platform using the deliverObject() 
 * callback of the plarform_srvcs struct (vt)
 * Alongside it passes an "envelope info" to let the platform know
 * how to handle them. Different envelope types are identified on the
 * platform side using the envelope_t enum
 *****************************************************************************/

// What should the platform do with data received by the ccn-lite kernel
// using the deliverObject() callback, can be inferred from these types
typedef enum{
    ENV_LINK,   // put on the wire
    ENV_APP,    // pass to app
    ENV_STATS,   // use for stats (copies of objects)
    // For NFN par example a different envelop type would be created
    END_ENVELOPE_T
} envelope_t;

//.. and here is a polymorphic envelope struct to hold the envelope info

struct envelope_s
{
    envelope_t  to;
    union {
        // case to == ENV_LINK
        struct {
            sockunion sa_local;
            sockunion sa_remote;
        } link;

        // case to == ENV_APP
        struct { /*TODO*/ } app;

        // case to == ENV_STATS
        struct { /*TODO*/ } stats;
    };
};







/*****************************************************************************
 * object-based type hierarchy for I and C data objects per suite
 *
 * We refer to these structs as data objects, as they convey data path info, 
 * ie payload data and header attribs for wire packets. As such they are all 
 * handled by the API generically through their base type "info_data_s"
 *****************************************************************************/

// base type object
struct info_data_s {
    struct info_data_vt_s const *   vtable;
    const char *                    name;               // User can provide the whole name (prefix) as a string
                                                        //   and we take care of componentisation, .. or ...
    const char *                    name_components[];  // The name components as a set of '\0'-terminated strings
                                                        //   (useful for nfn when the component delimiter is not at
                                                        //   every "/") -- last component ptr MUST be NULL
    int                             chunk_seqn;         // Optionally provided chunk num
                                                        //   Since chunk naming is somewhat arbitrary (e.g. ../cN, 
                                                        //   ../N, ../c_N, etc), and left to a sender/receiver contract,
                                                        //   to reduce ambiguity here we assume that the chunk seq num
                                                        //   SHOULD be present whenever the last component provided in   
                                                      	//   "name" or "name_components" members is NOT identifying a
                                                        //   chunk. The implication of this is that if a chunk_seqn is
                                                        //   not provided (-1 value) the "name" or "name_components" may
                                                        //   still encode a chunk no (appl specific convention), but
                                                        //   ccn-lite will be oblivious to this (therefore not being able
                                                        //   to handle top-level segmentation internally, only segmentation
                                                        //   of segments at a 2nd level).
    unsigned char *                 packet_bytes;       // Buffer holding the packet
    unsigned int                    packet_len;         // Packet buffer len
};

// interest type object for ndntlv
struct info_interest_ndnTlv_s {
    struct info_data_s  base;
    
    // ndntlv specific obj attribs
    unsigned int        nonce;      // nonce value
    char *              min_suffix;
    char *              max_suffix;
    char *              scope;
    int                 mustbe_fresh; // TODO: I think ccnlite does not use this yet
    char *              ppkl;       // publisher public key locator
    // ... other wished ...
};

// interest type object for ccntlv
struct info_interest_ccnTlv_s {
    struct info_data_s  base;

    // ccntlv specific obj attribs
    unsigned int        nonce;      // nonce value
    // ... other wished ... (publisher keyid, content digest, etc)
};

// interest type object for ccnb
struct info_interest_ccnXmlb_s {
    struct info_data_s   base;

    // TODO: possibly the following ptrs may need to be changed to int * (I ve used what I saw in the ccnlite code)
    // ccnb specific I obj attribs
    unsigned int        nonce;          // nonce value
    char *              min_suffix;
    char *              max_suffix;
    char *              scope;
    char *              content_digest; // in case different content with the same name, select based on content digest the right one
    char *              ppkd;           // publisher public key (digest)
    //int                aok;           // answer origin kind
    // ... other wished ...
};

// content type object for ndntlv
struct info_content_ndnTlv_s {
    struct info_data_s  base;
    
    // ndntlv content obj attribs
    unsigned char *     content_bytes;
    unsigned int        content_len;
    unsigned char *     signature_bytes;    // signature of C pkt generated using publisher key
    unsigned int        signature_len;
    unsigned char *     siginfo_bytes;      // typically the key locator and other info, for the generation of the signatures (replaces ppkd in ccnx xmlb which is an actual key instead of a key location)
    unsigned int        siginfo_len;
    unsigned char *     sigkey_bytes;       // actual public/secret key with which either received content has been signed, or published content is to be signed (acquired from the siginfo_bytes location)
    unsigned int        sigkey_len;
    int                 freshness_msec;     // content freshness time before becoming stale (not sure ccn-lite pays attention to this)
    // .. other wished ...
};

// content type object for ccntlv
struct info_content_ccnTlv_s {
    struct info_data_s  base;

    // ccntlv content obj attribs
    unsigned char *     content_bytes;
    unsigned int        content_len;
    unsigned char *     keyid_bytes;        // publisher key with which either received content has been signed, or publishable content is to be signed
    unsigned int        keyid_len;
    // .. other wished ...
};

// content type object for ccnb
struct info_content_ccnXmlb_s {
    struct info_data_s  base;
    
    // ccnb content obj attribs
    unsigned char *     content_bytes;      // C pkt payload
    unsigned int        content_len;
    unsigned char *     signature_bytes;    // signature of C pkt generated using publisher (secret) key
    unsigned int        signature_len;
    unsigned char *     ppkd_bytes;         // publisher pub key digest with which either received content has been signed, or publishable content is to be signed
    unsigned int        ppkd_len;
    int                 freshness_sec;      // content freshness time before becoming stale
    unsigned int        timestamp;
    // .. other wished expressed ...
};


/*
 * vtable for making the I/C type objects polymorphic from the base object.
 * In UAPI a global instance of this vtable is created for each sub-type
 *
 * generic functions on I/C data objects need this information to
 *  - know how to handle mem allocs
 *  - access the right vtable (suite funcs)
 *  - do correct/safe casting
 *  - assert the state/correctness of the info passed to them
 */
struct info_data_vt_s {
    int const                           size;
    info_data_t const                   type;   // I / C
    icn_suite_t const                   suite;  // NDN_TLV, CCN_TLV, CCN_XMLB (redundant but fast indexed)
    struct suite_vt_s const * const     suite_vtable;   // 2nd level vtable with suite specific functions

    void *  (* ctor) (void *self);      // polymorphic create/init
    void    (* dtor) (void *self);      // polymorphic destroy
};


/*
 * 2nd level vtable (under info_data_vt_s) with per suite functions
 * for processing I/C types
 * In this way each suite implements and exposes through the generic
 * UAPI its own custom pkt mgmt functions. An instance of this vtable
 * is created for each suite.
 */
struct suite_vt_s {
    // gen an I/C packet from a data object (the packet is made available at the base info_data_s object's packet_bytes/packet_len members)
    int  (* makeInterest) (struct info_data_s *);
    int  (* makeContent) (struct info_data_s *);

    // TODO: parse an I/C packet and populate a data object (the packet bytes are expected at the base info_data_s object's packet_bytes/packet_len members)
    struct info_data_s *  (* readInterest) (struct info_data_s *);
    struct info_data_s *  (* readContent) (struct info_data_s *);

    // TODO: define other needed ones ...
};








/*****************************************************************************
 * object-based type hierarchy for mgmt objects (used to configure the
 * internal state of the ccn-lite relay)
 *
 * Each of these structs holds mgmgt info relevant to the CS/FIB/IFACES
 * subsystems of a relay. The base object info_mgmt_s common to all, allows 
 * generic handling through the functions addMgmt/remMgmt/getMgmt/setMgmt and 
 * the (de)allocation of mgmt objects with mallocMgmtInfo/freeMgmtInfo
 *****************************************************************************/

// base type object for all mgmt subsystems
struct info_mgmt_s {
    struct info_mgmt_vt_s const *vtable;
    void                        *relay;
    struct info_mgmt_s          *next;
};

// CS subsystem conf type object
struct info_cs_data_s {
    struct info_mgmt_s     base;

    const char *           name;        // name registration for the content
    int                    chunk_seqn;  // optionally provided chunk num (if this is present the data_len MUST be <= max chunk size, if not then ccn-lite may kindly do segmentation & chunk numbering for us ?)
    int                    version_seqn;// optionally provided version num (TODO: not in use currently)
    unsigned char *        data_bytes;  // data to be split in content chunks
    unsigned int           data_len;    // data len
    icn_suite_t            suite;       // CS data are stored as formatted packets so we need to have a ref to pkt format suite
};

// FIB subsystem conf type object
struct info_fib_rule_s {
    struct info_mgmt_s      base;

    const char              *prefix;
    int                     dev_id; // depending on the structure of the FIB this may point to a face or an interface
    sockunion               next_hop;
    icn_suite_t             suite;
};

// interface subsystem conf type object
struct info_iface_conf_s {
    struct info_mgmt_s      base;

    sockunion               addr;
    int                     iface_id;	// id of interface struct in ccnlite
    int                     tx_pace;
    char                    status_up;  // face up(1)/down(0)
};

// vtable for making the different subsystem type objects polymorphic from the base object
struct info_mgmt_vt_s {
    int const               size;
    mgmt_subsys_t const     subsys;  // MGMT_CS, MGMT_FIB, MGMT_IFACE

    void *  (* ctor) (void *self);      // polymorphic create/init
    void    (* dtor) (void *self);      // polymorphic destroy

    int     (* add) (struct info_mgmt_s *mgmt);
    int     (* rem) (struct info_mgmt_s *mgmt);
    int     (* get) (struct info_mgmt_s *mgmt);
    int     (* set) (struct info_mgmt_s *mgmt);
};





/*****************************************************************************
 * named type declarations for the different types of objects (assignments to
 * type-description objects are in the ccnl-uapi.c)
 *****************************************************************************/

extern const void * info_interest_ndnTlv_s;
extern const void * info_content_ndnTlv_s;
extern const void * info_interest_ccnTlv_s;
extern const void * info_content_ccnTlv_s;
extern const void * info_interest_ccnXmlb_s;
extern const void * info_content_ccnXmlb_s;

extern const void * info_cs_data_s;
extern const void * info_fib_rule_s;
extern const void * info_iface_conf_s;






/*****************************************************************************
 * ccn lite uapi expects the following API to be implemented by the host 
 * platform
 *
 * Essentially the hosting platform code base should define and initialise the
 * "platform_srvcs" global vtable, with the functions providing the respective
 * platform services and callbacks that ccnlite needs.
 *****************************************************************************/


struct platform_vt_s {
    void    (* deliver_object) (void *relay, struct info_data_s const * io, struct envelope_s * env);    // MUST
    void    (* log_start) (struct ccnl_relay_s *relay, char * node_name);                          // MAY
    void    (* log_stop) (struct ccnl_relay_s *relay);                           // MAY
    void *  (* set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2 );     // MUST
    void *  (* set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2); // MUST
    void    (* rem_timer) (void *hdl);      // MUST
    double  (* get_time_now) ();            // MUST
    char *  (* get_timestamp) ();           // MUST if using debugging
    void    (* get_timeval) (struct timeval *tv);   // MUST
    void    (* log) (char *logmsg);  // MUST
    void    (* print_stats) (struct ccnl_relay_s *relay, int code); // MAY
    void    (* close_socket) (int s);   // MUST only if socket based faces are used
};

// actual vtable instance that uapi uses to acces the host platform services
extern struct platform_vt_s platform_srvcs;









/*****************************************************************************
 * The generic API (uapi) currently provides the following service functions
 * at the ccn-lite client side
 *****************************************************************************/

/* generic functions for all suites, both data objects (I/C) */
int                         suiteStr2Id(char *s);
static inline const char *  suiteId2Str(int id) {return (id >= END_SUITE_T) ? NULL : icn_suite_str_by_fmt[id]; };
static inline icn_suite_t   readSuite (const struct info_data_s *o) {return o->vtable->suite; };
static inline const char *  printSuite (const struct info_data_s *o) {return icn_suite_str[o->vtable->suite]; };
static inline info_data_t   readTypeIorC (const struct info_data_s *o) {return o->vtable->type; };
static inline const char *  printTypeIorC (const struct info_data_s *o) {return pkt_type_str_by_suite[o->vtable->type]; };
static inline int           createPacketI (struct info_data_s *o) {return o->vtable->suite_vtable->makeInterest(o); };
static inline int           createPacketC (struct info_data_s *o) {return o->vtable->suite_vtable->makeContent(o); };
struct info_data_s *        parsePacketI (char *pkt_buf, int pkt_len);      // parse an I suite-formatted packet and gen respective info struct
struct info_data_s *        parsePacketC (char *pkt_buf, int pkt_len);       // parse a C suite-formatted packet and gen respective info struct
struct info_data_s *        mallocSuiteInfo (const void *obj_descr);        // allocate the right object type info struct and presets some fields
void                        freeSuiteInfo (struct info_data_s *o);          // free mem of an object type
int                         processSuiteData (void *relay, struct info_data_s *o, sockunion * from_addr, int rx_iface);  // pass an I/C suite-formatted pkt to ccn-lite


/* generic functions for relay mgmt subsystems (CS/FIB/FACES) */
void *                      createRelay (const char * node_name, int cs_inodes, int cs_bytesize, char enable_stats, void *aux);  // malloc relay state
void                        destroyRelay (void *relay);                     // dispose relay state and free
void                        startRelay (void *relay);                       // start relay op with remembered state
void                        stopRelay (void *relay);                        // pause relay op but do not dispose state
struct info_mgmt_s *        mallocMgmtInfo (const void *mgmt_descr);        // create the right mgmt info struct and presets some attribs
void                        freeMgmtInfo (struct info_mgmt_s *m);           // delete mem of a mgmt info struct
static inline int           addMgmt (struct info_mgmt_s *m) {return m->vtable->add(m); };   // add cs data/fib rule/iface config
static inline int           remMgmt (struct info_mgmt_s *m) {return m->vtable->rem(m); };   // delete cs data/fib rule/iface config
static inline int           getMgmt (struct info_mgmt_s *m) {return m->vtable->get(m); };   // read cs data/fib rule/iface config
static inline int           setMgmt (struct info_mgmt_s *m) {return m->vtable->set(m); };   // update cs data/fib rule/iface config





/*****************************************************************************
 * Finally here is an attempt to assemble a reasonably general purpose 
 * ccn-lite instantiation, for someone that merely wants to include only
 * ccnl-uapi.h and have everything in place
 *****************************************************************************/


#include "ccnl-ext.h"
#include "ccnl-ext-debug.c"
#include "ccnl-ext-logging.c"
#define local_producer(...)             0
#define cache_strategy_remove(...)      0

/*
 * The following are aliases for legacy names that the ccn-lite code base uses,
 * as well as redefs of some macro functions
 *
 * TODO: ideally these need to be eliminated but legacy may not allow to 
 */
#ifdef PLATFORM_SRVCS
#  undef CCNL_NOW
#  define CCNL_NOW() (platform_srvcs.get_time_now())

# ifndef PLATFORM_LOG_THRESHOLD
    // NOTE that the 'debug_level' global threshhold (currently in ccnl-ext-logging.c), 
    // has no default value (that is console output is /dev/null-ed unless otherwise 
    // told in an instantiation of ccn-lite)! It may be set to a value 0-99 or to one 
    // of {FATAL, ERROR, WARNING, INFO, DEBUG, VERBOSE, TRACE} (which strangely mixes
    // type of debug info with severity levels !?!?).

    // Unless there is a different logging threshold defined/desired for the host 
    // platform logger (e.g. Omnet's EV logger) than the one used for the console 
    // (by ccn-lite), we send the same debug output to both by modifying the DEBUGMSG()
    // macro.
#   define PLATFORM_LOG_THRESHOLD  debug_level
# endif //PLATFORM_LOG_THRESHOLD

# ifdef DEBUGMSG
# undef DEBUGMSG
# define DEBUGMSG(LVL, ...) do {   \
      if ((LVL)<=debug_level) \
      { \
        fprintf(stderr, "[%c] %s: ", ccnl_debugLevelToChar(LVL), timestamp());   \
        fprintf(stderr, __VA_ARGS__);   \
      } \
      if ((LVL)<=PLATFORM_LOG_THRESHOLD) \
      { \
        char pbuf[200]; \
        sprintf(pbuf, __VA_ARGS__);  \
        platform_srvcs.log(pbuf); \
      } \
      break; \
    } while (0)
# endif // DEBUGMSG


  void * (* ccnl_set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2) = platform_srvcs.set_timer;
  void * (* ccnl_set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2) = platform_srvcs.set_absolute_timer;
  void   (* ccnl_rem_timer) (void *hdl) = platform_srvcs.rem_timer;
  void   (* ccnl_get_timeval) (struct timeval *tv) = platform_srvcs.get_timeval;
  void   (* ccnl_print_stats) (struct ccnl_relay_s *relay, int code) = platform_srvcs.print_stats;
  inline char * timestamp(void) {return platform_srvcs.get_timestamp();};
  inline void   ccnl_close_socket (int s) { if (platform_srvcs.close_socket) platform_srvcs.close_socket(s); };
#endif // PLATFORM_SRVCS



  void    ccnl_ll_TX (struct ccnl_relay_s *relay, struct ccnl_if_s *ifc, sockunion *dst, struct ccnl_buf_s *buf);
  void    ccnl_app_RX(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);

#define NEEDS_PREFIX_MATCHING
#define NEEDS_PACKET_CRAFTING
//#define USE_HMAC256           // this seems to be an incomplete feature at the moment

#include "ccnl-core.c"
#include "ccnl-ext-echo.c"
#include "ccnl-ext-hmac.c"
#include "ccnl-ext-http.c"
#include "ccnl-ext-localrpc.c"
#include "ccnl-ext-mgmt.c"
#include "ccnl-ext-sched.c"
#include "ccnl-ext-frag.c"
#include "ccnl-ext-crypto.c"
#include "ccnl-uapi.c"


#endif /* CCNL_UAPI_H_ */
