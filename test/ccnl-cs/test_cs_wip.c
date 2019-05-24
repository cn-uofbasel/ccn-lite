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
#include "ccnl-pkt-ndntlv.h"

static unsigned char _out[CCNL_MAX_PACKET_SIZE];
ccnl_cs_ops_t content_store;

static int _create_content(ccnl_cs_name_t *prefix, ccnl_cs_content_t *content, unsigned char *payload, int length);

static void setup(void **state) {
     ccnl_cs_init_ll(&content_store);
}

static int _create_content(ccnl_cs_name_t *prefix, ccnl_cs_content_t *content, unsigned char *payload, int length) 
{
    int suite = CCNL_SUITE_NDNTLV;
    size_t offset = CCNL_MAX_PACKET_SIZE;

    int arg_len = ccnl_ndntlv_prependContent(prefix, payload, length, NULL, NULL, &offset, _out, &length);

    int len;
    unsigned type;
    unsigned char *olddata;
    unsigned char *data = olddata = _out + offset;

    ccnl_ndntlv_dehead(&data, &arg_len, (int*) &type, &len);
    
    /*) 
    int result = -5;
    if ( (result = ccnl_ndntlv_dehead(&data, &arg_len, (int*) &type, &len)) || type != NDN_TLV_Data) {
        printf(">>> %d\n", result);
        return -1;
    }
    */

    struct ccnl_pkt_s *packet = ccnl_ndntlv_bytes2pkt(type, olddata, &data, &arg_len);

    if (!packet) {
        return -3;
    }

    content = ccnl_content_new(&packet);

    if (!content) {
        return -2;
    }

    return 0;
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
    /** prepare name */
    uint8_t prefix_len = 21;
    char *prefix = malloc(sizeof(char) * prefix_len);
    strncpy(prefix, "/this/is/another/test", prefix_len);

    /** set the name */
    ccnl_cs_name_t *name = ccnl_URItoPrefix(prefix, CCNL_SUITE_NDNTLV, NULL);

            
    ccnl_cs_content_t content;
    int result = ccnl_cs_lookup(NULL, name, &content);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPTIONS_ARE_NULL);

    /* we don't initialize the function pointers */
    ccnl_cs_ops_t options;

    result = ccnl_cs_lookup(&options, NULL, &content);
    /* if we pass an invalid name, call should fail */
    assert_int_equal(result, CS_NAME_IS_INVALID);

    result = ccnl_cs_lookup(&options, name, NULL);
    /* if we pass an invalid content object, call should fail */
    assert_int_equal(result, CS_CONTENT_IS_INVALID);

    ccnl_prefix_free(name);
    free(prefix);
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
/*
    int suite = CCNL_SUITE_NDNTLV;
    size_t offset = CCNL_MAX_PACKET_SIZE;

    int arg_len = ccnl_ndntlv_prependContent(name, payload, payload_len, NULL, NULL, &offset, _out, &payload_len);

    int len;
    unsigned type;
    unsigned char *olddata;
    unsigned char *data = olddata = _out + offset;

    if (ccnl_ndntlv_dehead(&data, &arg_len, (int*) &type, &len) || type != NDN_TLV_Data) {
        printf("error\n");
    }

    struct ccnl_pkt_s *packet = ccnl_ndntlv_bytes2pkt(type, olddata, &data, &arg_len);

    ccnl_cs_content_t *content = NULL;
    content = ccnl_content_new(&packet);
*/
    ccnl_cs_content_t *content = NULL;
    int result = _create_content(name, content, payload, payload_len);
    printf("result is %d\n", result);
    assert_int_equal(result, 0);
    assert_non_null(content);

    result = ccnl_cs_add(&content_store, name, content);
    //int result = ccnl_cs_add(&content_store, name, content);
    /* if we pass an invalid ccnl_cs_ops_t array, call should fail */
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** also the item should be in the content store */
//    result = ccnl_cs_exists(&content_store, name, content);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 1;
    assert_int_equal(expected_size, size); 
    
    /** remove the previously added element from the content store */
    //result = ccnl_cs_remove(&content_store, name);
    //assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    /** clear the content store, todo: move to teardown function */
    //ccnl_cs_clear(&content_store);

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
    /** add a single element and call clear */

    /** pass invalid parameter to the ccnl_cs_clear function */
    int result = ccnl_cs_clear(&content_store);
    assert_int_equal(result, CS_OPERATION_WAS_SUCCESSFUL);

    int size = ccnl_cs_get_cs_current_size();
    int expected_size = 0;
    assert_int_equal(expected_size, size); 


    /** add multiple elements and call clear */
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

void test_ccnl_cs_get_cs_capacity_successful(void) 
{
    size_t size = 32;
    assert_int_equal(size, ccnl_cs_get_cs_capacity());
}

size_t ccnl_cs_get_cs_current_size(void);

int main(void)
{
     const UnitTest tests[] = {
         //unit_test_setup(test_ccnl_cs_add_successful, setup),
         unit_test_setup(test_ccnl_cs_remove_non_existent_entry, setup),
         unit_test(test_ccnl_cs_add_invalid_parameters),
         unit_test(test_ccnl_cs_lookup_invalid_parameters),
         unit_test(test_ccnl_cs_remove_invalid_parameters),
         unit_test(test_ccnl_cs_clear_invalid_parameter),
         unit_test_setup(test_ccnl_cs_get_cs_capacity_successful, setup),
         unit_test(test_ccnl_cs_set_cs_capacity_successful),
         unit_test_setup(test_ccnl_cs_clear_successful, setup),
     };
    
     return run_tests(tests); 
}
