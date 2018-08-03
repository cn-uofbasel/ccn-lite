/**
 * @file test-cs.c
 * @brief CCN lite - Tests for content store implementation
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

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-cs.h"


static int setup(void **state) {
     ccnl_cs_ops_t content_store;
     /** TODO: set function pointers */

     state = (void*)&content_store;

     return 0;
}

static int teardown(void **state) {

     return 0;
}
 
void test1()
{
  int result = 0;
  assert_int_equal(result, 0);
}

void test_ccnl_cs_remove_invalid_struct()
{
    /** build up our ccnl_cs_name_t struct */
    uint8_t _size = 10;
    uint8_t _name[_size];
    memset(&(_name[0]), 0x42, _size);  

    /** set the name */
    ccnl_cs_name_t name;
    /** TODO: that's a bit hard to read -> fix */
    name.name = &(_name[0]);
    name.namelen = _size;
            
    int result = ccnl_cs_remove(NULL, &name);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, 0);
}

void test_ccnl_cs_remove_wrong_name_size(void **state)
{
    /* retrieve implementation */
    ccnl_cs_ops_t *content_store_impl = (ccnl_cs_ops_t*)state;
    /* build up our ccnl_cs_name_t struct */
    uint8_t _size = 10;
    uint8_t _name[_size];
    memset(&(_name[0]), 0x42, _size);  

    /** set the name */
    ccnl_cs_name_t name;
    /** TODO: that's a bit hard to read -> fix */
    name.name = &(_name[0]);
    /** what happens if we set the size to 0? */
    name.namelen = 0;
            
    /* TODO: pass actual valid first parameter */
    int result = ccnl_cs_remove(content_store_impl, &name);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, 0);

    /** what happens if we set the size to two-times the actual size? */
    name.namelen = 2 * _size;
    /* TODO: pass actual valid first parameter */
    result = ccnl_cs_remove(content_store_impl, &name);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, 0);
}

void test_ccnl_cs_remove_partially_valid_structs(void **state)
{
    /* retrieve implementation */
    ccnl_cs_ops_t *content_store_impl = (ccnl_cs_ops_t*)state;

    /** leave the member 'name' uninitialized */
    ccnl_cs_name_t name;
    /** but give it a size */
    name.namelen = 256;

    int result = ccnl_cs_remove(content_store_impl, &name);
    /* call should fail */
    assert_int_equal(result, 0);

    /* okay, let's set the name to NULL, is a check performed? */
    name.name = NULL;

    result = ccnl_cs_remove(content_store_impl, &name);
    /* call should fail */
    assert_int_equal(result, 0);
}

 
int main(void)
{
     const UnitTest tests[] = {
         unit_test(test1),
    /*
         unit_test(test_ccnl_cs_remove_invalid_struct),
         unit_test(test_ccnl_cs_remove_invalid_name),
         unit_test(test_ccnl_cs_remove_wrong_name_size, setup, NULL),
         unit_test(test_ccnl_cs_remove_partially_valid_structs, setup, NULL),
    */
     };
    
     return run_tests(tests); 
}