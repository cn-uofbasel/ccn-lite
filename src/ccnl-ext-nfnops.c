/*
 * @f ccnl-ext-nfnops.c
 * @b CCN-lite, builtin operations for Krivine's lazy machine
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-11-01: created
 */

// binds the name to the given fct in ZAM's list of known operations
void
ZAM_registerOp(char *name, BIF fct)
{
    struct builtin_s *b;
    struct ccnl_buf_s *buf;

    buf = ccnl_calloc(1, sizeof(*buf) + sizeof(*b));
    ccnl_core_addToCleanup(buf);

    b = (struct builtin_s*) buf->data;
    b->name = name;
    b->fct = fct;
    b->next = op_extensions;

    op_extensions = b;
}

//------------------------------------------------------------
/**
 * used to avoid code duplication for popping two integer values from result stack asynchronous
 * requires to define i1 and i2 before calling to store the results.
 */
#define pop2int() \
        do{     \
            struct stack_s *h1, *h2; \
            h1 = pop_or_resolve_from_result_stack(ccnl, config); \
            if(h1 == NULL){ \
                *halt = -1; \
                return prog; \
            } \
            if(h1->type == STACK_TYPE_INT) i1 = *(int*)h1->content;\
            h2 = pop_or_resolve_from_result_stack(ccnl, config); \
            if(h2 == NULL){ \
                *halt = -1; \
                push_to_stack(&config->result_stack, h1->content, h1->type); \
                ccnl_free(h1); \
                return prog; \
            } \
            if(h1->type == STACK_TYPE_INT) i2 = *(int*)h2->content;\
            ccnl_nfn_freeStack(h1); ccnl_nfn_freeStack(h2); \
        }while(0)

// ----------------------------------------------------------------------
// builtin operations

char*
op_builtin_add(struct ccnl_relay_s *ccnl, struct configuration_s *config,
               int *restart, int *halt, char *prog, char *pending,
               struct stack_s **stack)
{
    int i1=0, i2=0, *h;

    DEBUGMSG(DEBUG, "---to do: OP_ADD <%s> pending: %s\n", prog+7, pending);
    pop2int();
    h = ccnl_malloc(sizeof(int));
    *h = i1 + i2;
    push_to_stack(stack, h, STACK_TYPE_INT);

    return pending ? ccnl_strdup(pending) : NULL;
}

char*
op_builtin_find(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                int *restart, int *halt, char *prog, char *pending,
                struct stack_s **stack)
{
    int local_search = 0;
    struct stack_s *h;
    char *cp = NULL;
    struct ccnl_prefix_s *prefix;
    struct ccnl_content_s *c = NULL;

    if (*restart) {
        DEBUGMSG(DEBUG, "---to do: OP_FIND restart\n");
        *restart = 0;
        local_search = 1;
    } else {
        DEBUGMSG(DEBUG, "---to do: OP_FIND <%s> <%s>\n", prog+7, pending);
        h = pop_from_stack(&config->result_stack);
        //    if (h->type != STACK_TYPE_PREFIX)  ...
        config->fox_state->num_of_params = 1;
        config->fox_state->params = ccnl_malloc(sizeof(struct ccnl_stack_s *));
        config->fox_state->params[0] = h;
        config->fox_state->it_routable_param = 0;
    }
    prefix = config->fox_state->params[0]->content;

    //check if result is now available
    //loop by reentering (with local_search) after timeout of the interest...
    DEBUGMSG(DEBUG, "FIND: Checking if result was received\n");
    c = ccnl_nfn_local_content_search(ccnl, config, prefix);
    if (!c) {
        struct ccnl_prefix_s *copy;
        struct ccnl_interest_s *interest;
        if (local_search) {
            DEBUGMSG(INFO, "FIND: no content\n");
            return NULL;
        }
        //Result not in cache, search over the network
        //        struct ccnl_interest_s *interest = mkInterestObject(ccnl, config, prefix);
        copy = ccnl_prefix_dup(prefix);
        interest = ccnl_nfn_query2interest(ccnl, &copy, config);
        DEBUGMSG(DEBUG, "FIND: sending new interest from Face ID: %d\n",
                 interest->from->faceid);
        if (interest)
            ccnl_interest_propagate(ccnl, interest);
        //wait for content, return current program to continue later
        *halt = -1; //set halt to -1 for async computations
        return ccnl_strdup(prog);
    }

    DEBUGMSG(INFO, "FIND: result was found ---> handle it (%s), prog=%s, pending=%s\n", ccnl_prefix_to_path(prefix), prog, pending);
#ifdef USE_NACK
/*
    if (!strncmp((char*)c->content, ":NACK", 5)) {
        DEBUGMSG(DEBUG, "NACK RECEIVED, going to next parameter\n");
        ++config->fox_state->it_routable_param;

        return prog ? ccnl_strdup(prog) : NULL;
    }
*/
#endif
    prefix = ccnl_prefix_dup(prefix);
    push_to_stack(&config->result_stack, prefix, STACK_TYPE_PREFIX);

    if (pending) {
        DEBUGMSG(DEBUG, "Pending: %s\n", pending);

        cp = ccnl_strdup(pending);
    }
    return cp;
}

char*
op_builtin_mult(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                int *restart, int *halt, char *prog, char *pending,
                struct stack_s **stack)
{
    int i1=0, i2=0, *h;

    DEBUGMSG(DEBUG, "---to do: OP_MULT <%s>\n", prog+8);
    pop2int();
    h = ccnl_malloc(sizeof(int));
    *h = i2 * i1;
    push_to_stack(stack, h, STACK_TYPE_INT);

    return pending ? ccnl_strdup(pending) : NULL;
}

char*
op_builtin_raw(struct ccnl_relay_s *ccnl, struct configuration_s *config,
               int *restart, int *halt, char *prog, char *pending,
               struct stack_s **stack)
{
    int local_search = 0;
    struct stack_s *h;
    char *cp = NULL;
    struct ccnl_prefix_s *prefix;
    struct ccnl_content_s *c = NULL;

    //    print_argument_stack(config->argument_stack);
    //    print_result_stack(config->result_stack);
    if (*restart) {
        DEBUGMSG(DEBUG, "---to do: OP_RAW restart\n");
        *restart = 0;
        local_search = 1;
    } else {
        DEBUGMSG(DEBUG, "---to do: OP_RAW <%s>\n", prog+7);
        h = pop_from_stack(&config->result_stack);
        if (!h || h->type != STACK_TYPE_PREFIX) {
            DEBUGMSG(DEBUG, "  stack empty or has no prefix %p\n", (void*) h);
        } else {
            DEBUGMSG(DEBUG, "  found a prefix!\n");
        }
        config->fox_state->num_of_params = 1;
        config->fox_state->params = ccnl_malloc(sizeof(struct ccnl_stack_s *));
        config->fox_state->params[0] = h;
        config->fox_state->it_routable_param = 0;
    }
    prefix = config->fox_state->params[0]->content;

    //check if result is now available
    //loop by reentering (with local_search) after timeout of the interest...
    DEBUGMSG(DEBUG, "RAW: Checking if result was received\n");
    c = ccnl_nfn_local_content_search(ccnl, config, prefix);
    if (!c) {
        if (local_search) {
            DEBUGMSG(DEBUG, "RAW: no content\n");
            return NULL;
        }
        //Result not in cache, search over the network
        //        struct ccnl_interest_s *interest = mkInterestObject(ccnl, config, prefix);
        struct ccnl_prefix_s *copy = ccnl_prefix_dup(prefix);
        struct ccnl_interest_s *interest = ccnl_nfn_query2interest(ccnl, &copy, config);
        DEBUGMSG(DEBUG, "RAW: sending new interest from Face ID: %d\n", interest->from->faceid);
        if (interest)
            ccnl_interest_propagate(ccnl, interest);
        //wait for content, return current program to continue later
        *halt = -1; //set halt to -1 for async computations
        return prog ? ccnl_strdup(prog) : NULL;
    }

    DEBUGMSG(DEBUG, "RAW: result was found ---> handle it (%s), prog=%s, pending=%s\n", ccnl_prefix_to_path(prefix), prog, pending);
#ifdef USE_NACK
/*
    if (!strncmp((char*)c->content, ":NACK", 5)) {
        DEBUGMSG(DEBUG, "NACK RECEIVED, going to next parameter\n");
        ++config->fox_state->it_routable_param;

        return prog ? ccnl_strdup(prog) : NULL;
    }
*/
#endif
    prefix = ccnl_prefix_dup(prefix);
    push_to_stack(&config->result_stack, prefix, STACK_TYPE_PREFIXRAW);

    if (pending) {
        DEBUGMSG(DEBUG, "Pending: %s\n", pending+1);

        cp = ccnl_strdup(pending+1);
    }
    return cp;
}

char*
op_builtin_sub(struct ccnl_relay_s *ccnl, struct configuration_s *config,
               int *restart, int *halt, char *prog, char *pending,
               struct stack_s **stack)
{
    int i1=0, i2=0, *h;

    DEBUGMSG(DEBUG, "---to do: OP_SUB <%s>\n", prog+7);
    pop2int();
    h = ccnl_malloc(sizeof(int));
    *h = i2 - i1;
    push_to_stack(stack, h, STACK_TYPE_INT);

    return pending ? ccnl_strdup(pending) : NULL;
}

// ----------------------------------------------------------------------

char*
op_builtin_cmpeqc(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                  int *restart, int *halt, char *prog, char *pending,
                  struct stack_s **stack)
{
    int i1=0, i2=0;
    char res[1000], *cp;

    pop2int();
    cp = (i1 == i2) ? "@x@y x" : "@x@y y";
    DEBUGMSG(DEBUG, "---to do: OP_CMPEQ <%s>/<%s>\n", cp, pending);
    if (pending)
        sprintf(res, "RESOLVENAME(%s);%s", cp, pending);
    else
        sprintf(res, "RESOLVENAME(%s)", cp);
    return ccnl_strdup(res);
}

char*
op_builtin_cmpleqc(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                  int *restart, int *halt, char *prog, char *pending,
                  struct stack_s **stack)
{
    int i1=0, i2=0;
    char res[1000], *cp;

    pop2int();
    cp = (i2 <= i1) ? "@x@y x" : "@x@y y";
    DEBUGMSG(DEBUG, "---to do: OP_CMPLEQ <%s>/%s\n", cp, pending);
    if (pending)
        sprintf(res, "RESOLVENAME(%s);%s", cp, pending);
    else
        sprintf(res, "RESOLVENAME(%s)", cp);
    return ccnl_strdup(res);
}

char*
op_builtin_cmpeq(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                 int *restart, int *halt, char *prog, char *pending,
                 struct stack_s **stack)
{
    int i1=0, i2=0;
    char res[1000], *cp;

    DEBUGMSG(DEBUG, "---to do: OP_CMPEQ<%s>\n", pending);
    pop2int();
    cp = (i1 == i2) ? "1" : "0";
    if (pending)
        sprintf(res, "RESOLVENAME(%s);%s", cp, pending);
    else
        sprintf(res, "RESOLVENAME(%s)", cp);
    return ccnl_strdup(res);
}

char*
op_builtin_cmpleq(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                  int *restart, int *halt, char *prog, char *pending,
                  struct stack_s **stack)
{
    int i1=0, i2=0;
    char res[1000], *cp;

    DEBUGMSG(DEBUG, "---to do: OP_CMPLEQ <%s>\n", pending);
    pop2int();
    cp = (i2 <= i1) ? "1" : "0";
    if (pending)
        sprintf(res, "RESOLVENAME(%s);%s", cp, pending);
    else
        sprintf(res, "RESOLVENAME(%s)", cp);
    return ccnl_strdup(res);
}

char*
op_builtin_ifelse(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                  int *restart, int *halt, char *prog, char *pending,
                  struct stack_s **stack)
{
    struct stack_s *h;
    int i1=0;

    DEBUGMSG(DEBUG, "---to do: OP_IFELSE <%s>\n", prog+10);
    h = pop_or_resolve_from_result_stack(ccnl, config);
    if (!h) {
        *halt = -1;
        return ccnl_strdup(prog);
    }
    if (h->type != STACK_TYPE_INT) {
        DEBUGMSG(WARNING, "ifelse requires int as condition");
        ccnl_nfn_freeStack(h);
        return NULL;
    }
    i1 = *(int *)h->content;
    if (i1) {
        struct stack_s *stack = pop_from_stack(&config->argument_stack);
        DEBUGMSG(DEBUG, "Execute if\n");
        pop_from_stack(&config->argument_stack);
        push_to_stack(&config->argument_stack, stack->content,
                      STACK_TYPE_CLOSURE);
    } else {
        DEBUGMSG(DEBUG, "Execute else\n");
        pop_from_stack(&config->argument_stack);
    }
    return ccnl_strdup(pending);
}

// ----------------------------------------------------------------------

#ifdef USE_NFN_NSTRANS

// NFN namespace translation example: "translate NS /some/uri/to/fetch"
// where NS is a constant: 'ccnb, 'ccnx2014, 'ndn2013

char*
op_builtin_nstrans(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                   int *restart, int *halt, char *prog, char *pending,
                   struct stack_s **stack)
{
    char *cp = NULL;
    struct stack_s *s1, *s2;

    DEBUGMSG(DEBUG, "---to do: OP_NSTRANS\n");

    s1 = pop_or_resolve_from_result_stack(ccnl, config);
    if (!s1) {
        *halt = -1;
        return prog;
    }
    s2 = pop_or_resolve_from_result_stack(ccnl, config);
    if (!s2) {
        ccnl_nfn_freeStack(s1);
        *halt = -1;
        return prog;
    }

    if (s2->type == STACK_TYPE_CONST && s1->type == STACK_TYPE_PREFIX) {
        struct ccnl_prefix_s *p = (struct ccnl_prefix_s*) s1->content;
        struct const_s *con = (struct const_s *) s2->content;
        int suite = ccnl_str2suite(con->str);
        DEBUGMSG(DEBUG, "  original packet format: %s\n", con->str);

        /*
        if (!strcmp(con->str, "ccnb"))
            suite = CCNL_SUITE_CCNB;
        else if (!strcmp(con->str, "ccnx2014"))
            suite = CCNL_SUITE_CCNTLV;
        else if (!strcmp(con->str, "cisco2015"))
            suite = CCNL_SUITE_CISTLV;
        else if (!strcmp(con->str, "ndn2013"))
            suite = CCNL_SUITE_NDNTLV;
        */
        if (suite < 0)
            goto out;
        DEBUGMSG(DEBUG, " >> changing PREFIX suite from %d to %d\n",
                 p->suite, suite);

        p->nfnflags = 0;
        p->suite = suite;
        push_to_stack(stack, s1->content, STACK_TYPE_PREFIX);

        ccnl_free(s1);
        s1 = NULL;

        if (pending) {
            cp = ccnl_malloc(strlen(pending)+1);
            strcpy(cp, pending);
        }
    } else {
out:
        *halt = -1;
        cp = prog;
    }
    if (s1)
        ccnl_nfn_freeStack(s1);
    ccnl_nfn_freeStack(s2);

    return cp;
}

#endif // USE_NFN_NSTRANS

// ----------------------------------------------------------------------

struct builtin_s bifs[] = {
    {"OP_ADD",           op_builtin_add,  NULL},
    {"OP_FIND",          op_builtin_find, NULL},
    {"OP_MULT",          op_builtin_mult, NULL},
    {"OP_RAW",           op_builtin_raw,  NULL},
    {"OP_SUB",           op_builtin_sub,  NULL},

    {"OP_CMPEQ_CHURCH",  op_builtin_cmpeqc, NULL},
    {"OP_CMPLEQ_CHURCH", op_builtin_cmpleqc, NULL},
    {"OP_CMPEQ",         op_builtin_cmpeq, NULL},
    {"OP_CMPLEQ",        op_builtin_cmpleq, NULL},
    {"OP_IFELSE",        op_builtin_ifelse, NULL},

#ifdef USE_NFN_NSTRANS
    {"OP_NSTRANS",       op_builtin_nstrans, NULL},
#endif

    {NULL, NULL, NULL}
};

// eof
