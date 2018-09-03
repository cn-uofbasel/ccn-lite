/*
 * @f ccnl-cs.c
 * @b Implementation of helper functions for the content store
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
 *
 */
#include "ccnl-cs-helper.h"

#include <string.h>

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


