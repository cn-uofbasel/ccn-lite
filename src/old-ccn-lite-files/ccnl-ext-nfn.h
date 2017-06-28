/*
 * @f ccnl-ext-nfn.h
 * @b header file for the NFN extension ("Krivine lambda expression resolver")
 *
 * Copyright (C) 2014, <christopher.scherb@unibas.ch>
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
 * 2014-04-04 created
 */

#ifndef CCNL_EXT_NFN_H
#define CCNL_EXT_NFN_H

// ----------------------------------------------------------------------
// config and constants

#define NFN_MAX_RUNNING_COMPUTATIONS    100
#define NFN_DEFAULT_WAITING_TIME        100

#define STACK_TYPE_MARK                 0
#define STACK_TYPE_INT                  1
#define STACK_TYPE_CONST                2
#define STACK_TYPE_PREFIX               3
#define STACK_TYPE_PREFIXRAW            4
#define STACK_TYPE_CLOSURE              5

// ----------------------------------------------------------------------
// data structures

struct const_s{
    int len;
    char str[1];
};

struct stack_s{
    void *content;
    int type;
    struct stack_s *next;
};

struct environment_s{
    int refcount;
    char *name;
    struct closure_s *closure;
    struct environment_s *next;
};

struct closure_s{
    char *term;
    struct environment_s *env;
};

struct prefix_mapping_s{
     struct prefix_mapping_s *next, *prev;
     struct ccnl_prefix_s *key;
     struct ccnl_prefix_s *value;
};

struct fox_machine_state_s{
    int num_of_params;
    struct stack_s **params;
    int it_routable_param;
    struct prefix_mapping_s *prefix_mapping;
};

struct configuration_s{
    int configid;
    int local_done;
    int suite;
    int start_locally;
    char *prog;
    struct stack_s *result_stack;
    struct stack_s *argument_stack;
    struct environment_s *env;
    struct environment_s *global_dict;
    struct fox_machine_state_s *fox_state;
    struct ccnl_prefix_s *prefix;

    struct configuration_s *next;
    struct configuration_s *prev;

    double starttime;
    double endtime;
};



struct ccnl_krivine_s {
    struct configuration_s *configuration_list;
    int configid; // = -1;
    int numOfRunningComputations; // = 0;
};

// ----------------------------------------------------------------------
// prototypes

int ccnl_nfn(struct ccnl_relay_s *ccnl, // struct ccnl_buf_s *orig,
             struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
             struct configuration_s *config, struct ccnl_interest_s *interest,
             int suite, int start_locally);

struct ccnl_content_s *
create_content_object(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
        unsigned char *contentstr, int contentlen, int suite);

void ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                     struct ccnl_face_s *from, int suite);

// built in function
typedef char* (*BIF)(struct ccnl_relay_s *ccnl, struct configuration_s *config,
               int *restart, int *halt, char *prog, char *pending,
                      struct stack_s **stack);

#define A_BIF(FCT) char* FCT(struct ccnl_relay_s *ccnl,\
        struct configuration_s *config, int *restart, int *halt,\
        char *prog, char *pending, struct stack_s **stack);

A_BIF(op_builtin_add)
A_BIF(op_builtin_find)
A_BIF(op_builtin_mult)
A_BIF(op_builtin_raw)
A_BIF(op_builtin_sub)

struct builtin_s {
    char *name;
    BIF fct;
    struct builtin_s *next;
};

#endif // CCNL_EXT_NFN_H
