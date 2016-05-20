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
    c[con->len+2] = '\0';
    return c;
}

// ----------------------------------------------------------------------

struct ccnl_interest_s *
ccnl_nfn_query2interest(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        struct configuration_s *config)
{
    int nonce = rand();
    struct ccnl_pkt_s *pkt;
    struct ccnl_face_s *from;
    struct ccnl_interest_s *i;

    DEBUGMSG(TRACE, "nfn_query2interest(configID=%d)\n", config->configid);

    from = ccnl_malloc(sizeof(struct ccnl_face_s));
    pkt = ccnl_calloc(1, sizeof(*pkt));
    if (!from || !pkt) {
        free_2ptr_list(from, pkt);
        return NULL;
    }
    from->faceid = config->configid;
    from->last_used = CCNL_NOW();
    from->outq = NULL;

    pkt->suite = (*prefix)->suite;
    switch(pkt->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        pkt->s.ccnb.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
#endif
    case CCNL_SUITE_NDNTLV:
        pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;
        break;
    default:
        break;
    }
    pkt->buf = ccnl_mkSimpleInterest(*prefix, &nonce);
    pkt->pfx = *prefix;
    *prefix = NULL;
    pkt->val.final_block_id = -1;

    i = ccnl_interest_new(ccnl, from, &pkt);
    if (i) {
        DEBUGMSG(TRACE, "  new interest %p, from=%p, i->from=%p\n",
                 (void*)i, (void*)from, (void*)i->from);
    }
    return i;
}

struct ccnl_content_s *
ccnl_nfn_result2content(struct ccnl_relay_s *ccnl,
                        struct ccnl_prefix_s **prefix,
                        unsigned char *resultstr, int resultlen)
{
    int resultpos = 0;
    struct ccnl_pkt_s *pkt;
    struct ccnl_content_s *c;

    DEBUGMSG(TRACE, "ccnl_nfn_result2content(prefix=%s, suite=%s, contlen=%d)\n",
             ccnl_prefix_to_path(*prefix), ccnl_suite2str((*prefix)->suite),
             resultlen);

    pkt = ccnl_calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

    pkt->buf = ccnl_mkSimpleContent(*prefix, resultstr, resultlen, &resultpos);
    if (!pkt->buf) {
        ccnl_free(pkt);
        return NULL;
    }
    pkt->pfx = *prefix;
    *prefix = NULL;
    pkt->content = pkt->buf->data + resultpos;
    pkt->contlen = resultlen;

    c = ccnl_content_new(ccnl, &pkt);
    if (!c)
        free_packet(pkt);
    return c;
}

// ----------------------------------------------------------------------
struct fox_machine_state_s *
new_machine_state()
{
    struct fox_machine_state_s *ret;
    ret = ccnl_calloc(1, sizeof(struct fox_machine_state_s));
    return ret;
}

struct configuration_s *
new_config(struct ccnl_relay_s *ccnl, char *prog,
           struct environment_s *global_dict,
           int start_locally,
           struct ccnl_prefix_s *prefix, int configid, int suite)
{
    struct configuration_s *ret;

    ret = ccnl_calloc(1, sizeof(struct configuration_s));
    ret->prog = prog;
    ret->global_dict = global_dict;
    ret->fox_state = new_machine_state();
    ret->configid = ccnl->km->configid;
    ret->start_locally = start_locally;
    ret->prefix = prefix;
    ret->suite = suite;

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

        switch (s->type) {
        case STACK_TYPE_CLOSURE:
            ccnl_nfn_freeClosure((struct closure_s *)s->content);
            break;
        case STACK_TYPE_PREFIX:
            free_prefix(((struct ccnl_prefix_s *)s->content));
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
    DEBUGMSG(VERBOSE, "nfn_freeMachineState %p\n", (void*)f);

    if (!f)
        return;
    if (f->params) {
        struct stack_s **s;
        int i = f->num_of_params;
        for (s = f->params; i > 0; s++, i--)
            ccnl_nfn_freeStack(*s);
        ccnl_free(f->params);
    }
    /*while (f->prefix_mapping) {
        struct prefix_mapping_s *m = f->prefix_mapping;
        //FIXME: WHY SEGFAULT HERE???
        struct prefix_mapping_s *l = f->prefix_mapping;
        struct prefix_mapping_s *e = m;
        if ((l) == (e)) (l) = (e)->next;
        if ((e)->prev) (e)->prev->next = (e)->next;
        if ((e)->next) (e)->next->prev = (e)->prev;
        //DBL_LINKED_LIST_REMOVE(f->prefix_mapping, m);
        free_prefix(m->key);
        free_prefix(m->value);
    }*/
    ccnl_free(f);
    TRACEOUT();
}

void
ccnl_nfn_freeConfiguration(struct configuration_s* c)
{
    DEBUGMSG(TRACE, "ccnl_nfn_freeConfiguration(%p)\n", (void*)c);

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
    DEBUGMSG(TRACE, "ccnl_nfn_freeKrivine(%p)\n", ccnl ? (void*)ccnl->km : NULL);

    if (!ccnl || !ccnl->km)
        return;

    DEBUGMSG(DEBUG, "  configuration_list %p\n",
             (void*)ccnl->km->configuration_list);
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

// #ifndef USE_UTIL
void
set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref){
    struct ccnl_interest_s *interest = NULL;
    for(interest = ccnl->pit; interest; interest = interest->next){
        if(!ccnl_prefix_cmp(interest->pkt->pfx, 0, pref, CMP_EXACT)){
            interest->flags |= CCNL_PIT_COREPROPAGATES;
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

#if defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)
    if (config->suite == CCNL_SUITE_CCNTLV ||
                                         config->suite == CCNL_SUITE_CISTLV)
        offset = 4;
#endif
    name->bytes = ccnl_calloc(1, CCNL_MAX_PACKET_SIZE);
    name->compcnt = 1;
    //len = ccnl_pkt_mkComponent(config->suite, name->bytes, "NFN", strlen("NFN")); //FIXME: AT THE END?
    name->complen[1] = len;
    name->comp[0] = name->bytes + offset + len;

    len += sprintf((char *)name->bytes + offset + len, "(call %d",
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

        } else if (stack->type == STACK_TYPE_CONST) {
            struct const_s *con = stack->content;
            DEBUGMSG(DEBUG, "strlen: %d str: %.*s \n", con->len, con->len+2, ccnl_nfn_krivine_const2str(con));
            len += sprintf((char*)name->bytes + offset + len, " %.*s", con->len+2, ccnl_nfn_krivine_const2str(con));
        } else {
            DEBUGMSG(WARNING, "Invalid stack type %d\n", stack->type);
            return NULL;
        }

    }

    len += sprintf((char *)name->bytes + offset + len, ")");
#ifdef USE_SUITE_CCNTLV
    if (config->suite == CCNL_SUITE_CCNTLV) {
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, len, &offset,
                              name->bytes + name->complen[1]);
        len += 4;
    }
#endif
#ifdef USE_SUITE_CISTLV
    if (config->suite == CCNL_SUITE_CISTLV) {
        ccnl_cistlv_prependTL(CISCO_TLV_NameComponent, len, &offset,
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
    struct ccnl_prefix_s *prefixchunkzero;

    DEBUGMSG(TRACE, "ccnl_nfn_local_content_search(%s, suite=%s)\n",
             ccnl_prefix_to_path(prefix), ccnl_suite2str(prefix->suite));

    DEBUGMSG(DEBUG, "Searching local for content %s\n", ccnl_prefix_to_path(prefix));

    for (content = ccnl->contents; content; content = content->next) {
        if (content->pkt->pfx->suite == prefix->suite &&
                    !ccnl_prefix_cmp(prefix, 0, content->pkt->pfx, CMP_EXACT))
            return content;
    }

    // If the content for the prefix is chunked, the exact match on the prefix fails.
    // We assume that if chunk 0 is available the content is available.
    // Searching the content again with the same prefix for chunk 0.
    prefixchunkzero = ccnl_prefix_dup(prefix);
    ccnl_prefix_addChunkNum(prefixchunkzero, 0);
    for (content = ccnl->contents; content; content = content->next) {
        if (content->pkt->pfx->suite == prefix->suite &&
           !ccnl_prefix_cmp(prefixchunkzero, 0, content->pkt->pfx, CMP_EXACT)) {
            free_prefix(prefixchunkzero);
            return content;
        }
    }
    if (prefixchunkzero) free_prefix(prefixchunkzero);
    prefixchunkzero = 0;

    if (!config || !config->fox_state || !config->fox_state->prefix_mapping)
        return NULL;

    for (iter = config->fox_state->prefix_mapping; iter; iter = iter->next) {
        if (!ccnl_prefix_cmp(prefix, 0, iter->key, CMP_EXACT)) {
            for (content = ccnl->contents; content; content = content->next) {
                if (content->pkt->pfx->suite == prefix->suite &&
                 !ccnl_prefix_cmp(iter->value, 0, content->pkt->pfx, CMP_EXACT))
                    return content;
            }
        }
    }

    return NULL;
}
// #endif //USE_UTIL


#ifdef USE_NACK
void
ccnl_nack_reply(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
                struct ccnl_face_s *from, int suite)
{
    struct ccnl_content_s *nack;
    DEBUGMSG(TRACE, "ccnl_nack_reply()\n");
    if (from->faceid <= 0) {
        return;
    }
    nack = ccnl_nfn_result2content(ccnl, &prefix,
                                    (unsigned char*)":NACK", 5);
    ccnl_nfn_monitor(ccnl, from, nack->pkt->pfx,
                     nack->pkt->content, nack->pkt->contlen);
    DEBUGMSG(WARNING, "+++ nack->pkt is %p\n", (void*) nack->pkt);
    ccnl_face_enqueue(ccnl, from, nack->pkt->buf);
}
#endif // USE_NACK

struct ccnl_interest_s*
ccnl_nfn_interest_remove(struct ccnl_relay_s *relay, struct ccnl_interest_s *i)
{
    int faceid = 0;
    struct ccnl_face_s *face;

    if (!i) {
        DEBUGMSG(WARNING, "nfn_interest_remove: should remove NULL interest\n");
        return NULL;
    }

    face = i->from;
    if (face)
        faceid = face->faceid;
    DEBUGMSG(DEBUG, "ccnl_nfn_interest_remove %d\n", faceid);

    if (faceid < 0)
        ccnl_free(face);
#ifdef USE_NACK
    else
        ccnl_nack_reply(relay, i->pkt->pfx, face, i->pkt->suite);
#endif
    i->from = NULL;
    i = ccnl_interest_remove(relay, i);

    if (faceid < 0)
        ccnl_nfn_continue_computation(relay, -faceid, 1);

    TRACEOUT();
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
ccnl_nfnprefix_contentIsNACK(struct ccnl_content_s *c)
{
    return !memcmp(c->pkt->content, ":NACK", 5);
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

    DEBUGMSG(DEBUG, "exclude parameter: %d\n", exclude_param);
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
            DEBUGMSG(ERROR, "Invalid type in createComputationString() #%d (%d)\n",
                     j, entry->type);
            break;
        }
    }
    if (exclude_param >= 0)
        len += sprintf(buf + len, ")");
    return len;
}

struct ccnl_prefix_s *
ccnl_nfnprefix_mkCallPrefix(struct ccnl_prefix_s *name,
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

    for (i = 0, len = 0; i < name->compcnt; i++) {
        p->complen[i] = name->complen[i];
        p->comp[i] = (unsigned char*)(bytes + len);
        memcpy(p->comp[i], name->comp[i], p->complen[i]);
        len += p->complen[i];
    }

    switch (p->suite) {
#if defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)
    case CCNL_SUITE_CCNTLV:
    case CCNL_SUITE_CISTLV:
        offset = 4;
        break;
#endif
    default:
        break;
    }
    p->comp[i] = (unsigned char*)(bytes + len);
    p->complen[i] = ccnl_nfnprefix_fillCallExpr(bytes + len + offset,
                                                config->fox_state,
                                                parameter_num);
    switch (p->suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, p->complen[i],
                              &offset, p->comp[i]);
        p->complen[i] += 4;
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        ccnl_cistlv_prependTL(CISCO_TLV_NameComponent, p->complen[i],
                              &offset, p->comp[i]);
        p->complen[i] += 4;
        break;
#endif
    default:
        break;
    }
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

    p->comp[0] = (unsigned char*) bytes;
    len = ccnl_pkt_mkComponent(suite, p->comp[0], "COMPUTE", strlen("COMPUTE"));
    p->complen[0] = len;

    switch (p->suite) {
#if defined(USE_SUITE_CCNTLV) || defined(USE_SUITE_CISTLV)
    case CCNL_SUITE_CCNTLV:
    case CCNL_SUITE_CISTLV:
        offset = 4;
        break;
#endif
    default:
        break;
    }
    p->comp[1] = (unsigned char*) (bytes + len);
    p->complen[1] = ccnl_nfnprefix_fillCallExpr(bytes + len + offset,
                                                config->fox_state, -1);
    switch (p->suite) {
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        ccnl_ccntlv_prependTL(CCNX_TLV_N_NameSegment, p->complen[1],
                              &offset, p->comp[1]);
        p->complen[1] += 4;
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        ccnl_cistlv_prependTL(CISCO_TLV_NameComponent, p->complen[1],
                              &offset, p->comp[1]);
        p->complen[1] += 4;
        break;
#endif
    default:
        break;
    }
    len += p->complen[1];

    p->bytes = ccnl_realloc(bytes, len);
    for (i = 0; i < p->compcnt; i++)
        p->comp[i] = (unsigned char*)(p->bytes + ((char*)p->comp[i] - bytes));

    return p;
}

// eof
