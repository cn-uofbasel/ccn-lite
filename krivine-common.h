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
int numOfRunningComputations = 0;

struct fox_machine_state_s{
    int num_of_params;
    char **params;
    int it_routable_param;
    int thunk_request;
    int num_of_required_thunks;
    char *thunk;
};

struct configuration_s{
    int configid;
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
};

struct thunk_s{
    struct thunk_s *next, *prev;
    char thunkid[10];
    struct ccnl_prefix_s *prefix;
};

struct thunk_s *thunk_list;
int thunkid = 0;

struct configuration_s *configuration_list;

int configid = -1;

#endif //KRIVINE_COMMON_H