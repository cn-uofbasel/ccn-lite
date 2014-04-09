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

#include "ccnl-core.h"


#include "krivine.c"
#include "ccnl-includes.h"

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
    return num;
}



void * 
ccnl_nfn_thread(void *arg)
{
    struct thread_parameter_s *t = ((struct thread_parameter_s*)arg);
    
    struct ccnl_relay_s *ccnl = t->ccnl;
    struct ccnl_buf_s *orig = t->orig;
    struct ccnl_prefix_s *prefix = t->prefix;
    struct ccnl_face_s *from = t->from;
    struct thread_s *thread = t->thread;
    
    int thunk_request = 0;
    int num_of_thunks = 0;
    struct ccnl_prefix_s *original_prefix;
    DEBUGMSG(49, "ccnl_nfn(%p, %p, %p, %p)\n", ccnl, orig, prefix, from);
    DEBUGMSG(99, "NFN-engine\n"); 
    if(!memcmp(prefix->comp[prefix->compcnt-2], "THUNK", 5))
    {
        
        ccnl_nfn_copy_prefix(prefix, &original_prefix);
        thunk_request = 1;
        //FIXME: WARUM COPY VON PREFIX NÖTIG?
      
        DEBUGMSG(99, "  Thunk-request, currently not implementd\n"); 
        //TODO: search for num of required thunks???
    }
    else{
        original_prefix = prefix;
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
        num_of_thunks = ccnl_nfn_count_required_thunks(str);
    }
    char *res = Krivine_reduction(ccnl, str, thunk_request, &num_of_thunks,
        original_prefix, thread);
    //stores result if computed      
    if(res){
        DEBUGMSG(2,"Computation finshed: %s\n", res);
        if(thunk_request){      
            original_prefix->comp[original_prefix->compcnt-2] =  original_prefix->comp[original_prefix->compcnt-1];
            original_prefix->complen[original_prefix->compcnt-2] =  original_prefix->complen[original_prefix->compcnt-1];
            --original_prefix->compcnt;
        }
        struct ccnl_content_s *c = add_computation_to_cache(ccnl, original_prefix, res, strlen(res));
            
        c->flags = CCNL_CONTENT_FLAGS_STATIC;
        DEBUGMSG(99, "DELIVER CONTENT!\n");
        if(!thunk_request)ccnl_content_serve_pending(ccnl,c);
        ccnl_content_add2cache(ccnl, c);
        
    }
    
    //TODO: check if really necessary
    /*if(thunk_request)
    {
       ccnl_nfn_delete_prefix(prefix);
    }*/
    //pthread_exit ((void *) 0);
    return 0;
}

void 
ccnl_nfn_send_signal(int threadid){
    struct thread_s *thread = threads[threadid];
    pthread_cond_broadcast(&thread->cond);
}


int 
ccnl_nfn(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    struct thread_s *thread = new_thread(threadid);
    struct thread_parameter_s *arg = malloc(sizeof(struct thread_parameter_s *));
    char *h = malloc(10);
    arg->ccnl = ccnl;
    arg->orig = orig;
    arg->prefix = prefix;
    arg->from = from;
    arg->thread = thread;
    
    int threadpos = -threadid;
    threads[threadpos] = thread;
    --threadid;
    pthread_create(&thread->thread, NULL, ccnl_nfn_thread, (void *)arg);
    return 0;
}