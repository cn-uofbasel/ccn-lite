/**
 * @f ccnl-cs-ll.h
 * @b linked list content store 
 *
 * Copyright (C) 2019 Safety IO
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

#ifndef CCNL_CS_LINKED_LIST
#define CCNL_CS_LINKED_LIST

#include "ccnl-cs.h"


/**
 * @brief Sets the \p ops struct to the linked list content store
 *
 * @param[in] ops The struct to set the functions points to the internal
 *   linked list  add/lookup/remove functions
 */
void ccnl_cs_init_ll(ccnl_cs_ops_t *ops);


#endif //CCNL_CS_LINKED_LIST
