/*
 * krivine-common.h
 * Tools for the "Krivine lambda expression resolver" for CCN
 *
 * (C) 2014 <christian.tschudin@unibas.ch>
 *
 * 2014-04-04 created <christopher.scherb@unibas.ch>
 */

#ifndef KRIVINE_COMMON_H
#define KRIVINE_COMMON_H


#define NFN_MAX_RUNNING_COMPUTATIONS 10
#define NFN_DEFAULT_WAITING_TIME 10
int numOfRunningComputations = 0;


#define STACK_TYPE_INT 0
#define STACK_TYPE_PREFIX 1
#define STACK_TYPE_THUNK 2
#define STACK_TYPE_CLOSURE 3

struct stack_s{
    void *content;
    int type;
    struct stack_s *next;
};

struct environment_s{
    char *name;
    void *element;
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
    int thunk_request;
    int num_of_required_thunks;
    char *thunk;
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
    int thunk;
    double thunk_time;
};

struct thunk_s{
    struct thunk_s *next, *prev;
    char thunkid[10];
    struct ccnl_prefix_s *prefix;
    struct ccnl_prefix_s *reduced_prefix;
};

struct thunk_s *thunk_list;
int thunkid = 0;

struct configuration_s *configuration_list;

int configid = -1;


struct ccnl_content_s *
create_content_object(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
        unsigned char *contentstr, int contentlen, int suite);

void ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                     struct ccnl_face_s *from, int suite);

#endif //KRIVINE_COMMON_H
