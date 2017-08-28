/*
 * @f ccnl-buf.h
 * @b CCN lite (CCNL), core header file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
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
 * File history:
 * 2017-06-16 created
 */
#ifndef CCNL_LINUXKERNEL
#define CCNL_ARRAY_DEFAULT_CAPACITY 4
#define CCNL_ARRAY_CHECK_BOUNDS
#define CCNL_ARRAY_NOT_FOUND -1

struct ccnl_array_s
{
    int count;
    int capacity;
    void **items;
};

struct ccnl_array_s*
ccnl_array_new(int capacity);

void
ccnl_array_free(struct ccnl_array_s *array);

// Insert an item at the end of the array, possibly realocating the array.
void
ccnl_array_push(struct ccnl_array_s *array, void *item);

// Return and remove the last item.
void*
ccnl_array_pop(struct ccnl_array_s *array);

// Insert item at index by pushing down all subsequent items by one, 
// possibly reallocating the array.
void
ccnl_array_insert(struct ccnl_array_s *array, void *item, int index);

// Remove all occurrences of item by moving up the subsequent items.
void 
ccnl_array_remove(struct ccnl_array_s *array, void *item);

// Remove the item at index by moving up all subsequent items by one.
void 
ccnl_array_remove_index(struct ccnl_array_s *array, int index);

// Returns the index of the first occurrence of item, or CCNL_ARRAY_NOT_FOUND.
int 
ccnl_array_find(struct ccnl_array_s *array, void *item);

// Returns 1 if the item can be found in the array.
int 
ccnl_array_contains(struct ccnl_array_s *array, void *item);

#endif
