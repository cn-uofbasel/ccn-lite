//
// Created by Balázs Faludi on 27.06.17.
//

#ifndef CCNL_NFN_KRIVINE_H
#define CCNL_NFN_KRIVINE_H

#include "ccnl-buf.h"
#include "ccnl-relay.h"
#include "ccnl-nfn.h"
#include "ccnl-prefix.h"

struct ccnl_buf_s*
Krivine_reduction(struct ccnl_relay_s *ccnl, char *expression,
                  int start_locally,
                  struct configuration_s **config,
                  struct ccnl_prefix_s *prefix, int suite);

void
push_to_stack(struct stack_s **top, void *content, int type);

struct stack_s *
pop_from_stack(struct stack_s **top);

struct stack_s *
pop_or_resolve_from_result_stack(struct ccnl_relay_s *ccnl,
                                 struct configuration_s *config);

#endif //CCNL_NFN_KRIVINE_H
