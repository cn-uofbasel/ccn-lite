/*
 * krivine-common.c
 * Tools for the "Krivine lambda expression resolver" for CCN
 *
 * (C) 2014 <christian.tschudin@unibas.ch>
 *
 * 2014-03-14 created <christopher.scherb@unibas.ch>
 */

#ifndef KRIVINE_COMMON_C
#define KRIVINE_COMMON_C

#include "krivine-common.h"

#include "ccnl.h"
#ifdef USE_SUITE_CCNB
#include "pkt-ccnb-enc.c"
#endif
#ifdef USE_SUITE_NDNTLV
#include "pkt-ndntlv-enc.c"
#endif

#include "ccnl-objects.c"

#include "ccnl-core.h"


char*
ccnl_prefix_to_path2(struct ccnl_prefix_s *pr)
{
//    static char prefix_buf[1024];
    char *prefix_buf = malloc(4096);
    int len= 0, i;

    if (!pr)
    return NULL;
    for (i = 0; i < pr->compcnt; i++) {
        if(!strncmp("call", pr->comp[i], 4) && strncmp(pr->comp[pr->compcnt-1], "NFN", 3))
            len += sprintf(prefix_buf + len, "%s", pr->comp[i]);
        else
            len += sprintf(prefix_buf + len, "/%s", pr->comp[i]);
    }
    prefix_buf[len] = '\0';
    return prefix_buf;
}

struct fox_machine_state_s *
new_machine_state(int thunk_request, int num_of_required_thunks){
    struct fox_machine_state_s *ret = malloc(sizeof(struct fox_machine_state_s));
    ret->thunk_request = thunk_request;
    ret->num_of_required_thunks = num_of_required_thunks;
    ret->prefix_mapping = NULL;
    ret->it_routable_param = 0;
    ret->num_of_params = 0;
    ret->num_of_required_thunks = 0;
    ret->thunk = 0;
    return ret;
}

struct configuration_s *
new_config(char *prog, struct environment_s *global_dict, int thunk_request,
           int start_locally, int num_of_required_thunks, struct ccnl_prefix_s *prefix,
           int configid, int suite){
    struct configuration_s *ret = malloc(sizeof(struct configuration_s));
    ret->prog = prog;
    ret->result_stack = NULL;
    ret->argument_stack = NULL;
    ret->env = NULL;
    ret->global_dict = global_dict;
    ret->fox_state = new_machine_state(thunk_request, num_of_required_thunks);
    ret->configid = configid;
    ret->start_locally = start_locally;
    ret->prefix = prefix;
    ret->suite = suite;
    ret->thunk = 0;
    ret->local_done = 0;
    ret->thunk_time = NFN_DEFAULT_WAITING_TIME;
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
add_computation_components(struct ccnl_prefix_s *prefix, int thunk_request, unsigned char *comp){
    int i = 0;
    /*while(namecomp[i]) ++i;

    namecomp[i++] = comp;
    if(thunk_request) namecomp[i++] = (unsigned char *)"THUNK";
    namecomp[i++] = (unsigned char *)"NFN";
    namecomp[i] = NULL;*/

    int size = thunk_request ? 3 : 2;

    struct ccnl_prefix_s *ret = malloc(sizeof(struct ccnl_prefix_s));
    ret->comp = malloc((prefix->compcnt +size) * sizeof(char*));
    ret->complen = malloc((prefix->compcnt +size) * sizeof(int));
    ret->compcnt = prefix->compcnt +size;
    ret->path = NULL;
    for(i = 0; i < prefix->compcnt; ++i){
#ifndef USE_UTIL
        DEBUGMSG(99, "COPY %s\n", prefix->comp[i]);
#endif
        ret->comp[i] = strdup(prefix->comp[i]);
        ret->complen[i] = prefix->complen[i];
    }
    ret->comp[i] = comp;
    ret->complen[i] = strlen(comp);
    ++i;
    if(thunk_request){
        ret->comp[i] = (unsigned char *)"THUNK";
        ret->complen[i] = strlen("THUNK");
        ++i;
    }
    ret->comp[i] = (unsigned char *)"NFN";
    ret->complen[i] = strlen("NFN");

    return ret;
}

struct ccnl_prefix_s *
add_local_computation_components(struct configuration_s *config){

    int i = 0;
    char *comp = malloc(CCNL_MAX_PACKET_SIZE);
    struct ccnl_prefix_s *ret;
    int complen = sprintf(comp, "call %d", config->fox_state->num_of_params);
    for(i = 0; i < config->fox_state->num_of_params; ++i){
        if(config->fox_state->params[i]->type == STACK_TYPE_INT)
            complen += sprintf(comp+complen, " %d", *((int*)config->fox_state->params[i]->content));
        else if(config->fox_state->params[i]->type == STACK_TYPE_PREFIX)
            complen += sprintf(comp+complen, " %s", ccnl_prefix_to_path2((struct ccnl_prefix_s*)config->fox_state->params[i]->content));
#ifndef USE_UTIL
        else DEBUGMSG(1, "Invalid type %d\n", config->fox_state->params[i]->type);
#endif
    }

    i = 0;
    ret = malloc(sizeof(struct ccnl_prefix_s));
    int size = config->fox_state->thunk_request ? 4 : 3;

    ret->comp = malloc(sizeof(char*) * size);
    ret->complen = malloc(sizeof(int) * size);

    ret->comp[i] = (unsigned char *)"COMPUTE";
    ret->complen[i] = strlen("COMPUTE");
    ++i;
    ret->comp[i] = (unsigned char *)strdup(comp);
    ret->complen[i] = strlen(comp);
    ++i;
    if(config->fox_state->thunk_request){
        ret->comp[i] = (unsigned char *)"THUNK";
        ret->complen[i] = strlen("THUNK");
        ++i;
    }
    ret->comp[i] = (unsigned char *)"NFN";
    ret->complen[i] = strlen("NFN");

    ret->compcnt = i+1;

    return ret;
}

int
createComputationString(struct configuration_s *config, int parameter_num, unsigned char *comp){

    int i;
    int complen = sprintf((char*)comp, "(@x call %d", config->fox_state->num_of_params);
    for(i = 0; i < config->fox_state->num_of_params; ++i){
        if(parameter_num == i){
            complen += sprintf((char*)comp + complen, " x");
        }
        else{
            //complen += sprintf((char*)comp + complen, "%s ", config->fox_state->params[j]);

            if(config->fox_state->params[i]->type == STACK_TYPE_INT)
                complen += sprintf(comp+complen, " %d", *((int*)config->fox_state->params[i]->content));
            else if(config->fox_state->params[i]->type == STACK_TYPE_PREFIX)
                complen += sprintf(comp+complen, " %s", ccnl_prefix_to_path2((struct ccnl_prefix_s*)config->fox_state->params[i]->content));
#ifndef USE_UTIL
            else DEBUGMSG(1, "Invalid type in createComputationString() %d\n", config->fox_state->params[i]->type);
#endif
        }
    }
    complen += sprintf((char*)comp + complen, ")");
    return complen;
}

struct ccnl_prefix_s *
create_prefix_from_name(char* namestr)
{ 
    int i = 0, j = 0;
    char *cp = NULL;
    char *prefixcomp[CCNL_MAX_NAME_COMP];
    struct ccnl_prefix_s *prefix = malloc(sizeof(struct ccnl_prefix_s));
    cp = strtok(namestr, "/");
    
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefixcomp[i++] = (unsigned char *)cp;
        cp = strtok(NULL, "/");
    }
    prefixcomp[i] = NULL;
    prefix->comp = (char **)malloc(i*sizeof(char*));
    prefix->compcnt = i;
    prefix->path = NULL;
    prefix->complen = (int *)malloc(i*sizeof(int));
    for(j = 0; j < i; ++j){
        prefix->complen[j] = strlen(prefixcomp[j]);
        prefix->comp[j] = strdup(prefixcomp[j]);
    }
    return prefix;
}

#ifndef USE_UTIL
void
set_propagate_of_interests_to_1(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *pref){
    struct ccnl_interest_s *interest = NULL;
    for(interest = ccnl->pit; interest; interest = interest->next){
        if(!ccnl_prefix_cmp(interest->prefix, 0, pref, CMP_EXACT)){
            interest->propagate = 1;
        }
    }
}

struct ccnl_prefix_s *
create_prefix_for_content_on_result_stack(struct ccnl_relay_s *ccnl, struct configuration_s *config){
    struct ccnl_prefix_s *name = malloc(sizeof(struct ccnl_prefix_s));
    name->comp = malloc(2*sizeof(char*));
    name->complen = malloc(2*sizeof(int));
    name->compcnt = 1;
    name->comp[1] = "NFN";
    name->complen[1] = 3;
    name->comp[0] = malloc(CCNL_MAX_PACKET_SIZE);
    memset(name->comp[0], 0, CCNL_MAX_PACKET_SIZE);
    name->path = NULL;

    int it, len = 0;
    len += sprintf(name->comp[0]+len, "call %d", config->fox_state->num_of_params);
    for(it = 0; it < config->fox_state->num_of_params; ++it){

        struct stack_s *stack = config->fox_state->params[it];
        if(stack->type == STACK_TYPE_PREFIX){
            char *pref_str = ccnl_prefix_to_path2((struct ccnl_prefix_s*)stack->content);
            len += sprintf(name->comp[0]+len, " %s", pref_str);
        }
        else if(stack->type == STACK_TYPE_INT){
            len += sprintf(name->comp[0]+len, " %d", *(int*)stack->content);
        }
        else{
            DEBUGMSG(1, "Invalid stack type\n");
            return NULL;
        }

    }
    name->complen[0] = len;
    return name;
}

void
ccnl_nfn_copy_prefix(struct ccnl_prefix_s *prefix, struct ccnl_prefix_s **copy){
    DEBUGMSG(2, "ccnl_nfn_copy_prefix()\n");
    int i = 0;
    (*copy) = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    (*copy)->compcnt = prefix->compcnt;

    (*copy)->complen = ccnl_malloc(sizeof(int) * prefix->compcnt);
    (*copy)->comp = ccnl_malloc(sizeof(char *) * prefix->compcnt);

    if(prefix->path)(*copy)->path = (unsigned char*)strdup((char *)prefix->path);
    for(i = 0; i < (*copy)->compcnt; ++i){
        (*copy)->complen[i] = prefix->complen[i];
        (*copy)->comp[i] = (unsigned char*)strdup((char*)prefix->comp[i]);
    }
}

void
ccnl_nfn_delete_prefix(struct ccnl_prefix_s *prefix){
    int i;
    if(prefix->complen)free(prefix->complen);
    for(i = 0; i < prefix->compcnt; ++i){
        if(prefix->comp[i])free(prefix->comp[i]);
    }
    prefix->compcnt = 0;
}

struct ccnl_content_s *
ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl, struct configuration_s *config, struct ccnl_prefix_s *prefix){
    DEBUGMSG(2, "ccnl_nfn_local_content_search()\n");

    struct ccnl_content_s *content;
    int i;
    DEBUGMSG(99, "Searching local for content %s\n", ccnl_prefix_to_path2(prefix));
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
ccnl_nfn_add_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config, struct ccnl_prefix_s *prefix){
    DEBUGMSG(2, "ccnl_nfn_add_thunk()\n");
    struct ccnl_prefix_s *new_prefix;
    ccnl_nfn_copy_prefix(prefix, &new_prefix);
    
    if(!strncmp((char *)new_prefix->comp[new_prefix->compcnt-2], "THUNK", 5)){
        new_prefix->comp[new_prefix->compcnt-2] = (unsigned char*)"NFN";
        new_prefix->comp[new_prefix->compcnt-1] = NULL;
        --new_prefix->compcnt;
    }    
    struct thunk_s *thunk = ccnl_malloc(sizeof(struct thunk_s));
    thunk->prefix = new_prefix;
    thunk->reduced_prefix = create_prefix_for_content_on_result_stack(ccnl, config);
    sprintf(thunk->thunkid, "THUNK%d", thunkid++);
    DBL_LINKED_LIST_ADD(thunk_list, thunk);
    DEBUGMSG(99, "Created new thunk with id: %s\n", thunk->thunkid);
    return strdup(thunk->thunkid);
}

struct thunk_s *
ccnl_nfn_get_thunk(unsigned char *thunkid){
    struct thunk_s *thunk;
    for(thunk = thunk_list; thunk; thunk = thunk->next){
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
    struct thunk_s *thunk = ccnl_nfn_get_thunk(thunkid);
    if(thunk){
        char *out = ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
        memset(out, 0, CCNL_MAX_PACKET_SIZE);
        struct ccnl_interest_s *interest = mkInterestObject(ccnl, config, thunk->prefix);
        return interest;
    }
    return NULL;
}

void
ccnl_nfn_remove_thunk(char* thunkid){
    DEBUGMSG(2, "ccnl_nfn_remove_thunk()\n");
    struct thunk_s *thunk;
    for(thunk = thunk_list; thunk; thunk = thunk->next){
        if(!strncmp(thunk->thunkid, thunkid, sizeof(thunk->thunkid))){
            break;
        }
    }
    DBL_LINKED_LIST_REMOVE(thunk_list, thunk);
}

int 
ccnl_nfn_reply_thunk(struct ccnl_relay_s *ccnl, struct configuration_s *config){
    DEBUGMSG(2, "ccnl_nfn_reply_thunk()\n");
    struct ccnl_prefix_s *original_prefix = config->prefix;
    char reply_content[100];
    memset(reply_content, 0, 100);
    int thunk_time = (int)config->thunk_time; 
    sprintf(reply_content, "%d", thunk_time);
    struct ccnl_content_s *c = create_content_object(ccnl, original_prefix, (unsigned char*)reply_content,
                                                     strlen(reply_content), config->suite);
    set_propagate_of_interests_to_1(ccnl, c->name);
    ccnl_content_add2cache(ccnl, c);
    ccnl_content_serve_pending(ccnl,c);
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
#endif //KRIVINE_COMMON_C
