/*
 * @f ccnl-ext-nfn.c
 * @b CCN-lite, NFN related routines
 *
 * Copyright (C) 2014, Christopher Scherb, University of Basel
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

#ifdef USE_NFN

#include "ccnl-core.h"
#include "ccnl-ext-nfn.h"

#include "ccnl-ext-nfncommon.c"
#include "ccnl-ext-nfnkrivine.c"


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
ccnl_nfn_remove_thunk_from_prefix(struct ccnl_prefix_s *prefix)
{
    DEBUGMSG(49, "ccnl_nfn_remove_thunk_from_prefix()\n");
    if (!prefix || prefix->compcnt < 2)
	return;
    prefix->comp[prefix->compcnt-2] =  prefix->comp[prefix->compcnt-1];
    prefix->complen[prefix->compcnt-2] =  prefix->complen[prefix->compcnt-1];
    --prefix->compcnt;
}

void 
ccnl_nfn_continue_computation(struct ccnl_relay_s *ccnl, int configid, int continue_from_remove){
    DEBUGMSG(49, "ccnl_nfn_continue_computation()\n");
    struct configuration_s *config = find_configuration(ccnl->km->configuration_list, -configid);
    
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
        DBL_LINKED_LIST_REMOVE(ccnl->km->configuration_list, config);
        //Reply error!
        //config->thunk = 0;
        return;
    }
    ccnl_nfn(ccnl, NULL, NULL, NULL, config, NULL, 0, 0);
}

void
ccnl_nfn_nack_local_computation(struct ccnl_relay_s *ccnl,
				struct ccnl_buf_s *orig,
                                struct ccnl_prefix_s *prefix,
				struct ccnl_face_s *from,
                                int suite)
{
    DEBUGMSG(49, "ccnl_nfn_nack_local_computation\n");

    ccnl_nfn(ccnl, orig, prefix, from, NULL, NULL, suite, 1);
}

int
ccnl_nfn_thunk_already_computing(struct ccnl_relay_s *ccnl,
				 struct ccnl_prefix_s *prefix)
{
    int i = 0;

    DEBUGMSG(49, "ccnl_nfn_thunk_already_computing()\n");
    for (i = 0; i < -ccnl->km->configid; ++i) {
        struct ccnl_prefix_s *copy;
        struct configuration_s *config;

	config = find_configuration(ccnl->km->configuration_list, -i);
        if (!config)
	    continue;
	copy = ccnl_prefix_dup(config->prefix);
        ccnl_nfn_remove_thunk_from_prefix(copy);
        if(!ccnl_prefix_cmp(copy, NULL, prefix, CMP_EXACT)){
             return 1;
        }     
    }
    return 0;
}

int 
ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	 struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, 
	 struct configuration_s *config, struct ccnl_interest_s *interest,
         int suite, int start_locally)
{
    int num_of_required_thunks = 0;
    int thunk_request = 0;
    struct ccnl_prefix_s *original_prefix = NULL;

    DEBUGMSG(49, "ccnl_nfn(%p, %p, %s, %p, %p)\n",
             (void*)ccnl, (void*)orig, ccnl_prefix_to_path(prefix),
	     (void*)from, (void*)config);

    prefix = ccnl_prefix_dup(prefix);

/*
    if (suite == CCNL_SUITE_CCNTLV){
        prefix = ccnl_nfn_create_new_prefix(prefix);
    }
    if (suite == CCNL_SUITE_NDNTLV){
        prefix = ccnl_nfn_create_new_prefix(prefix);
    }
*/
    DEBUGMSG(99, "Namecomps: %s \n", ccnl_prefix_to_path(prefix));
    if(config){
        suite = config->suite;
        thunk_request = config->fox_state->thunk_request;
        goto restart; //do not do parsing thinks again
    }

    from->flags = CCNL_FACE_FLAGS_STATIC;

    if (ccnl_nfn_thunk_already_computing(ccnl, prefix)) {
        DEBUGMSG(9, "Computation for this interest is already running\n");
        return -1;
    }
    unsigned char *res = NULL;
    original_prefix = ccnl_prefix_dup(prefix);
   
    if (ccnl_isTHUNK(prefix))
        thunk_request = 1;

    if(interest && interest->prefix->compcnt > 2 + thunk_request){ // forward interests with outsourced components
	struct ccnl_prefix_s *prefix_name = ccnl_prefix_dup(prefix);
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
    if (prefix->suite == CCNL_SUITE_CCNTLV) {
        len = prefix->complen[prefix->compcnt-2-thunk_request] - 4;
        memcpy(str, prefix->comp[prefix->compcnt-2-thunk_request] + 4, len);
        str[len] = '\0';
    } else {
        len = prefix->complen[prefix->compcnt-2-thunk_request];
        memcpy(str, prefix->comp[prefix->compcnt-2-thunk_request], len);
        str[len] = '\0';
    }
    if(prefix->compcnt > 2 + thunk_request){
        len += sprintf(str + len, " ");
    }

    for (i = 0; i < prefix->compcnt-2-thunk_request; ++i) {
        len += sprintf(str + len, "/%.*s", prefix->complen[i], prefix->comp[i]);
    }
    DEBUGMSG(99, "expr is <%s>\n", str);
    //search for result here... if found return...
    if(thunk_request){
        num_of_required_thunks = ccnl_nfn_count_required_thunks(str);
    }
    
    ++ccnl->km->numOfRunningComputations;
restart:
    res = Krivine_reduction(ccnl, str, thunk_request, start_locally,
            num_of_required_thunks, &config, original_prefix, suite);
    
    //stores result if computed      
    if (res) {
	struct ccnl_prefix_s *copy;
        DEBUGMSG(2,"Computation finished: %s, running computations: %d\n", res, ccnl->km->numOfRunningComputations);
        if (config && config->fox_state->thunk_request) {
            ccnl_nfn_remove_thunk_from_prefix(config->prefix);
        }
	copy = ccnl_prefix_dup(config->prefix);
        struct ccnl_content_s *c = ccnl_nfn_newresult(ccnl, &copy,
				res, strlen((char *)res));
        c->flags = CCNL_CONTENT_FLAGS_STATIC;

        set_propagate_of_interests_to_1(ccnl, c->name);
        ccnl_content_serve_pending(ccnl,c);
        ccnl_content_add2cache(ccnl, c);
        --ccnl->km->numOfRunningComputations;

        DBL_LINKED_LIST_REMOVE(ccnl->km->configuration_list, config);
    }
#ifdef USE_NACK
    else if(config->local_done){
        struct ccnl_content_s *nack = create_content_object(ccnl, config->prefix, (unsigned char*)":NACK", 5, config->suite);
        ccnl_content_serve_pending(ccnl, nack);

    }
#endif
    
    //TODO: check if really necessary
    /*if (thunk_request) {
       free_prefix(prefix);
    }*/
    return 0;
}

int
ccnl_isNACK(struct ccnl_content_s *c)
{
    return !memcmp(c->content, ":NACK", 5);
}

int
ccnl_isNFNrequest(struct ccnl_prefix_s *p)
{
    int rc = 0;

    if (!p || p->compcnt < 2)
	return 0;
    switch (p->suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
	rc = p->complen[p->compcnt-1] > 6 &&
	    !memcmp(p->comp[p->compcnt-1], "\x00\x01\x00\x03NFN", 7);
	break;
#endif
    default:
	rc = p->complen[p->compcnt-1] == 3 &&
	    !memcmp(p->comp[p->compcnt-1], "NFN", 3);
	break;
    }

    return rc;
}

int
ccnl_isTHUNK(struct ccnl_prefix_s *p)
{
    int rc = 0;

    if (!p || p->compcnt < 3)
	return 0;
    switch (p->suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
	rc = p->complen[p->compcnt-2] > 8 &&
	    !memcmp(p->comp[p->compcnt-2], "\x00\x01\x00\x05THUNK", 9);
	break;
#endif
    default:
	rc = p->complen[p->compcnt-1] == 5 &&
	    !memcmp(p->comp[p->compcnt-1], "THUNK", 5);
	break;
    }

    return rc;
}

struct ccnl_interest_s*
ccnl_nfn_request(struct ccnl_relay_s *ccnl, struct ccnl_face_s *from,
		 int suite, struct ccnl_buf_s *buf, struct ccnl_prefix_s *p,
		 int minsfx, int maxsfx)
{
    struct ccnl_interest_s *i;
    struct ccnl_buf_s *buf2 = buf;
    struct ccnl_prefix_s *p2 = p;

    if (ccnl->km->numOfRunningComputations >= NFN_MAX_RUNNING_COMPUTATIONS)
	return 0;

    i = ccnl_interest_new(ccnl, from, p->suite, &buf, &p, minsfx, maxsfx);
    i->corePropagates = 0; //do not forward interests for running computations
    ccnl_interest_append_pending(i, from);
    if (!i->corePropagates)
	ccnl_nfn(ccnl, buf2, p2, from, NULL, i, suite, 0);

    return i;
}

#endif //USE_NFN


// eof
