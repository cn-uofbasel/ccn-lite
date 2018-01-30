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

void null_func(void);

#ifndef CCNL_LINUXKERNEL

#include "ccnl-array.h"

#include <stddef.h>

#include "ccnl-malloc.h"

struct ccnl_array_s*
ccnl_array_new(int capacity)
{
    size_t size = sizeof(struct ccnl_array_s);
    struct ccnl_array_s* array = (struct ccnl_array_s *)ccnl_malloc(size);
    array->count = 0;
    array->capacity = capacity ? capacity : CCNL_ARRAY_DEFAULT_CAPACITY;
    array->items = (void **)ccnl_calloc(array->capacity, sizeof(void *));
    return array;
}

void
ccnl_array_free(struct ccnl_array_s *array)
{
    if (array) 
    {
        ccnl_free(array->items);
        ccnl_free(array);
    }
}

void
ccnl_array_push(struct ccnl_array_s *array, void *item)
{
    ccnl_array_insert(array, item, array->count);
}

void*
ccnl_array_pop(struct ccnl_array_s *array)
{
    #ifdef CCNL_ARRAY_CHECK_BOUNDS
        if (array->count < 1)
        {
            // TODO: warning
            return NULL;
        }
    #endif
    array->count--;
    return array->items[array->count];
}

void
ccnl_array_insert(struct ccnl_array_s *array, void *item, int index)
{
    int i;
    #ifdef CCNL_ARRAY_CHECK_BOUNDS
        if (index > array->count)
        {
            // TODO: warning
            return;
        }
    #endif
    if (index >= array->capacity) 
    {
        array->capacity = array->capacity * 3 / 2;
        array->items = (void **)ccnl_realloc(array->items, array->capacity * sizeof(void *));
    }

    for (i = array->count - 1; i >= index; i--)
    {
        array->items[i+1] = array->items[i];
    }
    array->items[index] = item;
    array->count++;
}

void 
ccnl_array_remove(struct ccnl_array_s *array, void *item)
{
    int offset = 0;
    int i = 0;
    while (i + offset < array->count) 
    {
        if (array->items[i + offset] == item) 
        {
            offset++;
        } 
        else 
        {
            array->items[i] = array->items[i + offset];
            i++;
        }
    }
    array->count -= offset;
}

void 
ccnl_array_remove_index(struct ccnl_array_s *array, int index)
{
    int i;
    #ifdef CCNL_ARRAY_CHECK_BOUNDS
        if (index >= array->count)
        {
            // TODO: warning
            return;
        }
    #endif
    for (i = index; i < array->count - 1; i++)
    {
        array->items[i] = array->items[i+1];
    }
    array->count--;
}

int 
ccnl_array_find(struct ccnl_array_s *array, void *item)
{
    int i;
    for (i = 0; i < array->count; i++)
    {
        if (array->items[i] == item) 
        {
            return i;
        }
    }
    return CCNL_ARRAY_NOT_FOUND;
}

int 
ccnl_array_contains(struct ccnl_array_s *array, void *item)
{
    return ccnl_array_find(array, item) != CCNL_ARRAY_NOT_FOUND;
}

#endif
