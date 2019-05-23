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
#include "ccnl-cs-helper.h"

void test_ccnl_cs_create_name_successful(void **state) 
{
    const char* name = "/this/is/a/test";
    size_t length = sizeof(uint8_t) * 15;
    ccnl_cs_name_t *entry = ccnl_cs_create_name(name, length);
    assert_ptr_not_equal(entry, NULL);

    int result = memcmp(name, entry->name, length);
    assert_int_equal(entry->length, length);
    assert_int_equal(result, 0);

    free(entry);
}

void test_ccnl_cs_create_name_fails(void **state) 
{
    const char* name = "/this/is/a/test";
    size_t length = 0;
    ccnl_cs_name_t *entry = ccnl_cs_create_name(name, length);
    assert_ptr_equal(entry, NULL);

    const char* another_name = NULL;
    length = 23;
    ccnl_cs_name_t *another_entry = ccnl_cs_create_name(another_name, length);
    assert_ptr_equal(another_entry, NULL);
}

void test_ccnl_cs_create_content_successful(void **state) 
{
    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = sizeof(uint8_t) * 8;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    assert_ptr_not_equal(content, NULL);

    int result = memcmp(data, content->content, length);
    assert_int_equal(result, 0);
    assert_int_equal(content->length, length);

    free(content);
}

void test_ccnl_cs_create_content_fails(void **state) 
{
    uint8_t data[8] = { 0x43, 0x43, 0x4e, 0x2d, 0x4c, 0x49, 0x54, 0x45};
    size_t length = 0;
    ccnl_cs_content_t *content = ccnl_cs_create_content(data, length);
    assert_ptr_equal(content, NULL);

    length = 42;
    ccnl_cs_content_t *another_content = ccnl_cs_create_content(NULL, length);
    assert_ptr_equal(another_content, NULL);
}

void test_ccnl_cs_name_compare_illegal_parameters(void **state)
{
    ccnl_cs_name_t name;
    int result = ccnl_cs_name_compare(NULL, &name, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    result = ccnl_cs_name_compare(NULL, NULL, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    result = ccnl_cs_name_compare(&name, NULL, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    uint8_t unknown_mode = UINT8_MAX;
    result = ccnl_cs_name_compare(&name, &name, unknown_mode);
    assert_int_equal(result, -2);
}

//void test_ccnl_cs_name_compare(void **state)

void test_ccnl_cs_name_compare_successful(void **state)
{
    const char* name = "/this/is/a/test";
    size_t length = sizeof(uint8_t) * 15;
    ccnl_cs_name_t *first = ccnl_cs_create_name(name, length);

    const char* another_name = "/this/is/another/test";
    length = sizeof(uint8_t) * 21;
    ccnl_cs_name_t *second = ccnl_cs_create_name(another_name, length);

    int result = ccnl_cs_name_compare(first, second, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    free(first);
    free(second);
}

void test_ccnl_cs_name_compare_fail_count_mismatch(void **state)
{
    const char* name = "/this/is/a/test";
    size_t length = sizeof(uint8_t) * 15;

    ccnl_cs_name_t *first = ccnl_cs_create_name(name, length);
    ccnl_cs_name_t *second = ccnl_cs_create_name(name, length);

    first->component.count = 4;
    second->component.count = 21;

    int result = ccnl_cs_name_compare(first, second, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    free(first);
    free(second);
}

void test_ccnl_cs_name_compare_fail_chunknum_not_set(void **state)
{
    const char* name = "/this/is/a/test";
    size_t length = sizeof(uint8_t) * 15;

    ccnl_cs_name_t *first = ccnl_cs_create_name(name, length);
    ccnl_cs_name_t *second = ccnl_cs_create_name(name, length);

    first->component.count = 4;
    second->component.count = 4;

    first->chunknum = NULL;
    second->chunknum; 

    int result = ccnl_cs_name_compare(first, second, CCNL_CS_HELPER_COMPARE_EXACT);
    assert_int_equal(result, -1);

    free(first);
    free(second);
}

int main(void)
{
     const UnitTest tests[] = {
         unit_test(test_ccnl_cs_create_name_successful),
         unit_test(test_ccnl_cs_create_name_fails),
         unit_test(test_ccnl_cs_create_content_successful),
         unit_test(test_ccnl_cs_create_content_fails),
         unit_test(test_ccnl_cs_name_compare_illegal_parameters),
         unit_test(test_ccnl_cs_name_compare_successful),
         unit_test(test_ccnl_cs_name_compare_fail_count_mismatch),
         unit_test(test_ccnl_cs_name_compare_fail_chunknum_not_set),
     };
    
     return run_tests(tests); 
}
