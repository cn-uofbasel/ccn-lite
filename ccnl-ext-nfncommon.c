/*
 * @f ccnl-ext-nfncommon.c
 * @b CCN-lite, tools for the "Krivine lambda expression resolver"
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
 * 2014-03-14 created 
 */

// ----------------------------------------------------------------------

struct ccnl_interest_s *
mkInterestObject(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                 struct ccnl_prefix_s *prefix)
{

    DEBUGMSG(2, "mkInterestObject()\n");
    int scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen, mbf=0, len, typ, num, i;
    struct ccnl_buf_s *buf = 0, *ppkd=0, *nonce=0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *out = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    unsigned char *content;


    struct ccnl_face_s * from = ccnl_calloc(1, sizeof(struct ccnl_face_s *));
    from->faceid = config->configid;
    from->last_used = CCNL_NOW();
    from->outq = ccnl_calloc(1, sizeof(struct ccnl_buf_s) + strlen((char *)prefix->comp[0]));
    from->outq->datalen = strlen((char *)prefix->comp[0]);
    memcpy((char *)(from->outq->data), (char *)prefix->comp[0], from->outq->datalen);

    char *namecomps[CCNL_MAX_NAME_COMP];
    for(i = 0; i < prefix->compcnt; ++i){
        namecomps[i] = strdup((char *)prefix->comp[i]);
    }
    namecomps[prefix->compcnt] = 0;

    if(config->suite == CCNL_SUITE_CCNB){

        len = ccnl_ccnb_mkInterest(namecomps, NULL, out, 0);
        if(ccnl_ccnb_dehead(&out, &len, &num, &typ)){
            return 0;
        }
        buf = ccnl_ccnb_extract(&out, &len, &scope, &aok, &minsfx, &maxsfx,
                                &p, &nonce, &ppkd, &content, &contlen);
        return ccnl_interest_new(ccnl, from, CCNL_SUITE_CCNB, &buf, &p, minsfx, maxsfx);
    }
    else if(config->suite == CCNL_SUITE_CCNTLV){
        //NOT YET IMPLEMETED
        return 0;
    }
    else if(config->suite == CCNL_SUITE_NDNTLV){
       int tmplen = CCNL_MAX_PACKET_SIZE;
       int nonce2;
       len = ccnl_ndntlv_mkInterest(namecomps, -1, &nonce2, &tmplen, out);
       memmove(out, out + tmplen, CCNL_MAX_PACKET_SIZE - tmplen);
       len = CCNL_MAX_PACKET_SIZE - tmplen;
       unsigned char *cp = out;
       if(ccnl_ndntlv_dehead(&out, &len, &typ, &num)){
           return 0;
       }
       buf = ccnl_ndntlv_extract(out - cp, &out, &len, &scope, &mbf, &minsfx, &maxsfx,
                                 &p, &nonce, &ppkd, &content, &contlen);
       struct ccnl_interest_s *ret = ccnl_interest_new(ccnl, from, CCNL_SUITE_NDNTLV, &buf, &p, minsfx, maxsfx);
       return ret;
    }
    return 0;
}

struct ccnl_content_s *
ccnl_nfn_newresult(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s **prefix,
		   unsigned char *resultstr, int resultlen)
{
    struct ccnl_buf_s *buf;
    int resultpos = 0;

    DEBUGMSG(49, "ccnl_nfn_newresult(prefix=%s, suite=%d, content=%s)\n",
	     ccnl_prefix_to_path(*prefix), (*prefix)->suite, resultstr);

    buf = ccnl_mkSimpleContent(*prefix, resultstr, resultlen, &resultpos);

    return ccnl_content_new(ccnl, (*prefix)->suite, &buf, prefix,
			    NULL, buf->data + resultpos, resultlen);
}

// ----------------------------------------------------------------------

struct fox_machine_state_s *
new_machine_state(int thunk_request, int num_of_required_thunks){
    struct fox_machine_state_s *ret = ccnl_calloc(1, sizeof(struct fox_machine_state_s));
    ret->thunk_request = thunk_request;
    ret->num_of_required_thunks = num_of_required_thunks;
/*
    ret->prefix_mapping = NULL;
    ret->it_routable_param = 0;
    ret->num_of_params = 0;
    ret->num_of_required_thunks = 0;
    ret->thunk = 0;
*/
    return ret;
}

struct configuration_s *
new_config(struct ccnl_relay_s *ccnl, char *prog,
	   struct environment_s *global_dict, int thunk_request,
           int start_locally, int num_of_required_thunks,
	   struct ccnl_prefix_s *prefix, int configid, int suite)
{
    struct configuration_s *ret = ccnl_calloc(1, sizeof(struct configuration_s));
    ret->prog = prog;
    ret->global_dict = global_dict;
    ret->fox_state = new_machine_state(thunk_request, num_of_required_thunks);
    ret->configid = ccnl->km->configid;
    ret->start_locally = start_locally;
    ret->prefix = prefix;
    ret->suite = suite;
    ret->thunk_time = NFN_DEFAULT_WAITING_TIME;
/*
    ret->result_stack = NULL;
    ret->argument_stack = NULL;
    ret->env = NULL;
    ret->thunk = 0;
    ret->local_done = 0;
*/
    return ret;
}

struct configuration_s*
find_configuration(struct configuration_s *config_list, int configid){
    struct configuration_s *config;
    for(config = config_list; config; config = config->next){
        if(config->configid == configid){
            return config;
        }
    }
    return NULL;
}

int trim(char *str){  // inplace, returns len after shrinking
    int i;
    while(str[0] != '\0' && str[0] == ' '){
        for(i = 0; i < strlen(str); ++i)
            str[i] = str[i+1];
    }
    for(i = strlen(str)-1; i >=0; ++i){
        if(str[i] == ' '){
            str[i] = '\0';
            continue;
        }
        break;
    }
    return strlen(str);
}

struct ccnl_prefix_s *
add_computation_components(struct ccnl_prefix_s *prefix,
			   int thunk_request, char *comp)
{
    /*while(namecomp[i]) ++i;
    namecomp[i++] = comp;
    if(thunk_request) namecomp[i++] = (unsigned char *)"THUNK";
    namecomp[i++] = (unsigned char *)"NFN";
    namecomp[i] = NULL;*/

    DEBUGMSG(99, "add_computation_component(suite=%d, %s)\n", prefix->suite, comp);

    int incr = thunk_request ? 3 : 2, i, len, l1, l2;
    struct ccnl_prefix_s *ret = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    unsigned char buf1[20], buf2[20];

    ret->comp = ccnl_malloc((prefix->compcnt +incr) * sizeof(char*));
    ret->complen = ccnl_malloc((prefix->compcnt +incr) * sizeof(int));
    ret->suite = prefix->suite;

    for (i = 0, len = 0; i < prefix->compcnt; i++)
	len += prefix->complen[i];
    len += 4 + strlen(comp);
    if (thunk_request)
	len += l1 = ccnl_pkt_mkComponent(prefix->suite, buf1, "THUNK");
    len += l2 = ccnl_pkt_mkComponent(prefix->suite, buf2, "NFN");
    ret->path = ccnl_malloc(len);
    for (i = 0, len = 0; i < prefix->compcnt; i++) {
	ret->comp[i] = ret->path + len;
	memcpy(ret->comp[i], prefix->comp[i], prefix->complen[i]);
	len += prefix->complen[i];
    }
    ret->comp[i] = ret->path + len;
    len += ccnl_pkt_mkComponent(prefix->suite, ret->comp[i], comp);
    i++;
    if (thunk_request) {
	ret->comp[i] = ret->path + len;
	memcpy(ret->comp[i], buf1, l1);
	len += l1;
	i++;
    }
    ret->comp[i] = ret->path + len;
    memcpy(ret->comp[i], buf2, l2);
    i++;
    ret->compcnt = i;
    free_prefix(prefix);

    return ret;
}

struct ccnl_prefix_s *
add_local_computation_components(struct configuration_s *config){

    int i = 0;
    char *comp = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    struct ccnl_prefix_s *ret;
    int complen = sprintf(comp, "call %d", config->fox_state->num_of_params);
    for(i = 0; i < config->fox_state->num_of_params; ++i){
        if(config->fox_state->params[i]->type == STACK_TYPE_INT)
            complen += sprintf(comp+complen, " %d", *((int*)config->fox_state->params[i]->content));
        else if(config->fox_state->params[i]->type == STACK_TYPE_PREFIX)
            complen += sprintf(comp+complen, " %s", ccnl_prefix_to_path((struct ccnl_prefix_s*)config->fox_state->params[i]->content));
#ifndef USE_UTIL
        else DEBUGMSG(1, "Invalid type %d\n", config->fox_state->params[i]->type);
#endif
    }

    i = 0;
    ret = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    int size = config->fox_state->thunk_request ? 4 : 3;

    ret->comp = ccnl_malloc(sizeof(char*) * size);
    ret->complen = ccnl_malloc(sizeof(int) * size);

    ret->comp[i] = (unsigned char *)"COMPUTE";
    ret->complen[i] = strlen("COMPUTE");
    ++i;
    ret->comp[i] = (unsigned char *)strdup(comp);
    ret->complen[i] = strlen(comp);
    ++i;

#ifdef USE_SUITE_CCNTLV
    if (config->fox_state->thunk_request) {
        ret->comp[i] = (unsigned char *) "\x00\x01\x00\x05THUNK";
        ret->complen[i] = 4 + strlen("THUNK");
        ++i;
    }
    ret->comp[i] = (unsigned char *) "\x00\x01\x00\x03NFN";
    ret->complen[i] = 4 + strlen("NFN");
#else
    if (config->fox_state->thunk_request) {
	ret->comp[i] = (unsigned char *)"THUNK";
	ret->complen[i] = strlen("THUNK");
        ++i;
    }
    ret->comp[i] = (unsigned char *)"NFN";
    ret->complen[i] = strlen("NFN");
#endif

    ret->compcnt = i+1;

    return ret;
}

int
createComputationString(struct configuration_s *config, int parameter_num, char *comp){

    int i;
    int complen = sprintf((char*)comp, "(@x call %d", config->fox_state->num_of_params);
    for(i = 0; i < config->fox_state->num_of_params; ++i){
        if(parameter_num == i){
            complen += sprintf(comp + complen, " x");
        }
        else{
            //complen += sprintf((char*)comp + complen, "%s ", config->fox_state->params[j]);

            if(config->fox_state->params[i]->type == STACK_TYPE_INT)
                complen += sprintf(comp+complen, " %d", *((int*)config->fox_state->params[i]->content));
            else if(config->fox_state->params[i]->type == STACK_TYPE_PREFIX)
                complen += sprintf(comp+complen, " %s", ccnl_prefix_to_path((struct ccnl_prefix_s*)config->fox_state->params[i]->content));
#ifndef USE_UTIL
            else DEBUGMSG(1, "Invalid type in createComputationString() %d\n", config->fox_state->params[i]->type);
#endif
        }
    }
    complen += sprintf(comp + complen, ")");
    return complen;
}

#ifndef USE_UTIL
void
set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref){
    struct ccnl_interest_s *interest = NULL;
    for(interest = ccnl->pit; interest; interest = interest->next){
        if(!ccnl_prefix_cmp(interest->prefix, 0, pref, CMP_EXACT)){
            interest->corePropagates = 1;
        }
    }
}

struct ccnl_prefix_s *
create_prefix_for_content_on_result_stack(struct ccnl_relay_s *ccnl, struct configuration_s *config){
    struct ccnl_prefix_s *name = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    name->comp = ccnl_malloc(2*sizeof(char*));
    name->complen = ccnl_malloc(2*sizeof(int));
    name->compcnt = 1;
    name->comp[1] = (unsigned char *)"NFN";
    name->complen[1] = 3;
    name->comp[0] = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    memset(name->comp[0], 0, CCNL_MAX_PACKET_SIZE);
    name->path = NULL;

    int it, len = 0;
    len += sprintf((char *)name->comp[0]+len, "call %d", config->fox_state->num_of_params);
    for(it = 0; it < config->fox_state->num_of_params; ++it){

        struct stack_s *stack = config->fox_state->params[it];
        if(stack->type == STACK_TYPE_PREFIX){
            char *pref_str = ccnl_prefix_to_path((struct ccnl_prefix_s*)stack->content);
            len += sprintf((char*)name->comp[0]+len, " %s", pref_str);
        }
        else if(stack->type == STACK_TYPE_INT){
            len += sprintf((char*)name->comp[0]+len, " %d", *(int*)stack->content);
        }
        else{
            DEBUGMSG(1, "Invalid stack type\n");
            return NULL;
        }

    }
    name->complen[0] = len;
    return name;
}


struct ccnl_content_s *
ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl, struct configuration_s *config, struct ccnl_prefix_s *prefix){
    DEBUGMSG(2, "ccnl_nfn_local_content_search()\n");

    struct ccnl_content_s *content;
    DEBUGMSG(99, "Searching local for content %s\n", ccnl_prefix_to_path(prefix));
    for(content = ccnl->contents; content; content = content->next){
        if(!ccnl_prefix_cmp(prefix, 0, content->name, CMP_EXACT)){
            return content;
        }
    }
    struct prefix_mapping_s *iter;
    if(!config || !config->fox_state || !config->fox_state->prefix_mapping){
        return NULL;
    }
    for(iter = config->fox_state->prefix_mapping; iter; iter = iter->next){
        if(!ccnl_prefix_cmp(prefix, 0, iter->key, CMP_EXACT)){
            for(content = ccnl->contents; content; content = content->next){
                if(!ccnl_prefix_cmp(iter->value, 0, content->name, CMP_EXACT)){
                    return content;
                }
            }
        }
    }

    return NULL;
}

char * 
ccnl_nfn_add_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config,
		   struct ccnl_prefix_s *prefix)
{
    struct ccnl_prefix_s *new_prefix = ccnl_prefix_dup(prefix);
    struct thunk_s *thunk;

    DEBUGMSG(2, "ccnl_nfn_add_thunk()\n");
    
    if (ccnl_isTHUNK(new_prefix)) {
        new_prefix->comp[new_prefix->compcnt-2] = new_prefix->comp[new_prefix->compcnt-1];
        --new_prefix->compcnt;
    }

    thunk = ccnl_calloc(1, sizeof(struct thunk_s));
    thunk->prefix = new_prefix;
    thunk->reduced_prefix = create_prefix_for_content_on_result_stack(ccnl, config);
    sprintf(thunk->thunkid, "THUNK%d", ccnl->km->thunkid++);
    DBL_LINKED_LIST_ADD(ccnl->km->thunk_list, thunk);
    DEBUGMSG(99, "Created new thunk with id: %s\n", thunk->thunkid);
    return strdup(thunk->thunkid);
}

struct thunk_s *
ccnl_nfn_get_thunk(struct ccnl_relay_s *ccnl, unsigned char *thunkid){
    struct thunk_s *thunk;
    for(thunk = ccnl->km->thunk_list; thunk; thunk = thunk->next){
	if(!strcmp(thunk->thunkid, (char*)thunkid)){
            DEBUGMSG(49, "Thunk table entry found\n");
            return thunk;
        }
    }
    return NULL;
}

struct ccnl_interest_s *
ccnl_nfn_get_interest_for_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, unsigned char *thunkid){
    DEBUGMSG(2, "ccnl_nfn_get_interest_for_thunk()\n");
    struct thunk_s *thunk = ccnl_nfn_get_thunk(ccnl, thunkid);
    if(thunk){
        char *out = ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
        memset(out, 0, CCNL_MAX_PACKET_SIZE);
        struct ccnl_interest_s *interest = mkInterestObject(ccnl, config, thunk->prefix);
        return interest;
    }
    return NULL;
}

void
ccnl_nfn_remove_thunk(struct ccnl_relay_s *ccnl, char* thunkid){
    DEBUGMSG(2, "ccnl_nfn_remove_thunk()\n");
    struct thunk_s *thunk;
    for(thunk = ccnl->km->thunk_list; thunk; thunk = thunk->next){
        if(!strncmp(thunk->thunkid, thunkid, sizeof(thunk->thunkid))){
            break;
        }
    }
    DBL_LINKED_LIST_REMOVE(ccnl->km->thunk_list, thunk);
}

int 
ccnl_nfn_reply_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config)
{
    struct ccnl_prefix_s *prefix = ccnl_prefix_dup(config->prefix);
    char reply_content[100];
    int thunk_time = (int)config->thunk_time; 
    struct ccnl_content_s *c;

    DEBUGMSG(2, "ccnl_nfn_reply_thunk()\n");

    memset(reply_content, 0, 100);
    sprintf(reply_content, "%d", thunk_time);
    c = ccnl_nfn_newresult(ccnl, &prefix, (unsigned char*)reply_content,
			   strlen(reply_content));
    set_propagate_of_interests_to_1(ccnl, c->name);
    ccnl_content_add2cache(ccnl, c);
    ccnl_content_serve_pending(ccnl, c);
    return 0;
}

struct ccnl_content_s *
ccnl_nfn_resolve_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, unsigned char *thunk){
    DEBUGMSG(2, "ccnl_nfn_resolve_thunk()\n");
    struct ccnl_interest_s *interest = ccnl_nfn_get_interest_for_thunk(ccnl, config, thunk);

    if(interest){
        interest->last_used = CCNL_NOW();
        struct ccnl_content_s *c = NULL;
        if((c = ccnl_nfn_local_content_search(ccnl, config, interest->prefix)) != NULL){
            interest = ccnl_interest_remove(ccnl, interest);
            return c;
        }
        ccnl_interest_propagate(ccnl, interest);
    }
    return NULL;
}
#endif //USE_UTIL


// eof
