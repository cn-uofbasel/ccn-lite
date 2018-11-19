/**
 * @f ccnl-cs-uthash.h
 * @b uthash-based content store 
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

#ifndef CCNL_CS_SIMPLE
#define CCNL_CS_SIMPLE

#include "ccnl-cs.h"
#include "uthash.h"

#include <stdbool.h>

/**
 * @brief An entry in the uthash-based implementation of the
 *  content store.
 */
typedef struct {
    ccnl_cs_name_t *key;       /**< key */
    ccnl_cs_content_t *value;  /**< value */
    UT_hash_handle hh;         /**< makes structure hashable */
} ccnl_cs_uthash_t;

/**
 * @brief Sets the \p ops struct to the uthash-powered content store
 *
 * @param[in] ops The struct to set the functions points to the internal
 *   uthash-powered add/lookup/remove functions
 */
void ccnl_cs_init_uthash(ccnl_cs_ops_t *ops);

/**
 * @brief Checks if a given \p name is already stored in the content store
 *
 * @param[in] name The name to lookup in the content store
 *
 * @return true if the \p name is in the content store, false otherwise
 */
bool ccnl_cs_uthash_exists(ccnl_cs_name_t *name);


#endif //CCNL_CS_UTHASH
