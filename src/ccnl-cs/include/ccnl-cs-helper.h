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
//#include "ccnl-prefix.h"

#include <stdbool.h>

typedef enum {
    CCNL_CS_HELPER_COMPARE_EXACT = 0,             /**< */
    CCNL_CS_HELPER_COMPARE_MATCH = 1,             /**< */
    CCNL_CS_HELPER_COMPARE_LONGEST = 2,           /**< */

    CCNL_CS_HELPER_COMPARE_DO_NOT_USE = UINT8_MAX /**< sets the size of the enum to a fixed width, do not use */
} ccnl_cs_helper_compare_t;

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
ccnl_cs_content_t *ccnl_cs_create_content(uint8_t *content, size_t size);

/**
 * @brief Compares two \ref ccnl_cs_name_t structs
 *
 * @param[in] first  First name to be compared (shorter of longest prefix matching)
 * @param[in] second Second prefix to be compared (longer of longest prefix matching)
 * @param[in] mode   Mode for name comparison \ref ccnl_cs_helper_compare_t
 *
 * @return -1 if no match at all (all modes) or exact match failed
 * @return  0 if full match (\ref CCNL_CS_HELPER_COMPARE_EXACT)
 * @return  n>0 for matched components (only applies to modes \ref CCNL_CS_HELPER_COMPARE_MATCH 
     or \ref CCNL_CS_HELPER_COMPARE_LONGEST)
*/
int ccnl_cs_name_compare(ccnl_cs_name_t *first, ccnl_cs_name_t *second, uint8_t mode);


//ccnl_cs_name_t *ccnl_cs_prefix_to_name(struct ccnl_prefix_s *prefix);

ccnl_cs_name_t *ccnl_cs_char_to_prefix(char *name);

/**
 *
 */
bool ccnl_cs_perform_ageing(ccnl_cs_content_t *c);

#endif //CCNL_CS_HELPER
