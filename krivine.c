#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include "krivine-common.c"

#define LAMBDA '@'

#define term_is_var(t)     (!(t)->m)
#define term_is_app(t)     ((t)->m && (t)->n)
#define term_is_lambda(t)  (!(t)->n)

//Machine state functions
//------------------------------------------------------------------
struct stack_s{
    void *content;
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

struct configuration_s{
    char *prog;
    struct stack_s *result_stack;
    struct stack_s *argument_stack;
    struct environment_s *env; 
    struct environment_s *global_dict;
    struct thread_s *thread;
};

struct closure_s *
new_closure(char *term, struct environment_s *env){
    struct closure_s *ret = malloc(sizeof(struct closure_s));
    ret->term = term;
    ret->env = env;
    return ret;
}

struct configuration_s *
new_config(char *prog, struct environment_s *global_dict){
    struct configuration_s *ret = malloc(sizeof(struct configuration_s));
    ret->prog = prog;
    ret->result_stack = NULL;
    ret->argument_stack = NULL;
    ret->env = NULL;
    ret->global_dict = global_dict;
    return ret;
}

void 
push_to_stack(struct stack_s **top, void *content){
    struct stack_s *h = malloc(sizeof(struct stack_s));
    if(top != NULL && *top != NULL){
                h->next = *top;
        }else{
	        h->next = NULL;
	    }
    h->content = content;
    *top = h;
}

void *
pop_from_stack(struct stack_s **top){
    if(top == NULL) return NULL;
    if( *top != NULL){
            struct stack_s *h = *top;
            void *res = h->content;
    	*top = h->next;
            free(h);
            return res;
        }
    return NULL;
}

void 
print_environment(struct environment_s *env){
   struct environment_s *e;
   printf("Environment address is: %p\n",env);
   int num = 0;
   for(e = env; e; e = e->next){
	printf("Element %d %s %p\n", num++, e->name, e->element);        
   }
}

void 
print_result_stack(struct stack_s *stack){
   struct stack_s *s;
   int num = 0;
   for(s = stack; s; s = s->next){
       printf("Element %d %s\n", num++, s->content);
   }
}

void 
print_argument_stack(struct stack_s *stack){
   struct stack_s *s;
   int num = 0;
   for(s = stack; s; s = s->next){
        struct closure_s *c = s->content;
	printf("Element %d %s\n ---Env: \n", num++, c->term);  
	print_environment(c->env);
	printf("--End Env\n");      
   }
}

void 
add_to_environment(struct environment_s **env, char *name, void *element){
   struct environment_s *newelement = malloc(sizeof(struct environment_s));
   newelement->name = name;
   newelement->element = element;
   if(env == NULL || *env == NULL){
          newelement->next == NULL;
      }else{
             newelement->next = *env;
         }
   *env = newelement;
}

void *
search_in_environment(struct environment_s *env, char *name){
  struct environment_s *envelement;
  for(envelement = env; envelement; envelement = envelement->next){
    	if(!strcmp(name, envelement->name)){
		     return envelement->element;	
		}
    }
  return NULL;
}


//parse functions
//------------------------------------------------------------------

struct term_s {
    char *v;
    struct term_s *m, *n;
    // if m is 0, we have a var  v
    // is both m and n are not 0, we have an application  (M N)
    // if n is 0, we have a lambda term  /v M
};

char*
parse_var(char **cpp)
{
    char *p;
    int len;

    p = *cpp;
    while (*p && (isalnum(*p) || *p == '_' || *p == '=' || *p == '/'))
	p++;
    len = p - *cpp;
    p = malloc(len+1);
    if (!p)
	return 0;
    memcpy(p, *cpp, len);
    p[len] = '\0';
    *cpp += len;
    return p;
}

struct term_s*
parseKRIVINE(int lev, char **cp)
{
/* t = (v, m, n)

   var:     v!=0, m=0, n=0
   app:     v=0, m=f, n=arg
   lambda:  v!=0, m=body, n=0
 */
    struct term_s *t = 0, *s, *u;

    while (**cp) {
	while (isspace(**cp))
	    *cp += 1;

//	printf("parseKRIVINE %d %s\n", lev, *cp);

	if (**cp == ')')
	    return t;
	if (**cp == '(') {
	    *cp += 1;
	    s = parseKRIVINE(lev+1, cp);
	    if (!s)
		return 0;
	    if (**cp != ')') {
		printf("parseKRIVINE error: missing )\n");
		return 0;
	    } else
		*cp += 1;
	} else if (**cp == LAMBDA) {
	    *cp += 1;
	    s = calloc(1, sizeof(*s));
	    s->v = parse_var(cp);
	    s->m = parseKRIVINE(lev+1, cp);
//	    printKRIVINE(dummybuf, s->m, 0);
//	    printf("  after lambda: /%s %s --> <%s>\n", s->v, dummybuf, *cp);
	} else {
	    s = calloc(1, sizeof(*s));
	    s->v = parse_var(cp);
//	    printf("  var: <%s>\n", s->v);
	}
	if (t) {
//	    printKRIVINE(dummybuf, t, 0);
//	    printf("  old term: <%s>\n", dummybuf);
	    u = calloc(1, sizeof(*u));
	    u->m = t;
	    u->n = s;
	    t = u;
	} else
	    t = s;
//	printKRIVINE(dummybuf, t, 0);
//	printf("  new term: <%s>\n", dummybuf);
    }
//    printKRIVINE(dummybuf, t, 0);
//    printf("  we return <%s>\n", dummybuf);
    return t;
}

int
printKRIVINE(char *cfg, struct term_s *t, char last)
{
    int len = 0;

    if (t->v && t->m) { // Lambda (sequence)
	len += sprintf(cfg + len, "(%c%s", LAMBDA, t->v);
	len += printKRIVINE(cfg + len, t->m, 'a');
	len += sprintf(cfg + len, ")");
	return len;
    }
    if (t->v) { // (single) variable
	if (isalnum(last))
	    len += sprintf(cfg + len, " %s", t->v);
	else
	    len += sprintf(cfg + len, "%s", t->v);
	return len;
    }
    // application (sequence)
#ifdef CORRECT_PARENTHESES
    len += sprintf(cfg + len, "(");
    len += printKRIVINE(cfg + len, t->m, '(');
    len += printKRIVINE(cfg + len, t->n, 'a');
    len += sprintf(cfg + len, ")");
#else
    if (t->n->v && !t->n->m) {
	len += printKRIVINE(cfg + len, t->m, last);
	len += printKRIVINE(cfg + len, t->n, 'a');
    } else {
	len += printKRIVINE(cfg + len, t->m, last);
	len += sprintf(cfg + len, " (");
	len += printKRIVINE(cfg + len, t->n, '(');
	len += sprintf(cfg + len, ")");
    }
#endif
    return len;
}

int iscontent(char *cp){
	if(cp[0] == '/')
		return 1;
	return 0;
}
//------------------------------------------------------------
struct ccnl_content_s *
ccnl_nfn_handle_local_computation(struct ccnl_relay_s *ccnl, struct configuration_s *config,
        char **params, int num_params, char **namecomp, char *out, char *comp, int thunk_request){
    int complen = sprintf(comp, "call %d ", num_params);
    struct ccnl_interest_s * interest;
    struct ccnl_content_s *c;
    int i, len;
    for(i = 0; i < num_params; ++i){
        complen += sprintf(comp+complen, "%s ", params[i]);
    }
    i = 0;
    namecomp[i++] = "COMPUTE";
    namecomp[i++] = strdup(comp);
    if(thunk_request) namecomp[i++] = "THUNK";
    namecomp[i++] = "NFN";
    namecomp[i++] = NULL;
    len = mkInterest(namecomp, 0, out);
    interest = ccnl_nfn_create_interest_object(ccnl, out, len, namecomp[0]);
    
    /*if((c = ccnl_nfn_content_computation(ccnl, interest)) != NULL){
        DEBUGMSG(49, "Content can be computed");
        return c;
    }else{
        
        DEBUGMSG(49, "Got no thunk, continue with null result just for debugging\n");
        return 0;
    }*/
    interest->from->faceid = config->thread->id;
    printf("From face id %d\n", interest->from->faceid);
    ccnl_interest_propagate(ccnl, interest);
    printf("WAITING on Signal; Threadid : %d \n", config->thread->id);
    pthread_cond_wait(&config->thread->cond, &config->thread->mutex);
    //local search. look if content is now available!
    printf("Got signal CONTINUE\n");
    if((c = ccnl_nfn_local_content_search(ccnl, interest, CMP_MATCH)) != NULL){
        DEBUGMSG(49, "Content locally found\n");
        return c;
    }
    return 0;
}

struct ccnl_content_s *
ccnl_nfn_handle_network_search(struct ccnl_relay_s *ccnl, int current_param, char **params, 
        int num_params, char **namecomp, char *out, char *comp, char *param, 
        int thunk_request){
    struct ccnl_content_s *c;
    int j;
    int complen = sprintf(comp, "(@x call %d ", num_params);
    for(j = 0; j < num_params; ++j){
        if(current_param == j){
            complen += sprintf(comp + complen, "x ");
        }
        else{
            complen += sprintf(comp + complen, "%s ", params[j]);
        }
    }
    complen += sprintf(comp + complen, ")");
    DEBUGMSG(49, "Computation request: %s %s\n", comp, params[current_param]);
    //make interest
    int len = mkInterestCompute(namecomp, comp, complen, thunk_request, out); //TODO: no thunk request for local search  
    free(param);
    //search
    struct ccnl_interest_s *interest = ccnl_nfn_create_interest_object(ccnl, out, len, NULL); //FIXME: NAMECOMP[0]???
    if((c = ccnl_nfn_local_content_search(ccnl, interest, CMP_MATCH)) != NULL){
        DEBUGMSG(49, "Content locally found\n");
        return c;
    }

    else if((c = ccnl_nfn_global_content_search(ccnl, interest)) != NULL){
        DEBUGMSG(49, "Content found in the network\n");
        return c;
    }
}

struct ccnl_content_s *
ccnl_nfn_handle_routable_content(struct ccnl_relay_s *ccnl, struct configuration_s *config,
        int current_param, char **params, int num_params, int thunk_request){
    char *out =  ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
    char *comp =  ccnl_malloc(sizeof(char) * CCNL_MAX_PACKET_SIZE);
    char *namecomp[CCNL_MAX_NAME_COMP];
    char *param;
    struct ccnl_content_s *c;
    int j, complen;
    memset(comp, 0, CCNL_MAX_PACKET_SIZE);
    memset(out, 0, CCNL_MAX_PACKET_SIZE);
    
    param = strdup(params[current_param]);
    j = splitComponents(param, namecomp);
    
    if(isLocalAvailable(ccnl, namecomp)){
        DEBUGMSG(49, "Routable content %s is local availabe --> start computation\n",
                params[current_param]);
        c = ccnl_nfn_handle_local_computation(ccnl, config, params, num_params, 
                namecomp, out, comp, thunk_request);
        return c;
    }else{
        //search for content in the network
       //FIXME: enable c = ccnl_nfn_handle_network_search(ccnl, current_param, params, num_params,
                //namecomp, out, comp, param, thunk_request);
    }
    return c;
}
//------------------------------------------------------------
char*
ZAM_term(struct ccnl_relay_s *ccnl, struct configuration_s *configuration, 
        int thunk_request, int *num_of_required_thunks,  
        struct ccnl_prefix_s *original_prefix, int *halt, char *dummybuf)
{
    struct term_s *t;
    char *pending, *p, *cp;
    int len;
    char *prog = configuration->prog;
    
    //pop closure

    if (!prog || strlen(prog) == 0) {
         if(configuration->result_stack){
		return configuration->result_stack->content;
         }
         printf("no result returned\n");
         return NULL;
    }


    pending = strchr(prog, ';');
    p = strchr(prog, '(');

    if (!strncmp(prog, "ACCESS(", 7)) {
	char *cname, *sname;
	if (pending){
	    cp = pending-1;
        }
        else{
	    cp = prog + strlen(prog) - 1;
        }
        len = cp - p;
        cp = malloc(len);
        memcpy(cp, p+1, len-1);
        cp[len-1] = '\0';
        DEBUGMSG(2, "---to do: access <%s>\n", cp);

	struct closure_s *closure = search_in_environment(configuration->env, cp);
	if (!closure) {
		closure = search_in_environment(configuration->global_dict, cp);
		if(!closure){
			printf("?? could not lookup var %s\n", cp);
			return 0;
		}
	}
	push_to_stack(&configuration->argument_stack, closure);
	
	return pending+1;
    }
    if (!strncmp(prog, "CLOSURE(", 8)) {
        if (pending)
            cp = pending-1;
        else
            cp = prog + strlen(prog) - 1;
	len = cp - p;
        cp = malloc(len);
        memcpy(cp, p+1, len-1);
        cp[len-1] = '\0';
        DEBUGMSG(2, "---to do: closure <%s>\n", cp);

        if (!configuration->argument_stack && !strncmp(cp, "RESOLVENAME(", 12)) {
            char v[500], *c;
            int len;
            c = strchr(cp+12, ')'); if (!c) goto normal;
	    len = c - (cp+12);
	    memcpy(v, cp+12, len);
	    v[len] = '\0';
	    struct closure_s *closure = search_in_environment(configuration->env, v);
	    if(!closure) goto normal;
            if(!strcmp(closure->term, cp)){
                DEBUGMSG(2, "** detected tail recursion case %s\n", closure->term);
            }else{
                goto normal;
            }
        }
        else{
            struct closure_s *closure;
normal:
            closure = new_closure(cp, configuration->env); 
            //configuration->env = NULL;//FIXME new environment?
            push_to_stack(&configuration->argument_stack, closure);
        }
        if(pending){
            cp = pending + 1;
        }
        else{
	    printf("** not implemented %d\n", __LINE__);
        }
        return cp;
    }
    if (!strncmp(prog, "GRAB(", 5)) {
        char *v, *cname;
        if (pending)
	    v = pending-1;
	else
	    v = prog + strlen(prog) - 1;
	len = v - p;
	v = malloc(len);
	memcpy(v, p+1, len-1);
	v[len-1] = '\0';
	DEBUGMSG(2, "---to do: grab <%s>\n", v);

	struct closure_s *closure = pop_from_stack(&configuration->argument_stack);
	add_to_environment(&configuration->env, v, closure);

	if(pending)
            pending++;
        return pending;
    }
    if (!strncmp(prog, "TAILAPPLY", 9)) {
        char *cname, *code;
        DEBUGMSG(2, "---to do: tailapply\n");

        struct closure_s *closure = pop_from_stack(&configuration->argument_stack);
        code = closure->term;
        if(pending){ //build new term
            sprintf(dummybuf, "%s%s", code, pending);
        }else{
            sprintf(dummybuf, "%s", code);
        }
	configuration->env = closure->env; //set environment from closure
        return dummybuf;
    }
    if (!strncmp(prog, "RESOLVENAME(", 12)) {
	char res[1000];
	memset(res,0,sizeof(res));
        if (pending)
	    cp = pending-1;
	else
	    cp = prog + strlen(prog) - 1;
        len = cp - p;
        cp = malloc(len);
	memcpy(cp, p+1, len-1);
	cp[len-1] = '\0';
	DEBUGMSG(2, "---to do: resolveNAME <%s>\n", cp);

        //check if term can be made available, if yes enter it as a var
	//try with searching in global env for an added term!
        t = parseKRIVINE(0, &cp);
	if (term_is_var(t)) {
	    char *end = 0;
            cp = t->v;
            if (isdigit(*cp)) {
		// is disgit...
		strtol(cp, &end, 0);
		if (end && *end)
		    end = 0;
	    }
            else if(iscontent(cp)){
	   	// is content... 
		DEBUGMSG(99, "VAR IS CONTENT: %s\n", cp);
		end = cp;
	    }
            if (end) {
		push_to_stack(&configuration->result_stack, cp);
		if (pending)
		    sprintf(res, "TAILAPPLY%s", pending);
		else
		    sprintf(res, "TAILAPPLY");
	    }else {
		if (pending)
		    sprintf(res, "ACCESS(%s);TAILAPPLY%s", t->v, pending);
		else
		    sprintf(res, "ACCESS(%s);TAILAPPLY", t->v);
	    }
            return strdup(res);
        }
	if (term_is_lambda(t)) {
            char *var;
	    var = t->v;
	    printKRIVINE(dummybuf, t->m, 0);
	    cp = strdup(dummybuf);
            if (pending)
		sprintf(res, "GRAB(%s);RESOLVENAME(%s)%s", var, cp, pending);
	    else
		sprintf(res, "GRAB(%s);RESOLVENAME(%s)", var, cp);
	    free(cp);
            return strdup(res);
        }
        if (term_is_app(t)) {
            printKRIVINE(dummybuf, t->n, 0);
	    p = strdup(dummybuf);
	    printKRIVINE(dummybuf, t->m, 0);
	    cp = strdup(dummybuf);
            
            if (pending)
  		sprintf(res, "CLOSURE(RESOLVENAME(%s));RESOLVENAME(%s)%s", p, cp, pending);
	    else
		sprintf(res, "CLOSURE(RESOLVENAME(%s));RESOLVENAME(%s)", p, cp);
	   
            return strdup(res);
        }
    }
    if (!strncmp(prog, "OP_CMPEQ", 8)) {
	int i1, i2, acc;
	char *h;
        char res[1000];
	memset(res, 0, sizeof(res));
	DEBUGMSG(2, "---to do: OP_CMPEQ <%s>/<%s>\n", cp, pending);
	h = pop_from_stack(&configuration->result_stack);
	i1 = atoi(h);
	h = pop_from_stack(&configuration->result_stack);
	i2 = atoi(h);
	acc = i1 == i2;
        cp =  acc ? "@x@y x" : "@x@y y";
        if (pending)
	    sprintf(res, "RESOLVENAME(%s)%s", cp, pending);
	else
	    sprintf(res, "RESOLVENAME(%s)", cp);
	return strdup(res);
    }
    if (!strncmp(prog, "OP_CMPLEQ", 9)) {
	int i1, i2, acc;
	char *h;
        char res[1000];
	memset(res, 0, sizeof(res));
	DEBUGMSG(2, "---to do: OP_CMPLEQ <%s>/%s\n", cp, pending);
	h = pop_from_stack(&configuration->result_stack);
	i1 = atoi(h);
	h = pop_from_stack(&configuration->result_stack);
	i2 = atoi(h);
	acc = i2 <= i1;
        cp =  acc ? "@x@y x" : "@x@y y";
        if (pending)
	    sprintf(res, "RESOLVENAME(%s)%s", cp, pending);
	else
	    sprintf(res, "RESOLVENAME(%s)", cp);
	return strdup(res);
    }
    if (!strncmp(prog, "OP_ADD", 6)) {
	int i1, i2, res;
	char *h;
	DEBUGMSG(2, "---to do: OP_ADD <%s>\n", prog+7);
	h = pop_from_stack(&configuration->result_stack);
	i1 = atoi(h);
	h = pop_from_stack(&configuration->result_stack);
	i2 = atoi(h);
	res = i1+i2;
	h = malloc(sizeof(char)*10);
        memset(h,0,10);
	sprintf(h, "%d", res);
	push_to_stack(&configuration->result_stack, h);
	return pending+1;
    }
    if (!strncmp(prog, "OP_SUB", 6)) {
	int i1, i2, res;
	char *h;
	DEBUGMSG(2, "---to do: OP_SUB <%s>\n", prog+7);
	h = pop_from_stack(&configuration->result_stack);
	i1 = atoi(h);
	h = pop_from_stack(&configuration->result_stack);
	i2 = atoi(h);
	res = i2-i1;
	h = malloc(sizeof(char)*10);
        memset(h,0,10);
	sprintf(h, "%d", res);
	push_to_stack(&configuration->result_stack, h);
	return pending+1;
    }
    if (!strncmp(prog, "OP_MULT", 7)) {
	int i1, i2, res;
	char *h;
	DEBUGMSG(2, "---to do: OP_MULT <%s>\n", prog+8);
	h = pop_from_stack(&configuration->result_stack);
	i1 = atoi(h);
	h = pop_from_stack(&configuration->result_stack);
	i2 = atoi(h);
	res = i1*i2;
	h = malloc(sizeof(char)*10);
        memset(h,0,10);
	sprintf(h, "%d", res);
	push_to_stack(&configuration->result_stack, h);
	return pending+1;
    }
    if(!strncmp(prog, "OP_CALL", 7)){
	char *h;
	int i, offset;
	char name[5];
        h = pop_from_stack(&configuration->result_stack);
	int num_params = atoi(h);
        memset(dummybuf, 0, sizeof(dummybuf));
	sprintf(dummybuf, "CLOSURE(OP_FOX);RESOLVENAME(@op(");///x(/y y x 2 op)));TAILAPPLY";
        offset = strlen(dummybuf);	
	for(i = 0; i < num_params; ++i){
	    sprintf(name, "x%d", i);
	    offset += sprintf(dummybuf+offset, "@%s(", name);
	}
	for(i =  num_params - 1; i >= 0; --i){
	    sprintf(name, "x%d", i);
	    offset += sprintf(dummybuf+offset, " %s", name);
	}
        offset += sprintf(dummybuf + offset, " %d", num_params);
	offset += sprintf(dummybuf+offset, " op");
	for(i = 0; i < num_params+2; ++i){
            offset += sprintf(dummybuf + offset, ")");
	}
        offset += sprintf(dummybuf + offset, ";TAILAPPLY");
	return strdup(dummybuf);
    }
    if(!strncmp(prog, "OP_FOX", 6)){
        char *h = pop_from_stack(&configuration->result_stack);
        int num_params = atoi(h);
        int i;
        char **params = malloc(sizeof(char * ) * num_params); 
        
        for(i = 0; i < num_params; ++i){ //pop parameter from stack
            params[i] = pop_from_stack(&configuration->result_stack);
        }
        
        //as long as there is a routable parameter: try to find a result
        for(i = num_params - 1; i >= 0; --i){
            if(iscontent(params[i])){
                struct ccnl_content_s *c = ccnl_nfn_handle_routable_content(ccnl, 
                        configuration, i, params, num_params, thunk_request);
                if(c){
                    push_to_stack(&configuration->result_stack, c->content);
                    goto tail;
                }
            }//endif
        }//endfor
        
        push_to_stack(&configuration->result_stack, "42");
        
        tail:
        return pending+1;
    }
    
    if(!strncmp(prog, "halt", 4)){
	*halt = 1;
        //FIXME: create string from stack, allow particular reduced
        return configuration->result_stack->content;
    }

    printf("unknown built-in command <%s>\n", prog);

    return 0;
}


//----------------------------------------------------------------
void
setup_global_environment(struct environment_s **env){

    struct closure_s *closure = new_closure("RESOLVENAME(@x@y x)", NULL);
    add_to_environment(env, "true", closure);

    closure = new_closure("RESOLVENAME(@x@y y)", NULL);
    add_to_environment(env, "false", closure);

    closure = new_closure("CLOSURE(OP_CMPEQ);RESOLVENAME(@op(@x(@y x y op)))", NULL);
    add_to_environment(env, "eq", closure);

    closure = new_closure("CLOSURE(OP_CMPLEQ);RESOLVENAME(@op@x@y x y op)", NULL);
    add_to_environment(env, "leq", closure);

    closure = new_closure("RESOLVENAME(@expr@yes@no(expr yes no))", NULL);
    add_to_environment(env, "ifelse", closure);

    closure = new_closure("CLOSURE(OP_ADD);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY", NULL);
    add_to_environment(env, "add", closure);

    closure = new_closure("CLOSURE(OP_SUB);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY", NULL);
    add_to_environment(env, "sub", closure);
   
    closure = new_closure("CLOSURE(OP_MULT);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY", NULL);
    add_to_environment(env, "mult", closure);

    closure = new_closure("RESOLVENAME(@f@n (ifelse (leq n 1) 1 (mult n ((f f)(sub n 1))) ))", NULL);
    add_to_environment(env, "factprime", closure);

    closure = new_closure("RESOLVENAME(factprime factprime)", NULL);
    add_to_environment(env, "fact", closure);

    closure = new_closure("CLOSURE(OP_CALL);RESOLVENAME(@op(@x x op));TAILAPPLY", NULL);
    add_to_environment(env, "call", closure);
}

char *
Krivine_reduction(struct ccnl_relay_s *ccnl, char *expression, int thunk_request, 
        int *num_of_required_thunks, struct ccnl_prefix_s *original_prefix, 
        struct thread_s *thread){
    int steps = 0; 
    int halt = 0;
    int len = strlen("CLOSURE(halt);RESOLVENAME()") + strlen(expression);
    char *prog;
    struct environment_s *global_dict = NULL;
    char *dummybuf = malloc(2000);
    setup_global_environment(&global_dict);
    if(strlen(expression) == 0) return 0;
    prog = malloc(len*sizeof(char));
    sprintf(prog, "CLOSURE(halt);RESOLVENAME(%s)", expression);
    struct configuration_s *config = new_config(prog, global_dict);
    config->thread = thread;
    
    while (config->prog && !halt && config->prog != 1){
	steps++;
	DEBUGMSG(1, "Step %d: %s\n", steps, config->prog);
	config->prog = ZAM_term(ccnl, config, thunk_request, 
                num_of_required_thunks, original_prefix, &halt, dummybuf);
    }
    free(dummybuf);
    return config->result_stack->content;
}