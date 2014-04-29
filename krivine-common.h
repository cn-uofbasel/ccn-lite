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

#endif //KRIVINE_COMMON_H