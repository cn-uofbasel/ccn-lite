/*
 * @file ccnl-cs-simple.c
 * @brief Simple CS - Simple Content Store Implementation
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
 *
 */

#include "ccnl-cs-simple.h"

ccnl_cs_ops_t ccnl_cs_ops_simple;

static void add(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content) {
    return;
}

static cnl_cs_content_t *lookup(const ccnl_cs_name_t *name) {
    return NULL;
}

static void remove(const ccnl_cs_name_t *name) {
    return NULL;
}

void ccnl_cs_init_simple(void) {
    ccnl_cs_init(&ccnl_cs_ops_simple,
                 add,
                 lookup,
                 remove);
    return;
}
