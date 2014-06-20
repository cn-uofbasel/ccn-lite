/*
 * @f ccn-lite-relay.c
 * @b user space CCN relay
 *
 * Copyright (C) 2011-14, Christian Tschudin, University of Basel
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
 * File history:
 * 2014-02-06 <christopher.scherb@unibas.ch>created 
 */

#ifndef CCNL_EXT_NFN_H
#define CCNL_EXT_NFN_H

#include "ccnl-core.h"
#include "krivine.c"
#include "krivine-common.c"


static int
ccnl_nfn_count_required_thunks(char *str)
{
    int num = 0;
    char *tok;
    tok = str;
    while((tok = strstr(tok, "call")) != NULL ){
        tok += 4;
        ++num;
    }
    DEBUGMSG(99, "Number of required Thunks is: %d\n", num);
    return num;
}

static void
ccnl_nfn_remove_thunk_from_prefix(struct ccnl_prefix_s *prefix){
    DEBUGMSG(49, "ccnl_nfn_remove_thunk_from_prefix()\n");
    prefix->comp[prefix->compcnt-2] =  prefix->comp[prefix->compcnt-1];
    prefix->complen[prefix->compcnt-2] =  prefix->complen[prefix->compcnt-1];
    --prefix->compcnt;
}

void 
ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid, int continue_from_remove){
    DEBUGMSG(49, "ccnl_nfn_continue_computation()\n");
    struct configuration_s *config = find_configuration(configuration_list, -configid);
    
    if(!config){
        return;
    }

    struct ccnl_interest_s *original_interest;
    for(original_interest = ccnl->pit; original_interest; original_interest = original_interest->next){
        if(!ccnl_prefix_cmp(config->prefix, 0, original_interest->prefix, CMP_EXACT)){
            original_interest->last_used = CCNL_NOW();
            original_interest->retries = 0;
            original_interest->from->last_used = CCNL_NOW();
            break;
        }
    }
    if(config->thunk && CCNL_NOW() > config->endtime){
        DEBUGMSG(49, "NFN: Exit computation: timeout when resolving thunk\n");
        DBL_LINKED_LIST_REMOVE(configuration_list, config);
        //Reply error!
        //config->thunk = 0;
        return;
    }
    ccnl_nfn(ccnl, NULL, NULL, NULL, config, NULL, 0, 0);
}

void
ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                                  struct configuration_s *config, int suite){
    DEBUGMSG(49, "ccnl_nfn_nack_local_computation\n");
    struct ccnl_interest_s *interest = NULL;
    ccnl_nfn(ccnl, orig, prefix, from, config, interest, suite, 1);
}

int
ccnl_nfn_thunk_already_computing(struct ccnl_prefix_s *prefix)
{
    DEBUGMSG(49, "ccnl_nfn_thunk_already_computing()\n");
    int i = 0;
    for(i = 0; i < -configid; ++i){
        struct ccnl_prefix_s *copy;
        struct configuration_s *config = find_configuration(configuration_list, -i);
        if(!config) continue;
        ccnl_nfn_copy_prefix(config->prefix ,&copy);
        ccnl_nfn_remove_thunk_from_prefix(copy);
        if(!ccnl_prefix_cmp(copy, NULL, prefix, CMP_EXACT)){
             return 1;
        }     
    }
    return 0;
}

struct ccnl_prefix_s *
ccnl_nfn_create_new_prefix(struct ccnl_prefix_s *p){
    struct ccnl_prefix_s *p2 = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    p2->compcnt = p->compcnt;
    p2->complen = ccnl_malloc(sizeof(int) * p->compcnt);
    p2->comp = ccnl_malloc(sizeof(char *) * p->compcnt);
    int it1, it2;
    for(it1 = 0; it1 < p->compcnt; ++it1){
        int len = 0;
        p2->complen[it1] = p->complen[it1];
        p2->comp[it1] = ccnl_malloc(p->complen[it1] + 1);
        for(it2 = 0; it2 < p->complen[it1]; ++it2){
            len += sprintf(p2->comp[it1]+it2, "%c", p->comp[it1][it2]);
        }
        sprintf(p2->comp[it1]+ len, "\0");
    }

    return p2;
}

int 
ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, 
        struct configuration_s *config, struct ccnl_interest_s *interest,
         int suite, int start_locally)
{
    DEBUGMSG(49, "ccnl_nfn(%p, %p, %p, %p, %p)\n",
             (void*)ccnl, (void*)orig, (void*)prefix, (void*)from, (void*)config);

    if(suite == CCNL_SUITE_NDNTLV){
        prefix = ccnl_nfn_create_new_prefix(prefix);
    }
    DEBUGMSG(99, "Namecomps: %s \n", ccnl_prefix_to_path2(prefix));
    int thunk_request = 0;
    if(config){
        suite = config->suite;
        thunk_request = config->fox_state->thunk_request;
        goto restart; //do not do parsing thinks again
    }

    from->flags = CCNL_FACE_FLAGS_STATIC;



    if(ccnl_nfn_thunk_already_computing(prefix)){
        DEBUGMSG(9, "Computation for this interest is already running\n");
        return -1;
    }
    unsigned char *res = NULL;
    struct ccnl_prefix_s *original_prefix;
    ccnl_nfn_copy_prefix(prefix, &original_prefix);


    int num_of_required_thunks = 0;
   
    if(!memcmp(prefix->comp[prefix->compcnt-2], "THUNK", 5))
    {
        thunk_request = 1;
    }
    if(interest && interest->prefix->compcnt > 2 + thunk_request){ // forward interests with outsourced components
        struct ccnl_prefix_s *prefix_name;
        ccnl_nfn_copy_prefix(prefix, &prefix_name);
        prefix_name->compcnt -= (2 + thunk_request);
        if(ccnl_nfn_local_content_search(ccnl, NULL, prefix_name) == NULL){
            ccnl_interest_propagate(ccnl, interest);
            return 0;
        }
        else{
            start_locally = 1;
        }
    }

    char str[CCNL_MAX_PACKET_SIZE];
    int i, len = 0;
        
    
    //put packet together
    len = sprintf(str, "%s", prefix->comp[prefix->compcnt-2-thunk_request]);
    if(prefix->compcnt > 2 + thunk_request){
        len += sprintf(str + len, " ");
    }
    for(i = 0; i < prefix->compcnt-2-thunk_request; ++i){
        len += sprintf(str + len, "/%s", prefix->comp[i]);
    }
    
    DEBUGMSG(99, "%s\n", str);
    //search for result here... if found return...
    if(thunk_request){
        num_of_required_thunks = ccnl_nfn_count_required_thunks(str);
    }
    
    ++numOfRunningComputations;
restart:
    res = Krivine_reduction(ccnl, str, thunk_request, start_locally,
            num_of_required_thunks, &config, original_prefix, suite);
    
    //stores result if computed      
    if(res){
        
        DEBUGMSG(2,"Computation finshed: %s, running computations: %d\n", res, numOfRunningComputations);
        if(config && config->fox_state->thunk_request){      
            ccnl_nfn_remove_thunk_from_prefix(config->prefix);
        }
        struct ccnl_content_s *c = create_content_object(ccnl, config->prefix, res,
                                                         strlen((char *)res), config->suite);
        c->flags = CCNL_CONTENT_FLAGS_STATIC;

        set_propagate_of_interests_to_1(ccnl, c->name);
        ccnl_content_serve_pending(ccnl,c);
        ccnl_content_add2cache(ccnl, c);
        --numOfRunningComputations;

        DBL_LINKED_LIST_REMOVE(configuration_list, config);
    }
#ifdef CCNL_NACK
    else if(config->local_done){
        struct ccnl_content_s *nack = create_content_object(ccnl, config->prefix, ":NACK", 5, config->suite);
        ccnl_content_serve_pending(ccnl, nack);

    }
#endif
    
    //TODO: check if really necessary
    /*if(thunk_request)
    {
       ccnl_nfn_delete_prefix(prefix);
    }*/
    return 0;
}

#endif //CCNL_EXT_NFN_H
