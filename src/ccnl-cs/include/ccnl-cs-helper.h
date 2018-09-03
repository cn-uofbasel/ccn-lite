/**
 * @f ccnl-cs-helper.h
 * @b Helper functions for the content store 
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

#ifndef CCNL_CS_HELPER
#define CCNL_CS_HELPER

#include "ccnl-cs.h"

/**
 * Allocates a \ref ccnl_cs_name_t struct
 *
 * @param[in] prefix The prefix to set in the name
 * @param[in] size The length of the \p name
 *
 * @return Upon success an allocated and initialized \ref ccnl_cs_name_t struct, NULL otherwise
 */
ccnl_cs_name_t *ccnl_cs_create_name(const char* prefix, size_t length);

/**
 * Allocates a \ref ccnl_cs_content_t struct
 *
 * @param[in] content The content to set
 * @param[in] size The size of the \p content
 *
 * @return Upon success an allocated and initialized \ref ccnl_cs_content_t struct, NULL otherwise
 */
ccnl_cs_content_t *ccnl_cs_create_content(uint8_t *content, 
                size_t size);

#endif //CCNL_CS_HELPER
