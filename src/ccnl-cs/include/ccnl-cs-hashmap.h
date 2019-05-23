/**
 * @f ccnl-cs-hashmap.h
 * @b A hashmap-based content store 
 *
 * Copyright (C) 2018 MSA Safety
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

#ifndef CCNL_CS_HASHMAP
#define CCNL_CS_HASHMAP

#include "ccnl-cs.h"

#include <stdint.h>

typedef struct {
    ccnl_cs_name_t *key;
    ccnl_cs_content_t *value;
    struct ccnl_cs_hashmap_entry_t *next;
} ccnl_cs_hashmap_entry_t;

typedef struct {
    uint32_t size;
    ccnl_cs_hashmap_entry_t **entries;
} ccnl_cs_hashmap_t;


int init(const uint32_t size);

void deinit(void);

int hash(ccnl_cs_name_t *key);

void add(ccnl_cs_name_t *key, ccnl_cs_content_t *value);

int size(void);

//int lookup(int key);

#endif 
