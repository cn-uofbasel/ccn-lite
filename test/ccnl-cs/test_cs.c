/**
 * @file test-cs.c
 * @brief CCN lite - Tests for content store implementation
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

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>

#include "ccnl-cs.h"
#include "ccnl-defs.h"
#include "ccnl-cs-ll.h"
#include "ccnl-prefix.h"
#include "ccnl-cs-helper.h"
#include "ccnl-pkt.h"
#include "ccnl-pkt-ndntlv.h"

static ccnl_cs_ops_t content_store;

/** initializs the \ref ccnl_cs_ops_t with the function pointers of the utlist implementation */
static void setup_cs_ll(void **state) {
     ccnl_cs_init_ll(&content_store);
}

static void teardown_cs(void **state) {
     ccnl_cs_clear(&content_store);
}

/** initializs the \ref ccnl_cs_ops_t with the function pointers of the uthash implementation */
static void setup_cs_uthash(void **state) {
     ccnl_cs_init_uthash(&content_store);
}

void test_ccnl_cs_add_invalid_parameters()
{
    /** prepare name */
    uint8_t prefix_len = 18;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/some/test", prefix_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);
    
    ccnl_cs_content_t content;
            
    int result = ccnl_cs_add(NULL, name, &content);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);

    /* we don't initialize the function pointers */
    ccnl_cs_ops_t options;

    result = ccnl_cs_add(&options, NULL, &content);
    /* if we pass an invalid name, call should fail */
    assert_int_equal(result, CS_NAME_IS_INVALID);

    result = ccnl_cs_add(&options, name, NULL);
    /* if we pass an invalid content object, call should fail */
    assert_int_equal(result, CS_CONTENT_IS_INVALID);

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_lookup_invalid_parameters()
{
    ccnl_cs_name_t name; 
    ccnl_cs_content_t content;

    int result = ccnl_cs_lookup(NULL, &name, &content);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);

    /* we don't initialize the function pointers */
    ccnl_cs_ops_t options;

    result = ccnl_cs_lookup(&options, NULL, &content);
    /* if we pass an invalid name, call should fail */
    assert_int_equal(result, CS_NAME_IS_INVALID);
}

void test_ccnl_cs_remove_invalid_parameters()
{
    /** prepare name */
    uint8_t prefix_len = 27;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/again/another/test", prefix_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);
            
    int result = ccnl_cs_remove(NULL, name);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);

    /* we don't set function pointers */
    ccnl_cs_ops_t options;

    result = ccnl_cs_remove(&options, NULL);
    /* if we pass an invalid name, call should fail */
    assert_int_equal(result, CS_NAME_IS_INVALID);

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_add_successful()
{
    /** prepare name */
    uint8_t prefix_len = 29;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/content/store/test", prefix_len);

    uint8_t payload_len = 26;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "abcdefghijklmnopqrstuvwxyz", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);

    assert_int_equal(result, 0);
    assert_non_null(content);

    /** add element to the content store */
    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 1;
    assert_int_equal(expected_size, size); 
    
    /** clear the content store, todo: move to teardown function */
    ccnl_cs_clear(&content_store);

    ccnl_prefix_free(name);
    free(prefix);
    free(payload);
}

void test_ccnl_cs_add_unsuccessful()
{
    /** prepare name */
    uint8_t prefix_len = 29;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/content/store/test", prefix_len);

    uint8_t payload_len = 26;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "abcdefghijklmnopqrstuvwxyz", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);

    assert_int_equal(result, 0);
    assert_non_null(content);

    /** add element to the content store */
    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 1;
    assert_int_equal(expected_size, size); 

    /** adding the same element a second time should fail */
    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_UNSUCCESSFUL);

    /** there should be still only one entry in the content store */
    assert_int_equal(expected_size, size); 
    
    /** clear the content store, todo: move to teardown function */
    ccnl_cs_clear(&content_store);

    ccnl_prefix_free(name);
    free(prefix);
    free(payload);
}

void test_ccnl_cs_clear_invalid_parameter() 
{
    /** pass invalid parameter to the ccnl_cs_clear function */
    int result = ccnl_cs_clear(NULL);
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);
}

void test_ccnl_cs_clear_successful()
{
    /** prepare name */
    uint8_t prefix_len = 36;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/test/for/ccnl_cs_ll_clear", prefix_len);

    uint8_t payload_len = 6;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "abcdef", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);
    assert_non_null(content);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 0;
    assert_int_equal(expected_size, size); 

    /** add a single element and call clear */
    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    size = ccnl_cs_get_cs_current_size();
    expected_size = 1;
    assert_int_equal(expected_size, size); 

    result = ccnl_cs_clear(&content_store);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    size = ccnl_cs_get_cs_current_size();
    expected_size = 0;
    assert_int_equal(expected_size, size); 

    /** create a new content element */
    result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);
    assert_non_null(content);

    /** prepare another name */
    prefix_len = 42;
    char *another_prefix = malloc(sizeof(char) * prefix_len);
    strncpy(another_prefix, "/this/is/a/test/for/ccnl_cs_ll_clear/again", prefix_len);

    payload_len = 12;
    unsigned char *another_payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(another_payload, "abcdefghijkl", payload_len);

    /** set another name */
    ccnl_cs_name_t *another_name = ccnl_URItoPrefix(another_prefix, CCNL_SUITE_NDNTLV, NULL);

    /** create another content element */
    ccnl_cs_content_t *another_content = NULL;
    result = ccnl_cs_build_content(&another_content, another_name, NULL, another_payload, payload_len);
    assert_int_equal(result, 0);
    assert_non_null(content);

    /** add multiple elements and call clear */
    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** only one entry in the content store */
    size = ccnl_cs_get_cs_current_size();
    expected_size = 1;
    assert_int_equal(expected_size, size); 

    /** add another content element to the content store */
    result = ccnl_cs_add(&content_store, another_name, another_content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** there should be two entries in the content store */
    size = ccnl_cs_get_cs_current_size();
    expected_size = 2;
    assert_int_equal(expected_size, size); 

    /** remove everything */
    result = ccnl_cs_clear(&content_store);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** content store should be empty */
    size = ccnl_cs_get_cs_current_size();
    expected_size = 0;
    assert_int_equal(expected_size, size); 

    /** clean up */
    ccnl_prefix_free(another_name);
    free(another_prefix);
    free(another_payload);

    ccnl_prefix_free(name);
    free(prefix);
    free(payload);
}

void test_ccnl_cs_remove_non_existent_entry()
{
    /** the content store should be empty */
    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 0;
    assert_int_equal(expected_size, size); 

    /** create a prefix */
    uint8_t prefix_len = 35;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/another/content/store/test", prefix_len);
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    assert_non_null(name);

    /** the name/prefix has not been added before so the name should not be found */
    int result = ccnl_cs_remove(&content_store, name);
    assert_int_equal(result, CS_NAME_COULD_NOT_BE_FOUND);

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_set_cs_capacity_successful()
{
    size_t size = 42; 
    assert_int_not_equal(size, ccnl_cs_get_cs_capacity());
    ccnl_cs_set_cs_capacity(size);
    assert_int_equal(size, ccnl_cs_get_cs_capacity());
}

void test_ccnl_cs_get_cs_capacity_successful()
{
    size_t size = 32;
    assert_int_equal(size, ccnl_cs_get_cs_capacity());
}

void test_ccnl_cs_lookup_successful()
{
    /** prepare name */
    uint8_t prefix_len = 37;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/test/for/ccnl_cs_ll_lookup", prefix_len);

    uint8_t payload_len = 6;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "abc123", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);
    assert_non_null(content);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 0;
    assert_int_equal(expected_size, size); 

    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    size = ccnl_cs_get_cs_current_size();
    expected_size = 1;
    assert_int_equal(expected_size, size); 

    /** lookup a name in the content store */
    ccnl_cs_content_t *item = NULL;
    result = ccnl_cs_lookup(&content_store, name, &item);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);
    assert_non_null(item);

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_remove_successful()
{
    /** prepare name */
    uint8_t prefix_len = 37;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/test/for/ccnl_cs_ll_remove", prefix_len);

    uint8_t payload_len = 6;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "def123", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);
    assert_non_null(content);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 0;
    assert_int_equal(expected_size, size); 

    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    size = ccnl_cs_get_cs_current_size();
    expected_size = 1;
    assert_int_equal(expected_size, size); 

    result = ccnl_cs_remove(&content_store, name);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    size = ccnl_cs_get_cs_current_size();
    expected_size = 0;
    assert_int_equal(expected_size, size); 

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_match_interest_invalid_parameters() 
{
    struct ccnl_pkt_s packet;
    ccnl_cs_content_t *content = NULL;
            
    int result = ccnl_cs_match_interest(NULL, &packet, content);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);

    /* we don't set function pointers */
    ccnl_cs_ops_t options;

    result = ccnl_cs_match_interest(&options, NULL, content);
    /* if we pass an invalid name, call should fail */
    assert_int_equal(result, CS_PACKET_IS_INVALID);
}

void test_ccnl_cs_match_interest_successful() 
{
    /** prepare name */
    uint8_t prefix_len = 45;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/a/test/for/ccnl_cs_ll_match_interest", prefix_len);

    uint8_t payload_len = 6;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "jkl123", payload_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);

    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    ccnl_cs_content_t *temporary = NULL;
    result = ccnl_cs_match_interest(&content_store, content->pkt, temporary);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    ccnl_prefix_free(name);
    free(prefix);
}

void test_ccnl_cs_match_interest_unsuccessful() 
{
    uint8_t prefix_len = 51;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/another/test/for/ccnl_cs_ll_match_interest", prefix_len);

    uint8_t payload_len = 6;
    unsigned char *payload = malloc(sizeof(unsigned char) * payload_len);
    strncpy(payload, "jkl453", payload_len);

    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *content = NULL;
    int8_t result = ccnl_cs_build_content(&content, name, NULL, payload, payload_len);
    assert_int_equal(result, 0);

    result = ccnl_cs_add(&content_store, name, content);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    prefix_len = 23;
    char *some_other_prefix = malloc(sizeof(char) * prefix_len);
    strncpy(some_other_prefix, "/this/is/something/else", prefix_len);

    ccnl_cs_name_t *some_other_name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

    ccnl_cs_content_t *some_other_content = NULL;
    result = ccnl_cs_build_content(&some_other_content, some_other_name, NULL, payload, payload_len);
    assert_int_equal(result, 0);

    ccnl_cs_content_t *temporary = NULL;
    result = ccnl_cs_match_interest(&content_store, some_other_content->pkt, temporary);
    printf(">>>> result is %d\n", result);
    assert_int_equal(result, CS_OPERATION_UNSUCCESSFUL);

    ccnl_content_free(some_other_content);
    ccnl_prefix_free(some_other_name);
    free(some_other_prefix);

    ccnl_prefix_free(name);
    free(prefix);
}

int main(void)
{
     const UnitTest tests[] = {
         unit_test(test_ccnl_cs_add_invalid_parameters),
         unit_test_setup_teardown(test_ccnl_cs_add_successful, setup_cs_ll, teardown_cs),
         unit_test_setup_teardown(test_ccnl_cs_add_unsuccessful, setup_cs_ll, teardown_cs),

         unit_test(test_ccnl_cs_lookup_invalid_parameters),
         unit_test_setup_teardown(test_ccnl_cs_lookup_successful, setup_cs_ll, teardown_cs),

         unit_test(test_ccnl_cs_remove_invalid_parameters),
         unit_test_setup_teardown(test_ccnl_cs_remove_successful, setup_cs_ll, teardown_cs),
         unit_test_setup_teardown(test_ccnl_cs_remove_non_existent_entry, setup_cs_ll, teardown_cs),

         unit_test(test_ccnl_cs_clear_invalid_parameter),
         unit_test_setup_teardown(test_ccnl_cs_clear_successful, setup_cs_ll, teardown_cs),

         unit_test_setup_teardown(test_ccnl_cs_get_cs_capacity_successful, setup_cs_ll, teardown_cs),
         unit_test(test_ccnl_cs_set_cs_capacity_successful),

         unit_test(test_ccnl_cs_match_interest_invalid_parameters),
         unit_test_setup_teardown(test_ccnl_cs_match_interest_successful, setup_cs_ll, teardown_cs),
         unit_test_setup_teardown(test_ccnl_cs_match_interest_unsuccessful, setup_cs_ll, teardown_cs),
/*
         unit_test_setup(test_ccnl_cs_remove_non_existent_entry, setup_cs_uthash),
         unit_test_setup(test_ccnl_cs_get_cs_capacity_successful, setup_cs_uthash),
         unit_test_setup(test_ccnl_cs_clear_successful, setup_cs_uthash),
 */
     };
    
     return run_tests(tests); 
}
