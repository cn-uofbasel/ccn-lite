/*
 * ccnl-uapi.h
 *
 *  Created on: Oct 24, 2014
 *      Author: manolis
 */



/*****************************************************************************
 *                              OVERVIEW
 *                              ========
 *
 * ccn-lite expects the following API to be implemented by the host platform
 *
 * struct platform_vt_s {
 *   void    (* deliver_object) (void *relay, struct info_data_s const * io, struct envelope_s * env);   // MUST
 *   void    (* log_start) (struct ccnl_relay_s *relay, char * node_name);                         // MAY
 *   void    (* log_stop) (struct ccnl_relay_s *relay);                                            // MAY
 *   void *  (* set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2 );    // MUST
 *   void *  (* set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2); // MUST
 *   void    (* rem_timer) (void *hdl);                                 // MUST
 *   double  (* get_time_now) ();                                       // MUST
 *   void    (* get_timeval) (struct timeval *tv);                      // MUST
 *   void    (* log) (char *msg);                                       // MUST
 *   void    (* print_stats) (struct ccnl_relay_s *relay, int code);    // MAY
 *   void    (* close_socket) (int s);                                  // MUST only if socket based faces are used
 * };
 *
 * Essentially the hosting platform code should define and initialise the
 * "platform_srvcs" global vtable, with the functions providing the respective
 * platform services and callbacks that ccnlite needs. Additionally it should
 * set the PLATFORM_SRVCS def.
 *
 * E.g. in omnet I have something like the following
 *
 * #define PLATFORM_SRVCS
 *
 * struct platform_vt_s platform_srvcs = {
 *       opp_deliver,
 *       0,
 *       0,
 *       opp_set_timer,
 *       opp_set_absolute_timer,
 *       opp_rem_timer,
 *       opp_time,
 *       opp_get_timeval,
 *       opp_log,
 *       opp_print_stats,
 *       0
 * };
 *
 * ---------------------------------------------------------------------------
 *
 * The host platform on the other hand can use the following services
 *
 * icn_suite_t           readSuite (struct info_data_s *o);
 * const char *          printSuite (struct info_data_s *o);
 * int                   createInterest (struct info_data_s *o);
 * int                   createContent (struct info_data_s *o);
 * struct info_data_s *   parseInterest(char *pkt_buf, int pkt_len); // parse an I suite-formatted obj and gen respective info struct
 * struct info_data_s *   parseContent(char *pkt_buf, int pkt_len);  // parse a C suite-formatted obj and gen respective info struct
 * struct info_data_s *   mallocSuiteInfo (const void *obj_descr);   // creates the right object type info struct and presets some fields
 * void                  freeSuiteInfo (struct info_data_s *o);      // frees mem of a respective object type
 * int                   processObject (void *relay, struct info_data_s *o, sockunion * from);  // pass an I/C suite-formatted pkt to ccn-lite
 *
 * void *                createRelay (const char * node_name, int cs_inodes, int cs_bytesize, char enable_stats, void *aux);  // malloc relay state
 * void                  destroyRelay (void *relay);                // dispose relay state and free
 * void                  startRelay (void *relay);                  // start relay op with remembered state
 * void                  stopRelay (void *relay);                   // pause relay op but do not dispose state
 * struct info_mgmt_s *  mallocMgmtInfo (const void *mgmt_descr);   // create the right mgmt info struct and presets some attribs
 * void                  freeMgmtInfo (struct info_mgmt_s *m);      // delete mem of a mgmt info struct
 * int                   addMgmt (struct info_mgmt_s *m);           // add cs data/fib rule/face config
 * int                   remMgmt (struct info_mgmt_s *m);           // delete cs data/fib rule/face config
 * int                   getMgmt (struct info_mgmt_s *m);           // read cs data/fib rule/face config
 * int                   setMgmt (struct info_mgmt_s *m);           // update cs data/fib rule/face config
 *
 * ---------------------------------------------------------------------------
 *
 * The whole information exchange between the host platform and ccn-lite is
 * implemented by two types of objects and their sub-classed objects
 *
 * info_data_s : base class for I or C of any suite
 *   |
 *   |    (derived objects)
 *   +- info_interest_ndnTlv_s
 *   +- info_content_ndnTlv_s
 *   +- info_interest_ccnTlv_s
 *   +- info_content_ccnTlv_s
 *   +- info_interest_ccnXmlb_s
 *   +- info_content_ccnXmlb_s
 *
 *   they can be created/destroyed with mallocSuiteInfo()/freeSuiteInfo() and
 *   using directly their type. Eg.
 *      obj = mallocSuiteInfo(info_content_ccnTlv_s)
 *      freeSuiteInfo (obj)
 *
 *
 * info_mgmt_s : base class for exchanging info with the FIB/CS/FACES subsys
 *   |
 *   |    (derived objects)
 *   +- info_cs_data_s
 *   +- info_fib_rule_s
 *   +- info_face_conf_s
 *
 *   they can be created/destroyed with mallocMgmtInfo()/freeMgmtInfo()
 *
 *****************************************************************************/

#ifndef CCNL_UAPI_H_
#define CCNL_UAPI_H_






/*****************************************************************************
 * various enum type defs
 *****************************************************************************/

// suite types and description strings
typedef enum { CCNx_0=1, CCN_XMLB=1,
               CCNx_1=2, CCN_TLV=2,
               NDNx_0=3, NDN_TLV=3
} icn_suite_t;
//
const char  *icn_suite_str[] = {"N/A", "CCNx_0", "CCNx_1", "NDNx_0"};
const char  *icn_suite_str_by_fmt[] = {"N/A", "ccnb", "ccntlv", "ndntlv"};
const char  *icn_suite_str_by_year[] = {"N/A", "CCNx2013", "CCNx2014", "NDNx2014"};

// data object types (I/C) by suite
typedef enum { I_CCNx_0=1, C_CCNx_0,
               I_CCNx_1, C_CCNx_1,
               I_NDNx_0, D_NDNx_0
} info_data_t;
//
const char *pkt_type_str_by_suite[]= {"N/A", "I_CCNx_0","C_CCNx_0","I_CCNx_1","C_CCNx_1","I_NDNx_0","D_NDNx_0"};

// mgmt subsystem type
typedef enum{
    MGMT_CS,
    MGMT_FIB,
    MGMT_FACE
} mgmt_subsys_t;

// mgmt command type
typedef enum{
    MGMT_C_ADD,
    MGMT_C_REM,
    MGMT_C_GET,
    MGMT_C_SET
} mgmt_cmd_t;

// identifiers for logging different types of statistics (used by ccn-lite, not the UAPI)
enum {
    STAT_RCV_I,
    STAT_RCV_C,
    STAT_SND_I,
    STAT_SND_C,
    STAT_QLEN,
    STAT_EOP1
};

// What should the platform do with data received by the ccn-lite kernel
// using the deliveryObject() callback, can be figured out by these types
typedef enum{
    ENV_LINK,   // put on the wire
    ENV_APP,    // pass to app
    ENV_STATS   // use for stats (copies of objects)
    // For NFN par example a different envelop type would be created
} envelope_t;




/*****************************************************************************
 * ccn-lite passes i/c objects to the platform using the deliveryObject()
 * callback. Along side it passes "envelope info" to help the platform know
 * how to handle them. Different envelope types are identified on the
 * platform side using the envelope_t enum
 *****************************************************************************/

//.. and here is a polymorphic envelope struct to hold the envelope info

struct envelope_s
{
    envelope_t  to;
    union {
        // ENV_LINK
        struct {
            sockunion sa_local;
            sockunion sa_remote;
        } link;

        // ENV_STATS
        struct {} app;

        // ENV_APP
        struct {} stats;
    };
};







/*****************************************************************************
 *
 * classes and subclasses for various types of I and C objects
 *
 * Each of these is to be used for holding data and header attribs
 * for i and c objects of the different dialects. They are all handled
 * by the API generically through the base info_data_s
 *
 *****************************************************************************/

// base object
struct info_data_s {
    struct info_data_vt_s const *    vtable;
    const char *                    name;               // user can provide the whole name (prefix) as a string and we take care of componentisation, .. or ...
    const char *                    name_components[];  // the name components as a set of '\0'-terminated strings (useful for nfn) -- last component ptr MUST be 0
    int                             chunk_seqn;         // optionally provided chunk num
                                                        //  Since chunk naming is somewhat arbitrary (e.g. ../cN, ../N, ../c_N, etc), and left
                                                        //  to a sender/receiver contract, to reduce ambiguity here we assume that the chunk
                                                        //  seq num SHOULD be present whenever the last component provided in "name" or
                                                        //  "name_components" is known to be identifying a chunk.
    unsigned char *                 pkt_buf;    // buffer holding the packet
    unsigned int                    pkt_len;    // packet buffer len
};

// interest object for ndntlv
struct info_interest_ndnTlv_s {
    struct info_data_s  base;
    // TODO: ndntlv specific obj attribs
};

// interest object for ccntlv
struct info_interest_ccnTlv_s {
    struct info_data_s  base;
    // TODO: ccntlv specific obj attribs
};

// interest object for ccnb
struct info_interest_ccnXmlb_s {
    struct info_data_s   base;

    int                 nonce;      // nonce value
    // TODO: other ccnb specific obj attribs
};

// content object for ndntlv
struct info_content_ndnTlv_s {
    struct info_data_s  base;
    // TODO: ndntlv content obj attribs
};

// content object for ccntlv
struct info_content_ccnTlv_s {
    struct info_data_s  base;
    // TODO: ccntlv content obj attribs
};

// content object for ccnb
struct info_content_ccnXmlb_s {
    struct info_data_s  base;
    // TODO: ccnb content obj attribs
};


/*
 * class descriptor (+vtable) for each I/C object. In ccnl-uapi.c
 * a vtable of this struct is instantiated for each subclass
 *
 * generic functions need this information to
 *  - know how to handle mem allocs
 *  - access the right vtable (suite funcs)
 *  - do correct/safe casting
 *  - assert the state/correctness of the info passed to them
 */
struct info_data_vt_s {
    int const                           size;
    info_data_t const                    type;   // I / C
    icn_suite_t const                   suite;  // NDN_TLV, CCN_TLV, CCN_XMLB (redundant but fast index)
    struct suite_vt_s const * const     suite_vtable;

    void *  (* ctor) (void *self);      // polymorphic how to create/init
    void    (* dtor) (void *self);      // polymorphic how to destroy
};


/*
 * sub-vtable of functions that all I/C dialects implement and expose in the API
 * In ccnl-uapi.c a vtable is instantiated for each suite's (dialect) functions
 */
struct suite_vt_s {
    int  (* makeInterest) (struct info_data_s *);   //
    int  (* makeContent) (struct info_data_s *);    //

    struct info_data_s *     (* readInterest) (char *buf);  // TODO: needed ?
    struct info_data_s *     (* readContent) (char *buf);   // TODO: needed ?

    // TODO: define other needed ones ...
};








/*****************************************************************************
 * various relay mgmt object classes and subclasses
 *
 * Each of these structs holds mgmgt info relevant to the CS/FIB/FACES
 * subsystems of a relay. The base struct info_mgmt_s allows the use of
 * their handling through the generic set of functions addMgmt/remMgmt/
 * getMgmt/setMgmt and their abstracted away (de)allocation with
 * mallocMgmtInfo/freeMgmtInfo
 *
 *****************************************************************************/

//TODO: Rename everywhere info_mgmt to mgmt_obj

struct info_mgmt_s {
    struct info_mgmt_vt_s const *vtable;
    void                        *relay;
    struct info_mgmt_s          *next;
};

struct info_cs_data_s {
    struct info_mgmt_s     base;

    const char             *name;
    char                   *data_buf;
    unsigned int           data_size;
};

struct info_fib_rule_s {
    struct info_mgmt_s      base;

    const char              *prefix;
    int                     dev_id;
    sockunion               next_hop;
};

struct info_face_conf_s {
    struct info_mgmt_s      base;

    sockunion               addr;
    int                     tx_pace;
    char                    status_up;     // face up(1)/down(0)
};


/*
 * class description (+vtable) for the different mgmt subsystems
 */
struct info_mgmt_vt_s {
    int const               size;
    mgmt_subsys_t const     subsys;  // MGMT_CS, MGMT_FIB, MGMT_FACE

    void *  (* ctor) (void *self);      // polymorphic how to create/init
    void    (* dtor) (void *self);      // polymorphic how to destroy

    int     (* add) (struct info_mgmt_s *mgmt);
    int     (* rem) (struct info_mgmt_s *mgmt);
    int     (* get) (struct info_mgmt_s *mgmt);
    int     (* set) (struct info_mgmt_s *mgmt);
};





/*****************************************************************************
 * Type declarations for the different types of objects
 *****************************************************************************/

/* type decls for use as in mallocSuiteInfo()
 *      E.g. mallocSuiteInfo(info_interest_ndnTlv_s)
 */
extern const void * info_interest_ndnTlv_s;
extern const void * info_content_ndnTlv_s;
extern const void * info_interest_ccnTlv_s;
extern const void * info_content_ccnTlv_s;
extern const void * info_interest_ccnXmlb_s;
extern const void * info_content_ccnXmlb_s;

/* type decls for use in mallocMgmtInfo()
 *     E.g. mallocMgmtInfo(info_cs_data_s)
 */
extern const void * info_cs_data_s;
extern const void * info_fib_rule_s;
extern const void * info_face_conf_s;






/*****************************************************************************
 * ccn lite expects the following API to be implemented by the host platform
 *
 * Essentially the hosting platform code should define and initialise the
 * "platform_srvcs" global vtable, with the functions providing the respective
 * platform services and callbacks that ccnlite needs.
 *
 *****************************************************************************/

struct platform_vt_s {
    void    (* deliver_object) (void *relay, struct info_data_s const * io, struct envelope_s * env);    // MUST
    void    (* log_start) (struct ccnl_relay_s *relay, char * node_name);                          // MAY
    void    (* log_stop) (struct ccnl_relay_s *relay);                           // MAY
    void *  (* set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2 );     // MUST
    void *  (* set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2); // MUST
    void    (* rem_timer) (void *hdl);      // MUST
    double  (* get_time_now) ();            // MUST
    void    (* get_timeval) (struct timeval *tv);   // MUST
    void    (* log) (char *logmsg);  // MUST
    void    (* print_stats) (struct ccnl_relay_s *relay, int code); // MAY
    void    (* close_socket) (int s);   // MUST only if socket based faces are used
};
extern struct platform_vt_s platform_srvcs; // This is the vtable that uapi uses to acces the host platform services


/* The following are aliases to the above vtable functions, for names that the ccn-lite code base uses */
// TODO: ideally these need to be eliminated

#ifdef PLATFORM_SRVCS
 #undef CCNL_NOW
 #define CCNL_NOW() (platform_srvcs.get_time_now())

 #ifndef PLATFORM_LOG_THRESHOLD
 // Unless there is a different logging threshold defined for the host platform than the one for the console
 // the same debug output will be sent to both.
 // NOTE that the 'debug_level' global (which is define in ccnl-ext-debug.c) is not set by default (that is
 // console output goes to /dev/null unless otherwise told! It may be set to a value 0-99
 #define PLATFORM_LOG_THRESHOLD  debug_level
 #endif

 #undef DEBUGMSG
 #define DEBUGMSG(LVL, ...) do {   \
   if ((LVL)<=debug_level) \
   { \
       fprintf(stderr, "%s: ", timestamp());   \
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

 void * (* ccnl_set_timer) (int usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2) = platform_srvcs.set_timer;
 void * (* ccnl_set_absolute_timer) (struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2) = platform_srvcs.set_absolute_timer;
 void   (* ccnl_rem_timer) (void *hdl) = platform_srvcs.rem_timer;
 void   (* ccnl_get_timeval) (struct timeval *tv) = platform_srvcs.get_timeval;
 void   (* ccnl_print_stats) (struct ccnl_relay_s *relay, int code) = platform_srvcs.print_stats;

 void   ccnl_close_socket (int s) { if (platform_srvcs.close_socket) platform_srvcs.close_socket(s); };
#endif







/*****************************************************************************
 * The client API consists of the following functions
 *****************************************************************************/

/* generic functions for all suites both objects (I/C) */

static inline icn_suite_t   readSuite (const struct info_data_s *o) {return o->vtable->suite; };
static inline const char *  printSuite (const struct info_data_s *o) {return icn_suite_str[o->vtable->suite]; };
static inline info_data_t   readTypeIorC (const struct info_data_s *o) {return o->vtable->type; };
static inline const char *  printTypeIorC (const struct info_data_s *o) {return pkt_type_str_by_suite[o->vtable->type]; };
static inline int           createInterest (struct info_data_s *o) {return o->vtable->suite_vtable->makeInterest(o); };
static inline int           createContent (struct info_data_s *o) {return o->vtable->suite_vtable->makeContent(o); };
struct info_data_s *        parseInterest(char *pkt_buf, int pkt_len);      // parse an I suite-formatted obj and gen respective info struct
struct info_data_s *        parseContent(char *pkt_buf, int pkt_len);       // parse a C suite-formatted obj and gen respective info struct
struct info_data_s *        mallocSuiteInfo (const void *obj_descr);        // creates the right object type info struct and presets some fields
void                        freeSuiteInfo (struct info_data_s *o);           // frees mem of a respective object type
int                         processObject (void *relay, struct info_data_s *o, sockunion * from);  // pass an I/C suite-formatted pkt to ccn-lite


/* generic functions for all mgmt subsystems (CS/FIB/FACES) */

void *                      createRelay (const char * node_name, int cs_inodes, int cs_bytesize, char enable_stats, void *aux);  // malloc relay state
void                        destroyRelay (void *relay);                     // dispose relay state and free
void                        startRelay (void *relay);                       // start relay op with remembered state
void                        stopRelay (void *relay);                        // pause relay op but do not dispose state
struct info_mgmt_s *        mallocMgmtInfo (const void *mgmt_descr);        // create the right mgmt info struct and presets some attribs
void                        freeMgmtInfo (struct info_mgmt_s *m);           // delete mem of a mgmt info struct
static inline int           addMgmt (struct info_mgmt_s *m) {return m->vtable->add(m); };   // add cs data/fib rule/face config
static inline int           remMgmt (struct info_mgmt_s *m) {return m->vtable->rem(m); };   // delete cs data/fib rule/face config
static inline int           getMgmt (struct info_mgmt_s *m) {return m->vtable->get(m); };   // read cs data/fib rule/face config
static inline int           setMgmt (struct info_mgmt_s *m) {return m->vtable->set(m); };   // update cs data/fib rule/face config


#endif /* CCNL_UAPI_H_ */
