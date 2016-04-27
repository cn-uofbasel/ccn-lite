/*
 * @f ccnl-ext-nfnkrivine.c
 * @b CCN-lite, Krivine's lazy Lambda-Calculus reduction engine
 *
 * Copyright (C) 2013-14, Christian Tschudin, University of Basel
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
 * 2013-05-10 created
 * 2014-07-31 CCN-lite integration <christopher.scherb@unibas.ch>
 */


#ifndef CCNL_LINUXKERNEL

#include "ccnl-os-includes.h"

enum { // abstract machine instruction set
    ZAM_UNKNOWN,
    ZAM_ACCESS,
    ZAM_APPLY,
    ZAM_CALL,
    ZAM_CLOSURE,
    ZAM_FOX,
    ZAM_GRAB,
    ZAM_HALT,
    ZAM_RESOLVENAME,
    ZAM_TAILAPPLY
};

// ------------------------------------------------------------------
// Machine state functions

struct closure_s *
new_closure(char *term, struct environment_s *env)
{
    struct closure_s *ret = ccnl_calloc(1, sizeof(struct closure_s));

    ret->term = term;
    ret->env = env;
    ccnl_nfn_reserveEnvironment(env);

    return ret;
}

void
push_to_stack(struct stack_s **top, void *content, int type)
{
    struct stack_s *h;

    if (!top)
        return;

    h = ccnl_calloc(1, sizeof(struct stack_s));
    if (h) {
        DEBUGMSG(TRACE, "push_to_stack: %p\n", (void*)h);
        h->next = *top;
        h->content = content;
        h->type = type;
        *top = h;
    }
}

struct stack_s *
pop_from_stack(struct stack_s **top)
{
    struct stack_s *res;

    if (top == NULL || *top == NULL)
        return NULL;
    res = *top;
    *top = res->next;
    res->next = NULL;
    return res;
}

struct stack_s *
pop_or_resolve_from_result_stack(struct ccnl_relay_s *ccnl,
                                 struct configuration_s *config)
                                 // , int *restart)
{
    struct stack_s *elm = pop_from_stack(&config->result_stack);

    if (!elm)
        return NULL;
    DEBUGMSG(TRACE, "pop_or_resolve_from_RS: conf=%p, elm=%p, type=%d\n",
             (void*)config, (void*)elm, elm->type);
    return elm;
}

int
stack_len(struct stack_s *s)
{
    int cnt;

    for (cnt = 0; s; s = s->next)
            cnt++;
    return cnt;
}

// #ifdef XXX
void
print_environment(struct environment_s *env)
{
   int num = 0;

   printf("  env addr %p (refcount=%d)\n",
          (void*) env, env ? env->refcount : -1);
   while (env) {
       printf("  #%d %s %p\n", num++, env->name, (void*) env->closure);
       env = env->next;
   }
}

void
print_result_stack(struct stack_s *stack)
{
   int num = 0;

   while (stack) {
       printf("Res element #%d: type=%d\n", num++, stack->type);
       stack = stack -> next;
   }
}

void
print_argument_stack(struct stack_s *stack)
{
   int num = 0;

   while (stack){
        struct closure_s *c = stack->content;
        printf("Arg element #%d: %s\n", num++, c ? c->term : "<mark>");
        if (c)
            print_environment(c->env);
        stack = stack->next;
   }
}
// #endif

void
add_to_environment(struct environment_s **env, char *name,
                   struct closure_s *closure)
{
    struct environment_s *e;

    if (!env)
        return;

    e = ccnl_calloc(1, sizeof(struct environment_s));
    e->name = name;
    e->closure = closure;
    e->next = *env;
    *env = e;
    ccnl_nfn_reserveEnvironment(*env);
}

struct closure_s*
search_in_environment(struct environment_s *env, char *name)
{
    for (; env; env = env->next)
        if (!strcmp(name, env->name))
            return env->closure;

    return NULL;
}

int
iscontentname(char *cp)
{
    return cp[0] == '/';
}

// ----------------------------------------------------------------------

//choose the it_routable_param
int choose_parameter(struct configuration_s *config){

    int num, param_to_choose = config->fox_state->it_routable_param;

    DEBUGMSG(DEBUG, "choose_parameter(%d)\n", param_to_choose);
    for(num = config->fox_state->num_of_params-1; num >=0; --num){
        if(config->fox_state->params[num]->type == STACK_TYPE_PREFIX){
            --param_to_choose;
            if(param_to_choose <= 0){
                return num;
            }
        }
    }
    return -1;

}

struct ccnl_prefix_s *
create_namecomps(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                 int parameter_number, struct ccnl_prefix_s *prefix)
{
    DEBUGMSG(TRACE, "create_namecomps\n");
    if (config->start_locally ||
                ccnl_nfn_local_content_search(ccnl, config, prefix)) {
        //local computation name components
        DEBUGMSG(DEBUG, "content local available\n");
        struct ccnl_prefix_s *pref = ccnl_nfnprefix_mkComputePrefix(config, prefix->suite);
        DEBUGMSG(DEBUG, "LOCAL PREFIX: %s\n", ccnl_prefix_to_path(pref));
        return pref;
    }
    return ccnl_nfnprefix_mkCallPrefix(prefix, config, parameter_number);
}

// ----------------------------------------------------------------------

// return a positive integer if ZAM instuction, or
// return a negative integer if a builtin op, otherwise return 0
int
ZAM_nextToken(char *prog, char **argdup, char **cont)
{
    char *cp = prog, *cp2;
    int len;
    int token = ZAM_UNKNOWN;

    // TODO: count opening/closing parentheses when hunting for ';' ?
    while (*cp && *cp != '(' && *cp != ';')
        cp++;
    len = cp - prog;

    if (!strncmp(prog, "ACCESS", len))
        token = ZAM_ACCESS;
    else if (!strncmp(prog, "APPLY", len))
        token = ZAM_APPLY;
    else if (!strncmp(prog, "CALL", len))
        token = ZAM_CALL;
    else if (!strncmp(prog, "CLOSURE", len))
        token = ZAM_CLOSURE;
    else if (!strncmp(prog, "FOX", len))
        token = ZAM_FOX;
    else if (!strncmp(prog, "GRAB", len))
        token = ZAM_GRAB;
    else if (!strncmp(prog, "HALT", len))
        token = ZAM_HALT;
    else if (!strncmp(prog, "RESOLVENAME", len))
        token = ZAM_RESOLVENAME;
    else if (!strncmp(prog, "TAILAPPLY", len))
        token = ZAM_TAILAPPLY;
    else {
        for (len = 0; bifs[len].name; len++)
            if (!strncmp(prog, bifs[len].name, strlen(bifs[len].name))) {
                token = -(len+1);
                break;
            }
        if (token == ZAM_UNKNOWN)
            DEBUGMSG(INFO, "  ** DID NOT find %s\n", prog);
    }

    if (*cp == '(') {
        int nesting = 0;
        cp2 = ++cp;
        while (*cp2 && (*cp2 != ')' || nesting > 0)) {
            if (*cp2 == '(')
                nesting++;
            else if (*cp2 == ')')
                nesting--;
            cp2++;
        }
        len = cp2 - cp;
        *argdup = ccnl_malloc(len + 1);
        memcpy(*argdup, cp, len);
        (*argdup)[len] = '\0';
        if (*cp2)
            cp = cp2 + 1;
    } else
        *argdup = NULL;

    if (*cp == ';')
        *cont = cp + 1;
    else
        *cont = NULL;
    return token;
}

char*
ZAM_fox(struct ccnl_relay_s *ccnl, struct configuration_s *config,
        int *restart, int *halt, char *prog, char *arg, char *contd)
{
    int local_search = 0, i;
    int parameter_number = 0;
    struct ccnl_content_s *c = NULL;
    struct ccnl_prefix_s *pref;
    struct ccnl_interest_s *interest;

    DEBUGMSG(DEBUG, "---to do: FOX <%s>\n", arg);
    ccnl_free(arg);
    if (*restart) {
        *restart = 0;
        local_search = 1;
        goto recontinue;
    }

    {
        struct stack_s *h;
        h = pop_or_resolve_from_result_stack(ccnl, config);
        assert(h);
        //TODO CHECK IF INT
        config->fox_state->num_of_params = *(int*)h->content;
        h->next = NULL;
        ccnl_nfn_freeStack(h);
    }
    DEBUGMSG(DEBUG, "NUM OF PARAMS: %d\n", config->fox_state->num_of_params);

    config->fox_state->params = ccnl_malloc(sizeof(struct ccnl_stack_s *) *
                                            config->fox_state->num_of_params);

    for (i = 0; i < config->fox_state->num_of_params; ++i) {
        //pop parameter from stack
        config->fox_state->params[i] = pop_from_stack(&config->result_stack);
        switch (config->fox_state->params[i]->type) {
        case STACK_TYPE_INT:
            DEBUGMSG(DEBUG, "  info: Parameter %d %d\n", i,
                     *(int *)config->fox_state->params[i]->content);
            break;
        case STACK_TYPE_PREFIX:
            DEBUGMSG(DEBUG, "  info: Parameter %d %s\n", i,
                     ccnl_prefix_to_path((struct ccnl_prefix_s*)
                                      config->fox_state->params[i]->content));
            break;
        default:
            break;
        }
    }
    //as long as there is a routable parameter: try to find a result
    config->fox_state->it_routable_param = 0;

    //check if last result is now available
recontinue: //loop by reentering after timeout of the interest...
    if (local_search) {
        DEBUGMSG(DEBUG, "Checking if result was received\n");
        parameter_number = choose_parameter(config);
        pref = create_namecomps(ccnl, config, parameter_number,
                        config->fox_state->params[parameter_number]->content);
        // search for a result
        c = ccnl_nfn_local_content_search(ccnl, config, pref);
        set_propagate_of_interests_to_1(ccnl, pref);
        free_prefix(pref);
        //TODO Check? //TODO remove interest here?
        if (c) {
	    DEBUGMSG(DEBUG, "Result was found\n");
            DEBUGMSG_CFWD(INFO, "data after result was found %.*s\n", c->pkt->contlen, c->pkt->content);
            goto handlecontent;
	}
    }

    //result was not delivered --> choose next parameter
    ++config->fox_state->it_routable_param;
    parameter_number = choose_parameter(config);
    if (parameter_number < 0)
        //no more parameter --> no result found, can try a local computation
        goto local_compute;
    // create new prefix with name components!!!!
    pref = create_namecomps(ccnl, config, parameter_number,
                        config->fox_state->params[parameter_number]->content);
    c = ccnl_nfn_local_content_search(ccnl, config, pref);
    if (c) {
        free_prefix(pref);
        goto handlecontent;
    }

    // Result not in cache, search over the network
//    pref2 = ccnl_prefix_dup(pref);
    interest = ccnl_nfn_query2interest(ccnl, &pref, config);
    if (pref)
        free_prefix(pref);
    if (interest) {
        ccnl_interest_propagate(ccnl, interest);
        DEBUGMSG(DEBUG, "  new interest's face is %d\n", interest->from->faceid);
    }
    // wait for content, return current program to continue later
    *halt = -1; //set halt to -1 for async computations
    return ccnl_strdup(prog);

local_compute:
    if (config->local_done)
        return NULL;

    config->local_done = 1;
    pref = ccnl_nfnprefix_mkComputePrefix(config, config->suite);
    DEBUGMSG(DEBUG, "Prefix local computation: %s\n",
             ccnl_prefix_to_path(pref));
    interest = ccnl_nfn_query2interest(ccnl, &pref, config);
    if (pref)
        free_prefix(pref);
    if (interest)
        ccnl_interest_propagate(ccnl, interest);

handlecontent: //if result was found ---> handle it
    if (c) {
#ifdef USE_NACK
        if (!strncmp((char*)c->pkt->content, ":NACK", 5)) {
            DEBUGMSG(DEBUG, "NACK RECEIVED, going to next parameter\n");
            ++config->fox_state->it_routable_param;
            return prog ? ccnl_strdup(prog) : NULL;
        }
#endif
        int isANumber = 1, i = 0;
	    for(i = 0; i < c->pkt->contlen; ++i){
	    	if(!isdigit(c->pkt->content[i])){
                isANumber = 0;
                break;
            }
	    }
	    
        if (isANumber){
            int *integer = ccnl_malloc(sizeof(int));
            *integer = strtol((char*)c->pkt->content, 0, 0);
            push_to_stack(&config->result_stack, integer, STACK_TYPE_INT);
        } else {
            struct prefix_mapping_s *mapping;
            struct ccnl_prefix_s *name =
                create_prefix_for_content_on_result_stack(ccnl, config);
            push_to_stack(&config->result_stack, name, STACK_TYPE_PREFIX);
            mapping = ccnl_malloc(sizeof(struct prefix_mapping_s));
            mapping->key = ccnl_prefix_dup(name); //TODO COPY
            mapping->value = ccnl_prefix_dup(c->pkt->pfx);
            DBL_LINKED_LIST_ADD(config->fox_state->prefix_mapping, mapping);
            DEBUGMSG(DEBUG, "Created a mapping %s - %s\n",
                        ccnl_prefix_to_path(mapping->key),
                        ccnl_prefix_to_path(mapping->value));
        }
    }
    
    DEBUGMSG(DEBUG, " FOX continuation: %s\n", contd);
    return ccnl_strdup(contd);
}

char*
ZAM_resolvename(struct configuration_s *config, char *dummybuf,
                char *arg, char *contd)
{
    struct ccnl_lambdaTerm_s *t;
    char res[1000], *cp = arg;
    int len;

    DEBUGMSG(DEBUG, "---to do: resolveNAME <%s>\n", arg);

    //function definition
    if (!strncmp(cp, "let", 3)) {
        int i, end = 0, cp2len, namelength, lambdalen;
        char *h, *cp2, *name, *lambda_expr, *resolveterm;

        DEBUGMSG(DEBUG, " fct definition: %s\n", cp);
        strcpy(res, cp+3);
        for (i = 0; i < strlen(res); ++i) {
            if (!strncmp(res+i, "endlet", 6)) {
                end = i;
                break;
            }
        }
        cp2len = strlen(res+end) + strlen("RESOLVENAME()");
        h = strchr(cp, '=');
        namelength = h - cp;

        lambda_expr = ccnl_malloc(strlen(h));
        name = ccnl_malloc(namelength);
        cp2 = ccnl_malloc(cp2len);

        memset(cp2, 0, cp2len);
        memset(name, 0, namelength);
        memset(lambda_expr, 0, strlen(h));

        sprintf(cp2, "RESOLVENAME(%s)", res+end+7); //add 7 to overcome endlet
        memcpy(name, cp+3, namelength-3); //copy name without let and endlet
        trim(name);

        lambdalen = strlen(h)-strlen(cp2)+11-6;
        memcpy(lambda_expr, h+1, lambdalen); //copy lambda expression without =
        trim(lambda_expr);
        resolveterm = ccnl_malloc(strlen("RESOLVENAME()")+strlen(lambda_expr));
        sprintf(resolveterm, "RESOLVENAME(%s)", lambda_expr);

        add_to_environment(&config->env, name, new_closure(resolveterm, NULL));

        ccnl_free(cp);
        return strdup(contd);
    }

    //check if term can be made available, if yes enter it as a var
    //try with searching in global env for an added term!

    t = ccnl_lambdaStrToTerm(0, &cp, NULL);
    ccnl_free(arg);

    if (term_is_var(t)) {
        char *end = 0;
        cp = t->v;
        if (isdigit(*cp)) {
            // is disgit...
            int *integer = ccnl_malloc(sizeof(int));
            *integer = strtol(cp, &end, 0);
            if (end && *end)
                end = 0;
            if (end)
                push_to_stack(&config->result_stack, integer, STACK_TYPE_INT);
            else
                ccnl_free(integer);
        } else if (*cp == '\'') { // quoted name (constant)
            //determine size
            struct const_s *con = ccnl_nfn_krivine_str2const(cp);
            push_to_stack(&config->result_stack, con, STACK_TYPE_CONST);
            end = (char*)1;
        } else if (iscontentname(cp)) { // is content...
            struct ccnl_prefix_s *prefix;
            prefix = ccnl_URItoPrefix(cp, config->suite, NULL, NULL);
            push_to_stack(&config->result_stack, prefix, STACK_TYPE_PREFIX);
            end = (char*)1;
        }
        if (end) {
            if (contd)
                sprintf(res, "TAILAPPLY;%s", contd);
            else
                sprintf(res, "TAILAPPLY");
        } else {
            if (contd)
                sprintf(res, "ACCESS(%s);TAILAPPLY;%s", t->v, contd);
            else
                sprintf(res, "ACCESS(%s);TAILAPPLY", t->v);
        }
        ccnl_lambdaFreeTerm(t);
        return ccnl_strdup(res);
    }
    if (term_is_lambda(t)) {
        char *var;
        var = t->v;
        ccnl_lambdaTermToStr(dummybuf, t->m, 0);
        if (contd)
            sprintf(res, "GRAB(%s);RESOLVENAME(%s);%s", var, dummybuf, contd);
        else
            sprintf(res, "GRAB(%s);RESOLVENAME(%s)", var, dummybuf);
        ccnl_lambdaFreeTerm(t);
        return ccnl_strdup(res);
    }
    if (term_is_app(t)) {
        ccnl_lambdaTermToStr(dummybuf, t->n, 0);
        len = sprintf(res, "CLOSURE(RESOLVENAME(%s));", dummybuf);
        ccnl_lambdaTermToStr(dummybuf, t->m, 0);
        len += sprintf(res+len, "RESOLVENAME(%s)", dummybuf);
        if (contd)
            len += sprintf(res+len, ";%s", contd);
        ccnl_lambdaFreeTerm(t);
        return ccnl_strdup(res);
    }
    return NULL;
}

// executes a ZAM instruction, returns the term to continue working on
char*
ZAM_term(struct ccnl_relay_s *ccnl, struct configuration_s *config,
        int *halt, char *dummybuf, int *restart)
{
//    struct ccnl_lambdaTerm_s *t;
//    char *pending, *p, *cp, *prog = config->prog;
//    int len;

    char *prog = config->prog;
    struct builtin_s *bp;
    char *arg, *contd;
    int tok;

    //pop closure
    if (!prog || strlen(prog) == 0) {
         if (config->result_stack) {
             prog = ccnl_malloc(strlen((char*)config->result_stack->content)+1);
             strcpy(prog, config->result_stack->content);
             return prog;
         }
         DEBUGMSG(DEBUG, "no result returned\n");
         return NULL;
    }

    tok = ZAM_nextToken(prog, &arg, &contd);

    // TODO: count opening/closing parentheses when hunting for ';' ?
/*
    pending = strchr(prog, ';');
    p = strchr(prog, '(');
*/

    switch (tok) {
    case ZAM_ACCESS:
    {
        struct closure_s *closure = search_in_environment(config->env, arg);
        DEBUGMSG(DEBUG, "---to do: access <%s>\n", arg);
        if (!closure) {
            // TODO: is the following needed? Above search should have
            // visited global_dict, already!
            closure = search_in_environment(config->global_dict, arg);
            if (!closure) {
                DEBUGMSG(WARNING, "?? could not lookup var %s\n", arg);
                ccnl_free(arg);
                return NULL;
            }
        }
        ccnl_free(arg);
        closure = new_closure(ccnl_strdup(closure->term), closure->env);
        push_to_stack(&config->argument_stack, closure, STACK_TYPE_CLOSURE);
        return ccnl_strdup(contd);
    }
    case ZAM_APPLY:
    {
        struct stack_s *fct = pop_from_stack(&config->argument_stack);
        struct stack_s *par = pop_from_stack(&config->argument_stack);
        struct closure_s *fclosure, *aclosure;
        char *code;
        DEBUGMSG(DEBUG, "---to do: apply\n");

        if (!fct || !par)
            return NULL;
        fclosure = (struct closure_s *) fct->content;
        aclosure = (struct closure_s *) par->content;
        if (!fclosure || !aclosure)
            return NULL;
        ccnl_free(fct);
        ccnl_free(par);

        code = aclosure->term;
        if (config->env)
            ccnl_nfn_releaseEnvironment(&config->env);
        config->env = aclosure->env;
        ccnl_free(aclosure);
        push_to_stack(&config->argument_stack, fclosure, STACK_TYPE_CLOSURE);

        if (contd)
            sprintf(dummybuf, "%s;%s", code, contd);
        else
            strcat(dummybuf, code);
        ccnl_free(code);
        return ccnl_strdup(dummybuf);
    }
    case ZAM_CALL:
    {
        struct stack_s *h = pop_or_resolve_from_result_stack(ccnl, config);
        int i, offset, num_params = *(int *)h->content;
        char name[5];

        DEBUGMSG(DEBUG, "---to do: CALL <%s>\n", arg);
        ccnl_free(h->content);
        ccnl_free(h);
        sprintf(dummybuf, "CLOSURE(FOX);RESOLVENAME(@op(");
	// ... @x(@y y x 2 op)));TAILAPPLY";
        offset = strlen(dummybuf);
        for (i = 0; i < num_params; ++i) {
            sprintf(name, "x%d", i);
            offset += sprintf(dummybuf+offset, "@%s(", name);
        }
        for (i = num_params - 1; i >= 0; --i) {
            sprintf(name, "x%d", i);
            offset += sprintf(dummybuf+offset, " %s", name);
        }
        offset += sprintf(dummybuf + offset, " %d", num_params);
        offset += sprintf(dummybuf+offset, " op");
        for (i = 0; i < num_params+2; ++i)
            offset += sprintf(dummybuf + offset, ")");
        if (contd)
            sprintf(dummybuf + offset, ";%s", contd);
        return ccnl_strdup(dummybuf);
    }
    case ZAM_CLOSURE:
    {
        struct closure_s *closure;
        DEBUGMSG(DEBUG, "---to do: closure <%s> (contd=%s)\n", arg, contd);

        if (!config->argument_stack && !strncmp(arg, "RESOLVENAME(", 12)) {
            char v[500], *c;
            int len;
            c = strchr(arg+12, ')');
            if (!c)
                goto normal;
            len = c - (arg+12);
            memcpy(v, arg+12, len);
            v[len] = '\0';
            closure = search_in_environment(config->env, v);
            if (!closure)
                goto normal;
            if (!strcmp(closure->term, arg)) {
                DEBUGMSG(WARNING, "** detected tail recursion case %s/%s\n",
                         closure->term, arg);
            } else
                goto normal;
        } else {
normal:
            closure = new_closure(arg, config->env);
            //configuration->env = NULL;//FIXME new environment?
            push_to_stack(&config->argument_stack, closure, STACK_TYPE_CLOSURE);
            arg = NULL;
        }
        if (contd) {
            ccnl_free(arg);
            return ccnl_strdup(contd);
        }
        DEBUGMSG(ERROR, "** not implemented, see line %d\n", __LINE__);
        return arg;
    }
    case ZAM_FOX:
        return ZAM_fox(ccnl, config, restart, halt, prog, arg, contd);
    case ZAM_GRAB:
    {
        struct stack_s *stack = pop_from_stack(&config->argument_stack);
        DEBUGMSG(DEBUG, "---to do: grab <%s>\n", arg);
        add_to_environment(&config->env, arg, stack->content);
        ccnl_free(stack);
        return ccnl_strdup(contd);
    }
    case ZAM_HALT:
        ccnl_nfn_freeStack(config->argument_stack);
        //ccnl_nfn_freeStack(config->result_stack);
        config->argument_stack = /*config->result_stack =*/ NULL;
        *halt = 1;
        return ccnl_strdup(contd);
    case ZAM_RESOLVENAME:
        return ZAM_resolvename(config, dummybuf, arg, contd);
    case ZAM_TAILAPPLY:
    {
        struct stack_s *stack = pop_from_stack(&config->argument_stack);
        struct closure_s *closure = (struct closure_s *) stack->content;
        char *code = closure->term;
        DEBUGMSG(DEBUG, "---to do: tailapply\n");

        ccnl_free(stack);
        if (contd) //build new term
            sprintf(dummybuf, "%s;%s", code, contd);
        else
            strcpy(dummybuf, code);
        if (config->env)
            ccnl_nfn_releaseEnvironment(&config->env);
        config->env = closure->env; //set environment from closure
        ccnl_free(code);
        ccnl_free(closure);
        return ccnl_strdup(dummybuf);
    }
    case ZAM_UNKNOWN:
        break;
    default:
        DEBUGMSG(DEBUG, "builtin: %s (%s/%s)\n", bifs[-tok - 1].name, prog, contd);
        return bifs[-tok - 1].fct(ccnl, config, restart, halt, prog,
                                  contd, &config->result_stack);
    }

    ccnl_free(arg);

    // iterate through all extension operations
    for (bp = op_extensions; bp; bp = bp->next)
        if (!strncmp(prog, bp->name, strlen(bp->name)))
            return (bp->fct)(ccnl, config, restart, halt, prog,
                             contd, &config->result_stack);

    DEBUGMSG(INFO, "unknown (built-in) command <%s>\n", prog);

    return NULL;
}


//----------------------------------------------------------------

void
allocAndAdd(struct environment_s **env, char *name, char *cmd)
{
    struct closure_s *closure;

    closure = new_closure(ccnl_strdup(cmd), NULL);
    add_to_environment(env, ccnl_strdup(name), closure);
}

void
setup_global_environment(struct environment_s **env)
{
    //Operator on Church numbers
    allocAndAdd(env, "true_church",
                "RESOLVENAME(@x@y x)");
    allocAndAdd(env, "false_church",
                "RESOLVENAME(@x@y y)");
    allocAndAdd(env, "eq_church",
                "CLOSURE(OP_CMPEQ_CHURCH);RESOLVENAME(@op(@x(@y x y op)))");
    allocAndAdd(env, "leq_church",
                "CLOSURE(OP_CMPLEQ_CHURCH);RESOLVENAME(@op(@x(@y x y op)))");
    allocAndAdd(env, "ifelse_church",
                "RESOLVENAME(@expr@yes@no(expr yes no))");

    //Operator on integer numbers
    allocAndAdd(env, "eq",
                "CLOSURE(OP_CMPEQ);RESOLVENAME(@op(@x(@y x y op)))");
    allocAndAdd(env, "leq",
                "CLOSURE(OP_CMPLEQ);RESOLVENAME(@op(@x(@y x y op)))");
    allocAndAdd(env, "ifelse",
                "CLOSURE(OP_IFELSE);RESOLVENAME(@op(@x x op));TAILAPPLY");
    allocAndAdd(env, "add",
                "CLOSURE(OP_ADD);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY");
    allocAndAdd(env, "sub",
                "CLOSURE(OP_SUB);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY");
    allocAndAdd(env, "mult",
                "CLOSURE(OP_MULT);RESOLVENAME(@op(@x(@y x y op)));TAILAPPLY");

    allocAndAdd(env, "call",
                "CLOSURE(CALL);RESOLVENAME(@op(@x x op));TAILAPPLY");

    allocAndAdd(env, "raw", "TAILAPPLY;OP_RAW");

#ifdef USE_NFN_NSTRANS
    allocAndAdd(env, "translate",
                "CLOSURE(OP_NSTRANS);CLOSURE(OP_FIND);"
                "RESOLVENAME(@of(@o1(@x(@y x y o1 of))));TAILAPPLY");
#endif
}

// consumes the result stack, exports its content to a buffer
struct ccnl_buf_s*
Krivine_exportResultStack(struct ccnl_relay_s *ccnl,
                          struct configuration_s *config)
{
    char res[64000]; //TODO longer???
    int pos = 0;
    struct stack_s *stack;
    struct ccnl_content_s *cont;

    while ((stack = pop_or_resolve_from_result_stack(ccnl, config))) {
        if (stack->type == STACK_TYPE_PREFIX) {
            cont = ccnl_nfn_local_content_search(ccnl, config, stack->content);
            if (cont) {
                memcpy(res+pos, (char*)cont->pkt->content, cont->pkt->contlen);
                pos += cont->pkt->contlen;
            }
        } else if (stack->type == STACK_TYPE_PREFIXRAW) {
            cont = ccnl_nfn_local_content_search(ccnl, config, stack->content);
            if (cont) {
/*
                DEBUGMSG(DEBUG, "  PREFIXRAW packet: %p %d\n", (void*) cont->buf,
                         cont->buf ? cont->buf->datalen : -1);
*/
                memcpy(res+pos, (char*)cont->pkt->buf->data,
                       cont->pkt->buf->datalen);
                pos += cont->pkt->buf->datalen;
            }
        } else if (stack->type == STACK_TYPE_INT) {
            //h = ccnl_buf_new(NULL, 10);
            //sprintf((char*)h->data, "%d", *(int*)stack->content);
            //h->datalen = strlen((char*)h->data);
            pos += sprintf(res + pos, "%d", *(int*)stack->content);
        }
        ccnl_free(stack->content);
        ccnl_free(stack);
    }

    return ccnl_buf_new(res, pos);
}

// executes (loops over) a Lambda expression: tries to run to termination,
// or returns after having emitted further remote lookup/reduction requests
// the return val is a buffer with the result stack's (concatenated) content
struct ccnl_buf_s*
Krivine_reduction(struct ccnl_relay_s *ccnl, char *expression,
                  int start_locally,
                  struct configuration_s **config,
                  struct ccnl_prefix_s *prefix, int suite)
{
    int steps = 0, halt = 0, restart = 1;
    int len = strlen("CLOSURE(HALT);RESOLVENAME()") + strlen(expression) + 1;
    char *dummybuf;

    DEBUGMSG(TRACE, "Krivine_reduction()\n");

    if (!*config && strlen(expression) == 0)
        return 0;
    dummybuf = ccnl_malloc(2000);
    if (!*config) {
        char *prog;
        struct environment_s *global_dict = NULL;

        prog = ccnl_malloc(len*sizeof(char));
        sprintf(prog, "CLOSURE(HALT);RESOLVENAME(%s)", expression);
        setup_global_environment(&global_dict);
        DEBUGMSG(DEBUG, "PREFIX %s\n", ccnl_prefix_to_path(prefix));
        *config = new_config(ccnl, prog, global_dict,
                             start_locally,
                             prefix, ccnl->km->configid, suite);
        DBL_LINKED_LIST_ADD(ccnl->km->configuration_list, (*config));
        restart = 0;
        --ccnl->km->configid;
    }
    DEBUGMSG(INFO, "Prog: %s\n", (*config)->prog);

    while ((*config)->prog && !halt) {
        char *oldprog = (*config)->prog;
        steps++;
        DEBUGMSG(DEBUG, "Step %d (%d/%d): %s\n", steps,
                 stack_len((*config)->argument_stack),
                 stack_len((*config)->result_stack), (*config)->prog);
        (*config)->prog = ZAM_term(ccnl, *config, &halt, dummybuf, &restart);
        ccnl_free(oldprog);
    }
    ccnl_free(dummybuf);

    if (halt < 0) { //HALT < 0 means pause computation
        DEBUGMSG(INFO,"Pause computation: %d\n", -(*config)->configid);
        return NULL;
    }

    //HALT > 0 means computation finished
    DEBUGMSG(INFO, "end-of-computation (%d/%d)\n",
             stack_len((*config)->argument_stack),
             stack_len((*config)->result_stack));
/*
    print_argument_stack((*config)->argument_stack);
    print_result_stack((*config)->result_stack);
*/

    return Krivine_exportResultStack(ccnl, *config);
}

#endif // !CCNL_LINUXKERNEL

// eof
