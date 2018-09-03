/*
 * @file ccnl-cs.c
 * @brief CS - Content Store
 *
 * Copyright (C) 2018 HAW Hamburg
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
 *
 */
#include "ccnl-cs.h"

#include <stdio.h>
#include <string.h>

void
ccnl_cs_init(ccnl_cs_ops_t *ops,
             ccnl_cs_op_add_t add_fun,
             ccnl_cs_op_lookup_t lookup_fun,
             ccnl_cs_op_remove_t remove_fun) {

    if (ops) {
        ops->add = add_fun; 
        ops->lookup = lookup_fun; 
        ops->remove = remove_fun;
    }

    return;
}

ccnl_cs_status_t
ccnl_cs_add(ccnl_cs_ops_t *ops,
            const ccnl_cs_name_t *name,
            const ccnl_cs_content_t *content) {
    int result = CS_CONTENT_IS_INVALID;

    if (ops) {
        if (name) {
            if (content) {
                return ops->add(name, content); 
            }
        } else {
            result = CS_NAME_IS_INVALID;
        }
    } else {
        result = CS_OPTIONS_ARE_NULL;
    }

    return result;
}

ccnl_cs_status_t
ccnl_cs_lookup(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name, ccnl_cs_content_t *content) {
    int result = CS_CONTENT_IS_INVALID;

    if (ops) {
        if (name) {
            if (content) {
                return ops->lookup(name, content);
            }
        } else {
            result = CS_NAME_IS_INVALID;
        }
    } else {
        result = CS_OPTIONS_ARE_NULL;
    }

    return result;
}

ccnl_cs_status_t
ccnl_cs_remove(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name) {
    int result = CS_NAME_IS_INVALID;

    if (ops) {
        if (name) {
            return ops->remove(name);
        }
    } else {
        result = CS_OPTIONS_ARE_NULL;
    }

    return result;
}

ccnl_cs_name_t *ccnl_cs_create_name(const char* name, size_t length) {
    /** allocate the size of the structure + the size of the string we'll store*/
    size_t size = sizeof(ccnl_cs_name_t); // + (sizeof(uint8_t) * length);
    ccnl_cs_name_t *result = malloc(size);

    if (result) {
        size = (sizeof(uint8_t) * length);
        result->name = malloc(size);

        if (result->name) {
            /* set members of 'result' struct */
            memcpy(result->name, name, length);
            result->length = length;
        } else {
            free(result);
            result = NULL;
        }
    }

    return result;
}


ccnl_cs_content_t *ccnl_cs_create_content(uint8_t *content, size_t size) {
    ccnl_cs_content_t *result = malloc(sizeof(ccnl_cs_content_t));

    if (result) {
        result->content = malloc(sizeof(uint8_t) * size);

        if (result->content) {
            /* set members of 'result' struct */
            memcpy(result->content, content, size);
            result->length = size;
        /* could not allocate memory for member 'content' */
        } else {
            free(result);
            result = NULL;
        }
    }

    return result;
}


