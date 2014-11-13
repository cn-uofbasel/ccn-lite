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

struct const_s *
ccnl_nfn_krivine_str2const(char *str){
    int len = 0;
    char *ccp = str;
    struct const_s *c;
    if(*ccp != '\''){
        return 0;
    }
    ++ccp;
    while(*ccp != '\''){
        ++ccp;
    }
    len = (ccp - str);
    c = ccnl_calloc(1, sizeof(*c) + len);
    c->len = len-1;
    strncpy(c->str, str+1, len-1);

    return c;
}

char *
ccnl_nfn_krivine_const2str(struct const_s * con){ //may be unsafe
    char *c = ccnl_calloc(con->len+3, sizeof(*c));
    strncpy(c+1, con->str, con->len);
    c[0] = '\'';
    c[con->len+1] = '\'';
    return c;
}

// ----------------------------------------------------------------------

struct ccnl_interest_s *
ccnl_nfn_query2interest(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        struct configuration_s *config)
{
    struct ccnl_buf_s *buf;

    DEBUGMSG(2, "ccnl_nfn_query2interest()\n");

    struct ccnl_face_s * from = ccnl_malloc(sizeof(struct ccnl_face_s));
    from->faceid = config->configid;
    from->last_used = CCNL_NOW();
    from->outq = NULL;
    DEBUGMSG(99, "  Configuration ID: %d\n", config->configid);

    buf = ccnl_mkSimpleInterest(*prefix, NULL);

    return ccnl_interest_new(ccnl, from, (*prefix)->suite, &buf, prefix, 0, 0);
//    return ccnl_interest_new(ccnl, FROM, (*prefix)->suite, &buf, prefix, 0, 0);
}

struct ccnl_content_s *
ccnl_nfn_result2content(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        unsigned char *resultstr, int resultlen)
{
    struct ccnl_buf_s *buf;
    int resultpos = 0;

    DEBUGMSG(49, "ccnl_nfn_result2content(prefix=%s, suite=%s, contlen=%d)\n",
             ccnl_prefix_to_path(*prefix), ccnl_suite2str((*prefix)->suite),
             resultlen);

    buf = ccnl_mkSimpleContent(*prefix, resultstr, resultlen, &resultpos);
    if (!buf)
        return NULL;

    return ccnl_content_new(ccnl, (*prefix)->suite, &buf, prefix,
                            NULL, buf->data + resultpos, resultlen);
}

// ----------------------------------------------------------------------

struct fox_machine_state_s *
new_machine_state(int thunk_request, int num_of_required_thunks)
{
    struct fox_machine_state_s *ret;

    ret = ccnl_calloc(1, sizeof(struct fox_machine_state_s));
    if (ret) {
        ret->thunk_request = thunk_request;
        ret->num_of_required_thunks = num_of_required_thunks;
    }
    return ret;
}

struct configuration_s *
new_config(struct ccnl_relay_s *ccnl, char *prog,
           struct environment_s *global_dict, int thunk_request,
           int start_locally, int num_of_required_thunks,
           struct ccnl_prefix_s *prefix, int configid, int suite)
{
    struct configuration_s *ret;

    ret = ccnl_calloc(1, sizeof(struct configuration_s));
    ret->prog = prog;
    ret->global_dict = global_dict;
    ret->fox_state = new_machine_state(thunk_request, num_of_required_thunks);
    ret->configid = ccnl->km->configid;
    ret->start_locally = start_locally;
    ret->prefix = prefix;
    ret->suite = suite;
    ret->thunk_time = NFN_DEFAULT_WAITING_TIME;

    return ret;
}

void ccnl_nfn_freeClosure(struct closure_s *c);
void ccnl_nfn_releaseEnvironment(struct environment_s **env);

void
ccnl_nfn_freeEnvironment(struct environment_s *env)
{
    while (env && env->refcount <= 1) {
        struct environment_s *next = env->next;
        ccnl_free(env->name);
        ccnl_nfn_freeClosure(env->closure);
        ccnl_free(env);
        env = next;
    }
}

void
ccnl_nfn_reserveEnvironment(struct environment_s *env)
{
    if (!env)
        return;
    env->refcount++;
}

void
ccnl_nfn_releaseEnvironment(struct environment_s **env)
{
    if (!env | !*env)
        return;
    (*env)->refcount--;
    if ((*env)->refcount <= 0) {
        ccnl_nfn_freeEnvironment(*env);
        *env = NULL;
    }
}

void
ccnl_nfn_freeClosure(struct closure_s *c)
{
    if (!c)
        return;
    ccnl_free(c->term);
    ccnl_nfn_releaseEnvironment(&c->env);
    ccnl_free(c);
}

void
ccnl_nfn_freeStack(struct stack_s* s)
{
    while (s) {
        struct stack_s *next = s->next;
        struct thunk_s *t;

        switch (s->type) {
        case STACK_TYPE_CLOSURE:
            ccnl_nfn_freeClosure((struct closure_s *)s->content);
            break;
        case STACK_TYPE_PREFIX:
            free_prefix(((struct ccnl_prefix_s *)s->content));
            break;
        case STACK_TYPE_THUNK:
            t = (struct thunk_s *)s->content;
            free_prefix(t->prefix);
            free_prefix(t->reduced_prefix);
            break;
        case STACK_TYPE_INT:
        default:
            ccnl_free(s->content);
            break;
        }
        ccnl_free(s);
        s = next;
    }
}

void
ccnl_nfn_freeMachineState(struct fox_machine_state_s* f)
{
    if (!f)
        return;
    ccnl_free(f->thunk);
    while (f->prefix_mapping) {
        struct prefix_mapping_s *m = f->prefix_mapping;
        DBL_LINKED_LIST_REMOVE(f->prefix_mapping, m);
        free_prefix(m->key);
        free_prefix(m->value);
    }
    ccnl_free(f);
}

void
ccnl_nfn_freeConfiguration(struct configuration_s* c)
{
    DEBUGMSG(99, "ccnl_nfn_freeConfiguration(%p)\n", (void*)c);

    if (!c)
        return;
    ccnl_free(c->prog);
    ccnl_nfn_freeStack(c->result_stack);
    ccnl_nfn_freeStack(c->argument_stack);
    ccnl_nfn_releaseEnvironment(&c->env);
    ccnl_nfn_releaseEnvironment(&c->global_dict);
    ccnl_nfn_freeMachineState(c->fox_state);
    free_prefix(c->prefix);
    ccnl_free(c);
}

void
ccnl_nfn_freeKrivine(struct ccnl_relay_s *ccnl)
{
    DEBUGMSG(99, "ccnl_nfn_freeKrivine(%p)\n", ccnl ? (void*)ccnl->km : NULL);

    if (!ccnl || !ccnl->km)
        return;

    DEBUGMSG(99, "  configuration_list %p\n",
             (void*)ccnl->km->configuration_list);
    // free thunk_list;
    while (ccnl->km->configuration_list) {
        struct configuration_s *c = ccnl->km->configuration_list;
        DBL_LINKED_LIST_REMOVE(ccnl->km->configuration_list, c);
        ccnl_nfn_freeConfiguration(c);
    }
    ccnl_free(ccnl->km);
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

// ----------------------------------------------------------------------

struct ccnl_prefix_s *
add_computation_components(struct ccnl_prefix_s *prefix,
                           int thunk_request, char *comp)
{
    int incr = thunk_request ? 3 : 2, i, len, l1, l2;
    struct ccnl_prefix_s *ret;
    unsigned char buf1[20], buf2[20];

    DEBUGMSG(99, "add_computation_component(suite=%s, %s)\n",
             ccnl_suite2str(prefix->suite), comp);

    ret = ccnl_prefix_new(prefix->suite, prefix->compcnt + incr);
    if (!ret)
        return NULL;

    for (i = 0, len = 0; i < prefix->compcnt; i++)
        len += prefix->complen[i];
    len += 4 + strlen(comp); // add 4 to be on the safe side (CCNTLV)
    if (thunk_request)
        len += l1 = ccnl_pkt_mkComponent(prefix->suite, buf1, "THUNK");
    len += l2 = ccnl_pkt_mkComponent(prefix->suite, buf2, "NFN");
    ret->bytes = ccnl_malloc(len);
    for (i = 0, len = 0; i < prefix->compcnt; i++) {
        ret->comp[i] = ret->bytes + len;
        ret->complen[i] = prefix->complen[i];
        memcpy(ret->comp[i], prefix->comp[i], prefix->complen[i]);
        len += prefix->complen[i];
    }
    ret->comp[i] = ret->bytes + len;
    ret->complen[i] = ccnl_pkt_mkComponent(prefix->suite, ret->comp[i], comp);
    len += ret->complen[i];
    i++;
    if (thunk_request) {
        ret->comp[i] = ret->bytes + len;
        ret->complen[i] = l1;
        memcpy(ret->comp[i], buf1, l1);
        len += l1;
        i++;
    }
    ret->comp[i] = ret->bytes + len;
    ret->complen[i] = l2;
    memcpy(ret->comp[i], buf2, l2);

    free_prefix(prefix);
    return ret;
}

struct ccnl_prefix_s *
add_local_computation_components(struct configuration_s *config){

    int i = 0;
    char *comp = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    struct ccnl_prefix_s *ret;
    int complen = sprintf(comp, "call %d", config->fox_state->num_of_params);
    DEBUGMSG(99, "add_local_computation_components\n");
    for(i = 0; i < config->fox_state->num_of_params; ++i){
        if(config->fox_state->params[i]->type == STACK_TYPE_INT)
            complen += sprintf(comp+complen, " %d", *((int*)config->fox_state->params[i]->content));
        else if(config->fox_state->params[i]->type == STACK_TYPE_PREFIX)
            complen += sprintf(comp+complen, " %s", ccnl_prefix_to_path((struct ccnl_prefix_s*)config->fox_state->params[i]->content));
#ifndef USE_UTIL
        else DEBUGMSG(1, "Invalid type %d\n", config->fox_state->params[i]->type);
#endif
    }

    ret = ccnl_prefix_new(0, config->fox_state->thunk_request ? 4 : 3);
    if (!ret)
        return NULL;

    i = 0;
    ret->comp[i] = (unsigned char *)"COMPUTE"; //FOR CCNTLV??? XXX
    ret->complen[i] = strlen("COMPUTE");
    ++i;
    ret->comp[i] = (unsigned char *)ccnl_strdup(comp);
    ret->complen[i] = strlen(comp);
    ++i;

    if(config->suite == CCNL_SUITE_CCNTLV){
        if (config->fox_state->thunk_request) {
            ret->comp[i] = (unsigned char *) "\x00\x01\x00\x05THUNK";
            ret->complen[i] = 4 + strlen("THUNK");
            ++i;
        }
        ret->comp[i] = (unsigned char *) "\x00\x01\x00\x03NFN";
        ret->complen[i] = 4 + strlen("NFN");
    }
    else{
        if (config->fox_state->thunk_request) {
        ret->comp[i] = (unsigned char *)"THUNK";
        ret->complen[i] = strlen("THUNK");
            ++i;
        }
        ret->comp[i] = (unsigned char *)"NFN";
        ret->complen[i] = strlen("NFN");
    }

    return ret;
}


#ifndef USE_UTIL
void
set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref){
    struct ccnl_interest_s *interest = NULL;
    for(interest = ccnl->pit; interest; interest = interest->next){
        if(!ccnl_prefix_cmp(interest->prefix, 0, pref, CMP_EXACT)){
            interest->corePropagates = 1;
            /*interest->last_used = CCNL_NOW();
            interest->retries = 0;
            interest->from->last_used = CCNL_NOW();*/
        }
    }
}

struct ccnl_prefix_s *
create_prefix_for_content_on_result_stack(struct ccnl_relay_s *ccnl,
                                          struct configuration_s *config)
{
    struct ccnl_prefix_s *name;
    int it, len = 0, offset = 0;

    name = ccnl_prefix_new(config->suite, 2);
    if (!name)
        return NULL;

#ifdef USE_SUITE_CCNTLV
    if (config->suite == CCNL_SUITE_CCNTLV)
        offset = 4;
#endif
    name->bytes = ccnl_calloc(1, CCNL_MAX_PACKET_SIZE);
    name->compcnt = 1;
    len = ccnl_pkt_mkComponent(config->suite, name->bytes, "NFN");
    name->complen[1] = len;
    name->comp[0] = name->bytes + offset + len;

    len += sprintf((char *)name->bytes + offset + len, "call %d",
                   config->fox_state->num_of_params);
    for (it = 0; it < config->fox_state->num_of_params; ++it) {
        struct stack_s *stack = config->fox_state->params[it];

        if (stack->type == STACK_TYPE_PREFIX) {
            char *pref_str = ccnl_prefix_to_path(
                                      (struct ccnl_prefix_s*)stack->content);
            len += sprintf((char*)name->bytes + offset + len, " %s", pref_str);
        } else if (stack->type == STACK_TYPE_INT) {
            len += sprintf((char*)name->bytes + offset + len, " %d",
                           *(int*)stack->content);
        } else {
            DEBUGMSG(1, "Invalid stack type\n");
            return NULL;
        }

    }
#ifdef USE_SUITE_CCNTLV
    if (config->suite == CCNL_SUITE_CCNTLV) {
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, len, &offset,
                              name->bytes + name->complen[1]);
        len += 4;
    }
#endif
    name->complen[0] = len;
    return name;
}


struct ccnl_content_s *
ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl,
                              struct configuration_s *config,
                              struct ccnl_prefix_s *prefix)
{
    struct prefix_mapping_s *iter;
    struct ccnl_content_s *content;

    DEBUGMSG(2, "ccnl_nfn_local_content_search(%s, suite=%s)\n",
             ccnl_prefix_to_path(prefix), ccnl_suite2str(prefix->suite));

    DEBUGMSG(99, "Searching local for content %s\n", ccnl_prefix_to_path(prefix));
    for (content = ccnl->contents; content; content = content->next) {
        if (!ccnl_prefix_cmp(prefix, 0, content->name, CMP_EXACT)
                                            && content->suite == prefix->suite)
            return content;
    }
    if (!config || !config->fox_state || !config->fox_state->prefix_mapping)
        return NULL;

    for (iter = config->fox_state->prefix_mapping; iter; iter = iter->next) {
        if (!ccnl_prefix_cmp(prefix, 0, iter->key, CMP_EXACT)) {
            for (content = ccnl->contents; content; content = content->next) {
                if (!ccnl_prefix_cmp(iter->value, 0, content->name, CMP_EXACT)
                                            && content->suite == prefix->suite)
                    return content;
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
    
    if (ccnl_nfnprefix_isTHUNK(new_prefix)) {
        new_prefix->comp[new_prefix->compcnt-2] = new_prefix->comp[new_prefix->compcnt-1];
        --new_prefix->compcnt;
    }

    thunk = ccnl_calloc(1, sizeof(struct thunk_s));
    thunk->prefix = new_prefix;
    thunk->reduced_prefix = create_prefix_for_content_on_result_stack(ccnl, config);
    sprintf(thunk->thunkid, "THUNK%d", ccnl->km->thunkid++);
    DBL_LINKED_LIST_ADD(ccnl->km->thunk_list, thunk);
    DEBUGMSG(99, "Created new thunk with id: %s\n", thunk->thunkid);
    return ccnl_strdup(thunk->thunkid);
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
        struct ccnl_prefix_s *copy = ccnl_prefix_dup(thunk->prefix);
        //        struct ccnl_interest_s *interest = mkInterestObject(ccnl, config, thunk->prefix);
        struct ccnl_interest_s *interest = ccnl_nfn_query2interest(ccnl, &copy, config);
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
    c = ccnl_nfn_result2content(ccnl, &prefix, (unsigned char*)reply_content,
                                strlen(reply_content));
    if (c) {
        set_propagate_of_interests_to_1(ccnl, c->name);
        ccnl_content_add2cache(ccnl, c);
        ccnl_content_serve_pending(ccnl, c);
    }
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


#ifdef USE_NACK
void
ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                struct ccnl_face_s *from, int suite)
{
    struct ccnl_content_s *nack;
    DEBUGMSG(99, "ccnl_nack_reply()\n");
    if (from->faceid <= 0) {
        return;
    }
    nack = ccnl_nfn_result2content(ccnl, &prefix,
                                    (unsigned char*)":NACK", 5);
    ccnl_nfn_monitor(ccnl, from, nack->name, nack->content, nack->contentlen);
fprintf(stderr, "+++ nack->pkt is %p\n", (void*) nack->pkt);
    ccnl_face_enqueue(ccnl, from, nack->pkt);
}
#endif // USE_NACK

struct ccnl_interest_s*
ccnl_nfn_interest_remove(struct ccnl_relay_s *relay, struct ccnl_interest_s *i)
{
    int faceid = 0;

    if (i && i->from)
        faceid = i->from->faceid;
    DEBUGMSG(99, "ccnl_nfn_interest_remove %d\n", faceid);

#ifdef USE_NACK
    if (faceid >= 0)
        ccnl_nack_reply(relay, i->prefix, i->from, i->suite);
#endif

    i = ccnl_interest_remove(relay, i);

    if (faceid < 0)
        ccnl_nfn_continue_computation(relay, -faceid, 1);

    return i;
}

// ----------------------------------------------------------------------
// prefix (and NFN) related functionality

int
ccnl_nfnprefix_isNFN(struct ccnl_prefix_s *p)
{
    return p->nfnflags & CCNL_PREFIX_NFN;
}

int
ccnl_nfnprefix_isTHUNK(struct ccnl_prefix_s *p)
{
    return p->nfnflags & CCNL_PREFIX_THUNK;
}

int
ccnl_nfnprefix_contentIsNACK(struct ccnl_content_s *c)
{
    return !memcmp(c->content, ":NACK", 5);
}

void
ccnl_nfnprefix_set(struct ccnl_prefix_s *p, unsigned int flags)
{
    p->nfnflags |= flags;
}

void
ccnl_nfnprefix_clear(struct ccnl_prefix_s *p, unsigned int flags)
{
    p->nfnflags &= ~flags;
}

int
ccnl_nfnprefix_fillCallExpr(char *buf, struct fox_machine_state_s *s,
                            int exclude_param)
{
    int len, j;
    struct stack_s *entry;
    struct const_s *con;

    DEBUGMSG(99, "exclude parameter: %d\n", exclude_param);
    if (exclude_param >= 0){
        len = sprintf(buf, "(@x call %d", s->num_of_params);
    }
    else{
        len = sprintf(buf, "call %d", s->num_of_params);
    }

    for (j = 0; j < s->num_of_params; j++) {
        if (j == exclude_param) {
            len += sprintf(buf + len, " x");
            continue;
        }
        entry = s->params[j];
        switch (entry->type) {
        case STACK_TYPE_INT:
            len += sprintf(buf + len, " %d", *((int*)entry->content));
            break;
        case STACK_TYPE_PREFIX:
            len += sprintf(buf + len, " %s",
                           ccnl_prefix_to_path((struct ccnl_prefix_s*)entry->content));
            break;
        case STACK_TYPE_CONST:
            con = (struct const_s *)entry->content;
            char *str = ccnl_nfn_krivine_const2str(con);
            len += sprintf(buf + len, " %.*s", con->len+2, str);
            ccnl_free(str);
            break;

        default:
            DEBUGMSG(1, "Invalid type in createComputationString() #%d (%d)\n",
                     j, entry->type);
            break;
        }
    }
    if (exclude_param >= 0)
        len += sprintf(buf + len, ")");
    return len;
}

struct ccnl_prefix_s *
ccnl_nfnprefix_mkCallPrefix(struct ccnl_prefix_s *name, int thunk_request,
                            struct configuration_s *config, int parameter_num)
{
    int i, len, offset = 0;
    struct ccnl_prefix_s *p;
    char *bytes = ccnl_malloc(CCNL_MAX_PACKET_SIZE);

    p = ccnl_prefix_new(name->suite, name->compcnt + 1);
    if (!p)
        return NULL;

    p->compcnt = name->compcnt + 1;
    p->nfnflags = CCNL_PREFIX_NFN;
    if (thunk_request)
        p->nfnflags |= CCNL_PREFIX_THUNK;

    for (i = 0, len = 0; i < name->compcnt; i++) {
        p->complen[i] = name->complen[i];
        p->comp[i] = (unsigned char*)(bytes + len);
        memcpy(p->comp[i], name->comp[i], p->complen[i]);
        len += p->complen[i];
    }

#ifdef USE_SUITE_CCNTLV
    if (p->suite == CCNL_SUITE_CCNTLV)
        offset = 4;
#endif
    p->comp[i] = (unsigned char*)(bytes + len);
    p->complen[i] = ccnl_nfnprefix_fillCallExpr(bytes + len + offset,
                                                config->fox_state,
                                                parameter_num);
#ifdef USE_SUITE_CCNTLV
    if (p->suite == CCNL_SUITE_CCNTLV) {
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, p->complen[i],
                              &offset, (unsigned char*) bytes + len);
        p->complen[i] += 4;
    }
#endif
    len += p->complen[i];

    p->bytes = ccnl_realloc(bytes, len);
    for (i = 0; i < p->compcnt; i++)
        p->comp[i] = (unsigned char*)(p->bytes + ((char*)p->comp[i] - bytes));

    return p;
}

struct ccnl_prefix_s*
ccnl_nfnprefix_mkComputePrefix(struct configuration_s *config, int suite)
{
    int i, len = 0, offset = 0;
    struct ccnl_prefix_s *p;
    char *bytes = ccnl_malloc(CCNL_MAX_PACKET_SIZE);

    p = ccnl_prefix_new(suite, 2);
    p->compcnt = 2;
    if (!p)
        return NULL;
    p->nfnflags = CCNL_PREFIX_NFN;
    if (config->fox_state->thunk_request)
        p->nfnflags |= CCNL_PREFIX_THUNK;

    p->comp[0] = (unsigned char*) bytes;
    len = p->complen[0] = ccnl_pkt_mkComponent(suite, p->comp[0], "COMPUTE");

#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV)
        offset = 4;
#endif
    p->comp[1] = (unsigned char*) (bytes + len);
    p->complen[1] = ccnl_nfnprefix_fillCallExpr(bytes + len + offset,
                                                config->fox_state, -1);
#ifdef USE_SUITE_CCNTLV
    if (suite == CCNL_SUITE_CCNTLV) {
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, p->complen[1],
                              &offset, p->comp[1]);
        p->complen[1] += 4;
    }
#endif
    len += p->complen[1];

    p->bytes = ccnl_realloc(bytes, len);
    for (i = 0; i < p->compcnt; i++)
        p->comp[i] = (unsigned char*)(p->bytes + ((char*)p->comp[i] - bytes));

    return p;
}

// eof
