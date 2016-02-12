#ifndef CCNL_HEADERS_H
#define CCNL_HEADERS_H


//data structure forward declarations

#include <sys/time.h>
//CCNL INCLUDES
#include "ccnl-os-includes.h"
#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "ccnl-ext-debug.h"
#ifdef USE_NFN
#include "ccnl-ext-nfn.h"
#endif

/* ccnl-core.c */
int ccnl_prefix_cmp(struct ccnl_prefix_s *name, unsigned char *md, struct ccnl_prefix_s *p, int mode);
int ccnl_addr_cmp(sockunion *s1, sockunion *s2);
char* ccnl_addr2ascii(sockunion *su);
struct ccnl_face_s *ccnl_get_face_or_create(struct ccnl_relay_s *ccnl, int ifndx, struct sockaddr *sa, int addrlen);
struct ccnl_face_s *ccnl_face_remove(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);
void ccnl_interface_cleanup(struct ccnl_if_s *i);
void ccnl_interface_CTS(void *aux1, void *aux2);
void ccnl_interface_enqueue(void (tx_done)(void *, int, int), struct ccnl_face_s *f, struct ccnl_relay_s *ccnl, struct ccnl_if_s *ifc, struct ccnl_buf_s *buf, sockunion *dest);
struct ccnl_buf_s *ccnl_face_dequeue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);
void ccnl_face_CTS_done(void *ptr, int cnt, int len);
void ccnl_face_CTS(struct ccnl_relay_s *ccnl, struct ccnl_face_s *f);
int ccnl_face_enqueue(struct ccnl_relay_s *ccnl, struct ccnl_face_s *to, struct ccnl_buf_s *buf);
struct ccnl_interest_s* ccnl_interest_new(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from, struct ccnl_pkt_s **pkt);
int ccnl_interest_append_pending(struct ccnl_interest_s *i, struct ccnl_face_s *from);
void ccnl_interest_propagate(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);
struct ccnl_interest_s *ccnl_interest_remove(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i);
int ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix, int minsuffix, int maxsuffix, struct ccnl_content_s *c);
struct ccnl_content_s *ccnl_content_new(struct ccnl_relay_s *ccnl, struct ccnl_pkt_s **pkt);
struct ccnl_content_s *ccnl_content_remove(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);
struct ccnl_content_s *ccnl_content_add2cache(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);
int ccnl_content_serve_pending(struct ccnl_relay_s *ccnl, struct ccnl_content_s *c);
void ccnl_do_ageing(void *ptr, void *dummy);
int ccnl_nonce_find_or_append(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *nonce);
void ccnl_core_RX(struct ccnl_relay_s *relay, int ifndx, unsigned char *data, int datalen, struct sockaddr *sa, int addrlen);
void ccnl_core_init(void);
//void ccnl_core_addToCleanup(struct ccnl_buf_s *buf);
void ccnl_core_cleanup(struct ccnl_relay_s *ccnl);
#ifndef ccnl_buf_new
struct ccnl_buf_s* ccnl_buf_new(void *data, int len);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-debug.c */

#ifdef USE_DEBUG
char *frag_protocol(int e);
void ccnl_dump(int lev, int typ, void *p);
int get_buf_dump(int lev, void *p, long *outbuf, int *len, long *next);
int get_prefix_dump(int lev, void *p, int *len, char **val);
int get_num_faces(void *p);
int get_faces_dump(int lev, void *p, int *faceid, long *next, long *prev, int *ifndx, int *flags, char **peer, int *type, char **frag);
int get_num_fwds(void *p);
int get_fwd_dump(int lev, void *p, long *outfwd, long *next, long *face, int *faceid, int *suite, int *prefixlen, char **prefix);
int get_num_interface(void *p);
int get_interface_dump(int lev, void *p, int *ifndx, char **addr, long *dev, int *devtype, int *reflect);
int get_num_interests(void *p);
int get_interest_dump(int lev, void *p, long *interest, long *next, long *prev, int *last, int *min, int *max, int *retries, long *publisher, int *prefixlen, char **prefix);
int get_pendint_dump(int lev, void *p, char **out);
int get_num_contents(void *p);
int get_content_dump(int lev, void *p, long *content, long *next, long *prev, int *last_use, int *served_cnt, int *prefixlen, char **prefix);
#endif

#ifdef USE_DEBUG_MALLOC
void *debug_malloc(int s, const char *fn, int lno, char *tstamp);
void *debug_calloc(int n, int s, const char *fn, int lno, char *tstamp);
int debug_unlink(struct mhdr *hdr);
void *debug_realloc(void *p, int s, const char *fn, int lno);
void *debug_strdup(const char *s, const char *fn, int lno, char *tstamp);
void debug_free(void *p, const char *fn, int lno);
#endif


//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-frag.c */
#ifdef USE_FRAG
int ccnl_ccnb_mkHeader(unsigned char *buf, unsigned int num, unsigned int tt);
int ccnl_ccnb_addBlob(unsigned char *out, char *cp, int cnt);
int ccnl_ccnb_mkBlob(unsigned char *out, unsigned int num, unsigned int tt, char *cp, int cnt);
int ccnl_ccnb_mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt, char *str);
int ccnl_ccnb_mkBinaryInt(unsigned char *out, unsigned int num, unsigned int tt, unsigned int val, int bytes);
int ccnl_ccnb_mkComponent(unsigned char *val, int vallen, unsigned char *out);
int ccnl_ccnb_mkName(struct ccnl_prefix_s *name, unsigned char *out);
int ccnl_ccnb_fillInterest(struct ccnl_prefix_s *name, int *nonce, unsigned char *out, int outlen);
int ccnl_ccnb_fillContent(struct ccnl_prefix_s *name, unsigned char *data, int datalen, int *contentpos, unsigned char *out);
struct ccnl_frag_s *ccnl_frag_new(int protocol, int mtu);
#endif


//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-http.c */
#ifdef USE_HTTP_STATUS
struct ccnl_http_s *ccnl_http_new(struct ccnl_relay_s *ccnl, int serverport);
struct ccnl_http_s *ccnl_http_cleanup(struct ccnl_http_s *http);
int ccnl_cmpfaceid(const void *a, const void *b);
int ccnl_cmpfib(const void *a, const void *b);
int ccnl_http_status(struct ccnl_relay_s *ccnl, struct ccnl_http_s *http);
#endif


//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-http.c */
#ifdef USE_MGMT
int ccnl_mgmt_send_return_split(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, int len, unsigned char *buf);
struct ccnl_prefix_s *ccnl_prefix_clone(struct ccnl_prefix_s *p);
void ccnl_mgmt_return_ccn_msg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, char *component_type, char *answer);
int ccnl_mgmt_debug(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_newface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_setfrag(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_destroyface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_newdev(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_destroydev(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_prefixreg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int pkt2suite(unsigned char *data, int len);
int ccnl_mgmt_addcacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_removecacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
int ccnl_mgmt_handle(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, char *cmd, int verified);
int ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from);
#endif


//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-nfn.c */
#ifdef USE_NFN
int ccnl_nfnprefix_isNFN(struct ccnl_prefix_s *p);
int ccnl_nfnprefix_isTHUNK(struct ccnl_prefix_s *p);
int ccnl_nfnprefix_contentIsNACK(struct ccnl_content_s *c);
void ccnl_nfnprefix_set(struct ccnl_prefix_s *p, unsigned int flags);
void ccnl_nfnprefix_clear(struct ccnl_prefix_s *p, unsigned int flags);
int ccnl_nfnprefix_fillCallExpr(char *buf, struct fox_machine_state_s *s, int exclude_param);
struct ccnl_prefix_s *ccnl_nfnprefix_mkCallPrefix(struct ccnl_prefix_s *name, int thunk_request, struct configuration_s *config, int parameter_num);
struct ccnl_prefix_s *ccnl_nfnprefix_mkComputePrefix(struct configuration_s *config, int suite);
struct ccnl_interest_s *ccnl_nfn_query2interest(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s **prefix, struct configuration_s *config);
struct ccnl_content_s *ccnl_nfn_result2content(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s **prefix, unsigned char *resultstr, int resultlen);
struct fox_machine_state_s *new_machine_state(int thunk_request, int num_of_required_thunks);
struct configuration_s *new_config(struct ccnl_relay_s *ccnl, char *prog, struct environment_s *global_dict, int thunk_request, int start_locally, int num_of_required_thunks, struct ccnl_prefix_s *prefix, int configid, int suite);
void ccnl_nfn_freeEnvironment(struct environment_s *env);
void ccnl_nfn_reserveEnvironment(struct environment_s *env);
void ccnl_nfn_releaseEnvironment(struct environment_s **env);
void ccnl_nfn_freeClosure(struct closure_s *c);
void ccnl_nfn_freeStack(struct stack_s *s);
void ccnl_nfn_freeMachineState(struct fox_machine_state_s *f);
void ccnl_nfn_freeConfiguration(struct configuration_s *c);
void ccnl_nfn_freeKrivine(struct ccnl_relay_s *ccnl);
int trim(char *str);
void set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref);
struct ccnl_prefix_s *create_prefix_for_content_on_result_stack(struct ccnl_relay_s *ccnl, struct configuration_s *config);
struct ccnl_content_s *ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl, struct configuration_s *config, struct ccnl_prefix_s *prefix);
char *ccnl_nfn_add_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, struct ccnl_prefix_s *prefix);
struct thunk_s *ccnl_nfn_get_thunk(struct ccnl_relay_s *ccnl, unsigned char *thunkid);
struct ccnl_interest_s *ccnl_nfn_get_interest_for_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, unsigned char *thunkid);
void ccnl_nfn_remove_thunk(struct ccnl_relay_s *ccnl, char *thunkid);
int ccnl_nfn_reply_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config);
struct ccnl_content_s *ccnl_nfn_resolve_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, unsigned char *thunk);
struct ccnl_interest_s *ccnl_nfn_interest_remove(struct ccnl_relay_s *relay, struct ccnl_interest_s *i);
void ZAM_register(char *name, BIF fct);
struct closure_s *new_closure(char *term, struct environment_s *env);
void push_to_stack(struct stack_s **top, void *content, int type);
struct stack_s *pop_from_stack(struct stack_s **top);
struct stack_s *pop_or_resolve_from_result_stack(struct ccnl_relay_s *ccnl, struct configuration_s *config);
void add_to_environment(struct environment_s **env, char *name, struct closure_s *closure);
struct closure_s *search_in_environment(struct environment_s *env, char *name);
int iscontentname(char *cp);
int choose_parameter(struct configuration_s *config);
struct ccnl_prefix_s *create_namecomps(struct ccnl_relay_s *ccnl, struct configuration_s *config, int parameter_number, int thunk_request, struct ccnl_prefix_s *prefix);
char *ZAM_term(struct ccnl_relay_s *ccnl, struct configuration_s *config, int thunk_request, int *num_of_required_thunks, int *halt, char *dummybuf, int *restart);
void allocAndAdd(struct environment_s **env, char *name, char *cmd);
void setup_global_environment(struct environment_s **env);
struct ccnl_buf_s *Krivine_reduction(struct ccnl_relay_s *ccnl, char *expression, int thunk_request, int start_locally, int num_of_required_thunks, struct configuration_s **config, struct ccnl_prefix_s *prefix, int suite);
void ZAM_init(void);
struct configuration_s *ccnl_nfn_findConfig(struct configuration_s *config_list, int configid);
void ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid, int continue_from_remove);
void ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, int suite);
int ccnl_nfn_thunk_already_computing(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix);
int ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, struct configuration_s *config, struct ccnl_interest_s *interest, int suite, int start_locally);
int ccnl_nfn_RX_result(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct ccnl_content_s *c);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-platform.c */
long timevaldelta(struct timeval *a, struct timeval *b);
#ifdef CCNL_LINUXKERNEL
int current_time(void);
#else
double current_time(void);
#endif
char *timestamp(void);
#if defined(CCNL_UNIX) || defined(CCNL_RIOT) || defined(CCNL_SIMULATION)
void ccnl_get_timeval(struct timeval *tv);
void *ccnl_set_timer(uint64_t usec, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2);
void *ccnl_set_absolute_timer(struct timeval abstime, void (*fct)(void *aux1, void *aux2), void *aux1, void *aux2);
void ccnl_rem_timer(void *h);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-sched.c */
#ifdef USE_SCHEDULER
int ccnl_sched_init(void);
void ccnl_sched_cleanup(void);
struct ccnl_sched_s *ccnl_sched_dummy_new(void (cts)(void *aux1, void *aux2), struct ccnl_relay_s *ccnl);
struct ccnl_sched_s *ccnl_sched_pktrate_new(void (cts)(void *aux1, void *aux2), struct ccnl_relay_s *ccnl, int inter_packet_interval);
void ccnl_sched_destroy(struct ccnl_sched_s *s);
void ccnl_sched_RTS(struct ccnl_sched_s *s, int cnt, int len, void *aux1, void *aux2);
void ccnl_sched_CTS_done(struct ccnl_sched_s *s, int cnt, int len);
void ccnl_sched_RX_ok(struct ccnl_relay_s *ccnl, int ifndx, int cnt);
void ccnl_sched_RX_loss(struct ccnl_relay_s *ccnl, int ifndx, int cnt);
struct ccnl_sched_s *ccnl_sched_packetratelimiter_new(int inter_packet_interval, void (*cts)(void *aux1, void *aux2), void *aux1, void *aux2);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-core-util.c */
const char* ccnl_suite2str(int suite);
int hex2int(unsigned char c);
int unescape_component(char *comp);
int ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri);
struct ccnl_prefix_s *ccnl_URItoPrefix(char *uri, int suite, char *nfnexpr, unsigned int *chunknum);
int ccnl_pkt_mkComponent(int suite, unsigned char *dst, char *src, int srclen);
struct ccnl_prefix_s *ccnl_prefix_dup(struct ccnl_prefix_s *prefix);
int ccnl_pkt2suite(unsigned char *data, int len, int *skip);
char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip, int escape_components, int call_slash);
char *ccnl_lambdaParseVar(char **cpp);
struct ccnl_lambdaTerm_s *ccnl_lambdaStrToTerm(int lev, char **cp, int (*prt)(char *fmt, ...));
int ccnl_lambdaTermToStr(char *cfg, struct ccnl_lambdaTerm_s *t, char last);
void ccnl_lambdaFreeTerm(struct ccnl_lambdaTerm_s *t);
int ccnl_lambdaStrToComponents(char **compVector, char *str);
struct ccnl_buf_s *ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce);
struct ccnl_buf_s *ccnl_mkSimpleContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, int *payoffset);
int ccnl_str2suite(char *cp);
int ccnl_add_fib_entry(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx, struct ccnl_face_s *face);
void ccnl_show_fib(struct ccnl_relay_s *relay);

//---------------------------------------------------------------------------------------------------------------------------------------
/* fwd-ccnb.c */
#ifdef USE_SUITE_CCNB
int ccnl_ccnb_dehead(unsigned char **buf, int *len, int *num, int *typ);
int ccnl_ccnb_data2uint(unsigned char *cp, int len);
struct ccnl_buf_s *ccnl_ccnb_extract(unsigned char **data, int *datalen, int *scope, int *aok, int *min, int *max, struct ccnl_prefix_s **prefix, struct ccnl_buf_s **nonce, struct ccnl_buf_s **ppkd, unsigned char **content, int *contlen);
int ccnl_ccnb_unmkBinaryInt(unsigned char **data, int *datalen, unsigned int *result, unsigned char *width);
int ccnl_ccnb_forwarder(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from, unsigned char **data, int *datalen);


//---------------------------------------------------------------------------------------------------------------------------------------
/* pkt-ccnb-dec.c */
int ccnl_ccnb_consume(int typ, int num, unsigned char **buf, int *len, unsigned char **valptr, int *vallen);
int ccnl_ccnb_data2uint(unsigned char *cp, int len);
struct ccnl_buf_s *ccnl_ccnb_extract(unsigned char **data, int *datalen, int *scope, int *aok, int *min, int *max, struct ccnl_prefix_s **prefix, struct ccnl_buf_s **nonce, struct ccnl_buf_s **ppkd, unsigned char **content, int *contlen);
int ccnl_ccnb_unmkBinaryInt(unsigned char **data, int *datalen, unsigned int *result, unsigned char *width);


//---------------------------------------------------------------------------------------------------------------------------------------
/* pkt-ccnb-enc.c */
int ccnl_ccnb_mkHeader(unsigned char *buf, unsigned int num, unsigned int tt);
int ccnl_ccnb_addBlob(unsigned char *out, char *cp, int cnt);
int ccnl_ccnb_mkBlob(unsigned char *out, unsigned int num, unsigned int tt, char *cp, int cnt);
int ccnl_ccnb_mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt, char *str);
int ccnl_ccnb_mkBinaryInt(unsigned char *out, unsigned int num, unsigned int tt, unsigned int val, int bytes);
int ccnl_ccnb_mkComponent(unsigned char *val, int vallen, unsigned char *out);
int ccnl_ccnb_mkName(struct ccnl_prefix_s *name, unsigned char *out);
int ccnl_ccnb_fillInterest(struct ccnl_prefix_s *name, int *nonce, unsigned char *out, int outlen);
int ccnl_ccnb_fillContent(struct ccnl_prefix_s *name, unsigned char *data, int datalen, int *contentpos, unsigned char *out);
#endif // USE_SUITE_CCNB


int ccnl_switch_prependCoding(unsigned int code, int *offset, unsigned char *buf);
int ccnl_switch_dehead(unsigned char **buf, int *len, int *code);
int ccnl_enc2suite(int enc);
//---------------------------------------------------------------------------------------------------------------------------------------
/* fwd-ccntlv.c */
#ifdef USE_SUITE_CCNTLV
int ccnl_ccntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from, unsigned char **data, int *datalen);


//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-pkt-ccnb.c */
int ccnl_ccnb_mkField(unsigned char *out, unsigned int num, int typ, unsigned char *data, int datalen);

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-pkt-ccntlv.c */
int ccnl_ccnltv_extractNetworkVarInt(unsigned char *buf, int len, unsigned int *intval);
int ccnl_ccntlv_dehead(unsigned char **buf, int *len, unsigned int *typ, unsigned int *vallen);
struct ccnl_buf_s *ccnl_ccntlv_extract(int hdrlen, unsigned char **data, int *datalen, struct ccnl_prefix_s **prefix, unsigned char **keyid, int *keyidlen, unsigned int *lastchunknum, unsigned char **content, int *contlen);
int ccnl_ccntlv_prependTL(unsigned short type, unsigned short len, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependBlob(unsigned short type, unsigned char *blob, unsigned short len, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependNetworkVarInt(unsigned short type, unsigned int intval, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependFixedHdr(unsigned char ver, unsigned char packettype, unsigned short payloadlen, unsigned char hoplimit, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependChunkName(struct ccnl_prefix_s *name, int *chunknum, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependChunkInterestWithHdr(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
int ccnl_ccntlv_prependName(struct ccnl_prefix_s *name, int *offset, unsigned char *buf, unsigned int *lastchunknum);
int ccnl_ccntlv_fillInterest(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
int ccnl_ccntlv_fillChunkInterestWithHdr(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
int ccnl_ccntlv_fillInterestWithHdr(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
int ccnl_ccntlv_fillContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, int *lastchunknum, int *offset, int *contentpos, unsigned char *buf);
int ccnl_ccntlv_fillContentWithHdr(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, int *lastchunknum, int *offset, int *contentpos, unsigned char *buf);
struct ccnl_buf_s* ccnl_ccntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed);
#endif // USE_SUITE_CCNTLV

//---------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_SUITE_CISTLV
int ccnl_cistlv_prependChunkInterestWithHdr(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_SUITE_IOTTLV
int ccnl_iottlv_dehead(unsigned char **buf, int *len, unsigned int *typ, int *vallen);
int ccnl_iottlv_peekType(unsigned char *buf, int len);
int ccnl_iottlv_prependRequest(struct ccnl_prefix_s *name, int *ttl, int *offset, unsigned char *buf);
struct ccnl_buf_s* ccnl_iottlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed);
#endif

//---------------------------------------------------------------------------------------------------------------------------------------
/* fwd-ndntlv.c */
#ifdef USE_SUITE_NDNTLV
unsigned long int ccnl_ndntlv_nonNegInt(unsigned char *cp, int len);
int ccnl_ndntlv_dehead(unsigned char **buf, int *len, int *typ, int *vallen);
int ccnl_ndntlv_forwarder(struct ccnl_relay_s *relay, struct ccnl_face_s *from, unsigned char **data, int *datalen);
int ccnl_ndntlv_prependInterest(struct ccnl_prefix_s *name, int scope, int *nonce, int *offset, unsigned char *buf);
//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-pkt-ndntlv.c */
unsigned long int ccnl_ndntlv_nonNegInt(unsigned char *cp, int len);
int ccnl_ndntlv_dehead(unsigned char **buf, int *len, int *typ, int *vallen);
struct ccnl_buf_s *ccnl_ndntlv_extract(int hdrlen, unsigned char **data, int *datalen, int *scope, int *mbf, int *min, int *max, unsigned int *final_block_id, struct ccnl_prefix_s **prefix, struct ccnl_prefix_s **tracing, struct ccnl_buf_s **nonce, struct ccnl_buf_s **ppkl, unsigned char **content, int *contlen);
int ccnl_ndntlv_prependTLval(unsigned long val, int *offset, unsigned char *buf);
int ccnl_ndntlv_prependTL(int type, unsigned int len, int *offset, unsigned char *buf);
int ccnl_ndntlv_prependNonNegInt(int type, unsigned int val, int *offset, unsigned char *buf);
int ccnl_ndntlv_prependBlob(int type, unsigned char *blob, int len, int *offset, unsigned char *buf);
int ccnl_ndntlv_prependName(struct ccnl_prefix_s *name, int *offset, unsigned char *buf);
int ccnl_ndntlv_prependContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, int *contentpos, unsigned int *final_block_id, int *offset, unsigned char *buf);
int ccnl_ndntlv_fillInterest(struct ccnl_prefix_s *name, int scope, int *nonce, int *offset, unsigned char *buf);
int ccnl_ndntlv_fillContent(struct ccnl_prefix_s *name, unsigned char *payload, int paylen, int *offset, int *contentpos, unsigned char *final_block_id, int final_block_id_len, unsigned char *buf);
struct ccnl_buf_s* ccnl_ndntlv_mkFrag(struct ccnl_frag_s *fr, unsigned int *consumed);
struct ccnl_pkt_s* ccnl_ndntlv_bytes2pkt(unsigned int pkttype, unsigned char *start, unsigned char **data, int *datalen);
#endif //USE_SUITE_NDNTLV

//---------------------------------------------------------------------------------------------------------------------------------------
/* fwd-localrpc.c */
#ifdef USE_SUITE_LOCALRPC
struct rdr_ds_s *ccnl_rdr_mkApp(struct rdr_ds_s *expr, struct rdr_ds_s *arg);
struct rdr_ds_s *ccnl_rdr_mkSeq(void);
struct rdr_ds_s *ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el);
struct rdr_ds_s *ccnl_rdr_mkNonNegInt(unsigned int val);
struct rdr_ds_s *ccnl_rdr_mkCodePoint(unsigned char cp);
struct rdr_ds_s *ccnl_rdr_mkVar(char *s);
struct rdr_ds_s *ccnl_rdr_mkBin(char *data, int len);
struct rdr_ds_s *ccnl_rdr_mkStr(char *s);
int ndn_tlv_fieldlen(unsigned long val);
int ccnl_rdr_getFlatLen(struct rdr_ds_s *ds);
int ccnl_rdr_serialize(struct rdr_ds_s *ds, unsigned char *buf, int buflen);
struct rdr_ds_s *ccnl_rdr_unserialize(unsigned char *buf, int buflen);
int ccnl_rdr_getType(struct rdr_ds_s *ds);
int ccnl_rdr_dump(int lev, struct rdr_ds_s *x);
int ccnl_emit_RpcReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *nonce, int rc, char *reason, struct rdr_ds_s *content);
struct rpc_exec_s *rpc_exec_new(void);
void rpc_exec_free(struct rpc_exec_s *exec);
int rpc_syslog(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *nonce, struct rpc_exec_s *exec, struct rdr_ds_s *param);
int rpc_forward(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *nonce, struct rpc_exec_s *exec, struct rdr_ds_s *param);
int rpc_lookup(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *nonce, struct rpc_exec_s *exec, struct rdr_ds_s *param);
int ccnl_localrpc_handleReturn(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *rc, struct rdr_ds_s *aux);
int ccnl_localrpc_handleApplication(struct ccnl_relay_s *relay, struct ccnl_face_s *from, struct rdr_ds_s *fexpr, struct rdr_ds_s *args);
int ccnl_localrpc_exec(struct ccnl_relay_s *relay, struct ccnl_face_s *from, unsigned char **buf, int *buflen);
void ccnl_rdr_free(struct rdr_ds_s *x);

//---------------------------------------------------------------------------------------------------------------------------------------
/* pkt-localrpc-dec.c */
struct rdr_ds_s *ccnl_rdr_unserialize(unsigned char *buf, int buflen);
int ccnl_rdr_getType(struct rdr_ds_s *ds);


//---------------------------------------------------------------------------------------------------------------------------------------
/* pkt-localrpc-enc.c */
struct rdr_ds_s *ccnl_rdr_mkApp(struct rdr_ds_s *expr, struct rdr_ds_s *arg);
struct rdr_ds_s *ccnl_rdr_mkSeq(void);
struct rdr_ds_s *ccnl_rdr_seqAppend(struct rdr_ds_s *seq, struct rdr_ds_s *el);
struct rdr_ds_s *ccnl_rdr_mkNonNegInt(unsigned int val);
struct rdr_ds_s *ccnl_rdr_mkCodePoint(unsigned char cp);
struct rdr_ds_s *ccnl_rdr_mkVar(char *s);
struct rdr_ds_s *ccnl_rdr_mkBin(char *data, int len);
struct rdr_ds_s *ccnl_rdr_mkStr(char *s);
int ndn_tlv_fieldlen(unsigned long val);
int ccnl_rdr_getFlatLen(struct rdr_ds_s *ds);
int ccnl_rdr_serialize(struct rdr_ds_s *ds, unsigned char *buf, int buflen);
#endif //USE_SUITE_LOCALRPC

//---------------------------------------------------------------------------------------------------------------------------------------
/* ccnl-ext-nfnmonitor.c */
#ifdef USE_NFN_MONITOR
void build_decoding_table(void);
char *base64_encode(const char *data, size_t input_length, size_t *output_length);
unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length);
void base64_cleanup(void);
int ccnl_ext_nfnmonitor_record(char *toip, int toport, struct ccnl_prefix_s *prefix, unsigned char *data, int datalen, char *res);
int ccnl_ext_nfnmonitor_udpSendTo(int sock, char *address, int port, char *data, int len);
int ccnl_ext_nfnmonitor_sendToMonitor(struct ccnl_relay_s *ccnl, char *content, int contentlen);
int ccnl_nfn_monitor(struct ccnl_relay_s *ccnl, struct ccnl_face_s *face, struct ccnl_prefix_s *pr, unsigned char *data, int len);
#endif //USE_NFN_MONITOR

#endif //CCNL_HEADERS_H
