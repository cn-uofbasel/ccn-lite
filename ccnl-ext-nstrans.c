/*
 * @f ccnl-ext-nstrans.c
 * @b CCN lite, namespace translation, based on named-function-networking
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
 * 2014-10-15 created
 */

#ifdef USE_NFN_NSTRANS

// NFN example: "getFromNameSpace NS /some/uri/to/fetch"
// where NS is a constant: 'ccnb, 'ccnx2014, 'ndn2013

// ----------------------------------------------------------------------

char*
op_builtin_nstrans(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                   int *restart, int *halt, char *prog, char *pending,
                   struct stack_s **stack)
{
    char *cp = NULL;
    struct stack_s *s1, *s2;

    DEBUGMSG(99, "---to do: OP_NSTRANS\n");

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
        int suite = -1;

        if (!strcmp(s2->content, "ccnb"))
            suite = CCNL_SUITE_CCNB;
        else if (!strcmp(s2->content, "ccnx2014"))
            suite = CCNL_SUITE_CCNTLV;
        else if (!strcmp(s2->content, "ndn2013"))
            suite = CCNL_SUITE_NDNTLV;
        if (suite < 0)
            goto out;
        DEBUGMSG(99, " >> changed PREFIX suite to %d\n", suite);

        p->nfnflags = 0;
        p->suite = suite;
        push_to_stack(stack, s1->content, STACK_TYPE_PREFIX);

        ccnl_free(s1);
        s1 = NULL;

        if (pending) {
            cp = ccnl_malloc(strlen(pending+1)+1);
            strcpy(cp, pending+1);
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

// eof
