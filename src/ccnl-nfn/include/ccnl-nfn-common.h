//
// Created by Balázs Faludi on 27.06.17.
//

#ifndef CCNL_NFN_COMMON_H
#define CCNL_NFN_COMMON_H

#include "ccnl-core.h"

#include "ccnl-nfn.h"
#include "ccnl-nfn-requests.h"


int
ccnl_nfnprefix_isNFN(struct ccnl_prefix_s *p);

int
ccnl_nfnprefix_isKeepalive(struct ccnl_prefix_s *p);

int
ccnl_nfnprefix_isRequest(struct ccnl_prefix_s *p);

int
ccnl_nfnprefix_isRequestType(struct ccnl_prefix_s *p, enum nfn_request_type request_type);

int
ccnl_nfnprefix_isIntermediate(struct ccnl_prefix_s *p);

void
ccnl_nfnprefix_set(struct ccnl_prefix_s *p, unsigned int flags);

void
ccnl_nfnprefix_clear(struct ccnl_prefix_s *p, unsigned int flags);

struct ccnl_interest_s*
ccnl_nfn_interest_keepalive(struct ccnl_relay_s *relay, struct ccnl_interest_s *interest);

struct ccnl_content_s *
ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl,
                              struct configuration_s *config,
                              struct ccnl_prefix_s *prefix);

struct ccnl_content_s *
ccnl_nfn_result2content(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        unsigned char *resultstr, int resultlen);

void
set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref);

void
ccnl_nfn_freeConfiguration(struct configuration_s *c);

void
ccnl_nfn_reserveEnvironment(struct environment_s *env);

struct ccnl_prefix_s*
ccnl_nfnprefix_mkComputePrefix(struct configuration_s *config, int suite);

struct ccnl_prefix_s *
ccnl_nfnprefix_mkCallPrefix(struct ccnl_prefix_s *name,
                            struct configuration_s *config, int parameter_num);

void
ccnl_nfn_freeStack(struct stack_s* s);

struct ccnl_interest_s *
ccnl_nfn_query2interest(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        struct configuration_s *config);

struct ccnl_prefix_s *
create_prefix_for_content_on_result_stack(struct ccnl_relay_s *ccnl,
                                          struct configuration_s *config);

struct const_s *
ccnl_nfn_krivine_str2const(char *str);

void ccnl_nfn_freeClosure(struct closure_s *c);

void ccnl_nfn_releaseEnvironment(struct environment_s **env);

struct configuration_s *
new_config(struct ccnl_relay_s *ccnl, char *prog,
           struct environment_s *global_dict,
           int start_locally,
           struct ccnl_prefix_s *prefix, int configid, int suite);

int trim(char *str);

struct ccnl_interest_s*
ccnl_nfn_interest_remove(struct ccnl_relay_s *relay, struct ccnl_interest_s *i);

void
ccnl_nfn_freeKrivine(struct ccnl_relay_s *ccnl);

#endif //CCNL_NFN_COMMON_H
