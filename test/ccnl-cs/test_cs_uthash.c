/**
 * @file test_cs_uthash.c
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

#include "ccnl-cs.h"
#include "ccnl-cs-helper.h"
#include "ccnl-cs-uthash.h"

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


ccnl_cs_ops_t content_store;

static int setup(void **state) {
     ccnl_cs_init_uthash(&content_store);

     //state = (void*)&content_store;

     return 0;
}

static int teardown(void **state) {
     return 0;
}

void ccnl_cs_uthash_exists_false(void **state)
{
    const char *str = "/some/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);
    /** assert should fail, since we haven't added any data to the hash map */
    assert_false(ccnl_cs_uthash_exists(name));

    free(name);
}

void ccnl_cs_uthash_exists_true(void **state)
{
    const char *str = "/some/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);
    
    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = sizeof(uint8_t) * 8;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    /** check that creating content was successful */
    assert_ptr_not_equal(content, NULL);

    /** assert should fail, since we haven't added any data to the hash map */
    assert_false(ccnl_cs_uthash_exists(name));
    
    int result = ccnl_cs_add(&content_store, name, content);
    /** check that adding content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** assert should not fail, since we added data to the hash map */
    assert_true(ccnl_cs_uthash_exists(name));

    /** adding the same name/content should fail */
    result = ccnl_cs_add(&content_store, name, content);
    /** check that adding content failed */
    assert_int_equal(result, CS_OPERATION_UNSUCCESSFUL);

    /** remove entry from the hash map */
    result = ccnl_cs_remove(&content_store, name);
    /** check that removing content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);
}

void ccnl_cs_uthash_remove_name_not_found(void **state) 
{
    const char *str = "/yet/another/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);

    int result = ccnl_cs_remove(&content_store, name);
    /** check that removing content was unsuccessful */
    assert_int_equal(result, CS_NAME_COULD_NOT_BE_FOUND);

    free(name);
}

void ccnl_cs_uthash_remove_successful(void **state) 
{
    const char *str = "/and/yet/another/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);

    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = sizeof(uint8_t) * 8;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    /** check that creating content was successful */
    assert_ptr_not_equal(content, NULL);

    int result = ccnl_cs_add(&content_store, name, content);
    /** check that adding content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    result = ccnl_cs_remove(&content_store, name);
    /** check that removing content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);
}

void ccnl_cs_uthash_lookup_name_not_found(void **state) 
{
    const char *str = "/some/lookup/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);

    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = sizeof(uint8_t) * 8;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    /** check that creating content was successful */
    assert_ptr_not_equal(content, NULL);

    int result = ccnl_cs_lookup(&content_store, name, content);
    /** check that removing content was unsuccessful */
    assert_int_equal(result, CS_NAME_COULD_NOT_BE_FOUND);

    free(name);
    free(content);
}

void ccnl_cs_uthash_lookup_successful(void **state) 
{
    const char *str = "/another/lookup/test/case";
    ccnl_cs_name_t *name = ccnl_cs_create_name(str, strlen(str));
    /** check that creating a name was successful */
    assert_ptr_not_equal(name, NULL);

    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = sizeof(uint8_t) * 8;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    /** check that creating content was successful */
    assert_ptr_not_equal(content, NULL);

    int result = ccnl_cs_add(&content_store, name, content);
    /** check that adding content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    ccnl_cs_content_t *content_lookup_pointer = malloc(sizeof(ccnl_cs_content_t)); 

    result = ccnl_cs_lookup(&content_store, name, content_lookup_pointer);
    /** check that removing content was unsuccessful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    result = ccnl_cs_remove(&content_store, name);
    /** check that removing content was successful */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    free(content_lookup_pointer);
}


int main(void)
{
     const UnitTest tests[] = {
         unit_test_setup(ccnl_cs_uthash_exists_false, setup),
         unit_test_setup(ccnl_cs_uthash_exists_true, setup),
         unit_test_setup(ccnl_cs_uthash_remove_name_not_found, setup),
         unit_test_setup(ccnl_cs_uthash_remove_successful, setup),
         unit_test_setup(ccnl_cs_uthash_lookup_name_not_found, setup),
         unit_test_setup(ccnl_cs_uthash_lookup_successful, setup),
     };
    
     return run_tests(tests); 
}
