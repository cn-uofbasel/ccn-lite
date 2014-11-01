/*
 * @f ccnl-ext-nfnkrivine.h
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
 * File history: <christopher.scherb@unibas.ch>
 */

#ifndef CCNL_EXT_NFNKRIVINE_H
#define CCNL_EXT_NFNKRIVINE_H

// built in function
typedef char* (*BIF)(struct ccnl_relay_s *ccnl, struct configuration_s *config,
               int *restart, int *halt, char *prog, char *pending,
                      struct stack_s **stack);

#define A_BIF(FCT) char* FCT(struct ccnl_relay_s *ccnl,\
        struct configuration_s *config, int *restart, int *halt,\
        char *prog, char *pending, struct stack_s **stack);

A_BIF(op_builtin_add)
A_BIF(op_builtin_find)
A_BIF(op_builtin_mult)
A_BIF(op_builtin_raw)
A_BIF(op_builtin_sub)

struct builtin_s {
    char *name;
    BIF fct;
    struct builtin_s *next;
};

struct builtin_s *extensions;
#endif //CCNL_EXT_NFNKRIVINE_H
