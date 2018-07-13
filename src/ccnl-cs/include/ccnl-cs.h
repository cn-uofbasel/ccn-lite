/*
 * @file ccnl-cs.h
 * @brief CCN lite - Content Store Implementation
 *
 * Copyright (C) 2018 HAW Hamburg
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
 */

#ifndef CCNL_CS
#define CCNL_CS

#include "stdint.h"

typedef struct {
    uint8_t *name;
    unsigned namelen;
} ccnl_cs_name_t;

typedef struct {
    uint8_t *content;
    unsigned contlen;
} ccnl_cs_content_t;

typedef void (*ccnl_cs_op_add_t)(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content);
typedef ccnl_cs_content_t *(*ccnl_cs_op_lookup_t)(const ccnl_cs_name_t *name);
typedef void (*ccnl_cs_op_remove_t)(const ccnl_cs_name_t *name);

typedef struct {
    ccnl_cs_op_add_t add;
    ccnl_cs_op_lookup_t lookup;
    ccnl_cs_op_remove_t remove;
} ccnl_cs_ops_t;

void
ccnl_cs_init(ccnl_cs_ops_t *ops,
             ccnl_cs_op_add_t add_fun,
             ccnl_cs_op_lookup_t lookup_fun,
             ccnl_cs_op_remove_t remove_fun);
void
ccnl_cs_add(ccnl_cs_ops_t *ops,
            const ccnl_cs_name_t *name,
            const ccnl_cs_content_t *content);

ccnl_cs_op_lookup_t *
ccnl_cs_lookup(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name);

void
ccnl_cs_remove(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name);

#endif //CCNL_CS
