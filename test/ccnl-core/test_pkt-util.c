/**
 * @file test_pkt-util.c
 * @brief Tests for packet util functions
 *
 * Copyright (C) 2018 Safety IO
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

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "ccnl-defs.h"
#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-pkt-util.h"

#define USE_SUITE_CCNB 0x01
#define USE_SUITE_NDNTLV 0x06
#define USE_SUITE_CCNTLV 0x02
#define USE_SUITE_LOCALRPC 0x05

#define CCNL_SUITE_CCNB 0x01
#define CCNL_SUITE_CCNTLV 0x02
#define CCNL_SUITE_LOCALRPC 0x05
#define CCNL_SUITE_NDNTLV 0x06


void test_ccnl_is_suite_invalid()
{
    int invalid_suite = 0x42;
    int result = ccnl_isSuite(invalid_suite);
    assert_false(result);
}

void test_ccnl_is_suite_valid()
{
    int valid_suite = USE_SUITE_CCNB;
    int result = ccnl_isSuite(valid_suite);
    assert_true(result);

    valid_suite = USE_SUITE_CCNTLV;
    result = ccnl_isSuite(valid_suite);
    assert_true(result);

    valid_suite = USE_SUITE_LOCALRPC;
    result = ccnl_isSuite(valid_suite);
    assert_true(result);

    valid_suite = USE_SUITE_NDNTLV;
    result = ccnl_isSuite(valid_suite);
    assert_true(result);
}

void test_ccnl_suite2default_port_invalid()
{
    int invalid_suite = 0x42;
    int result = ccnl_suite2defaultPort(invalid_suite);
    /** returns by default \ref NDN_UDP_PORT */
    assert_int_equal(result, NDN_UDP_PORT);
}

void test_ccnl_suite2default_port_valid()
{
    int valid_suite = USE_SUITE_NDNTLV;
    int result = ccnl_suite2defaultPort(valid_suite);
    assert_int_equal(result, NDN_UDP_PORT);

    valid_suite = USE_SUITE_CCNB;
    result = ccnl_suite2defaultPort(valid_suite);
    assert_int_equal(result, CCN_UDP_PORT);

    valid_suite = USE_SUITE_CCNTLV;
    result = ccnl_suite2defaultPort(valid_suite);
    assert_int_equal(result, CCN_UDP_PORT);
}

void test_ccnl_suite2str_invalid()
{
    int invalid_suite = 0x42;
    const char *result = ccnl_suite2str(invalid_suite);
    assert_string_equal(result, "?");
}

void test_ccnl_suite2str_valid()
{
    int valid_suite = USE_SUITE_NDNTLV;
    const char* ndn_result = ccnl_suite2str(valid_suite);
    assert_string_equal(ndn_result, "ndn2013");

    valid_suite = USE_SUITE_CCNB;
    const char* ccnb_result = ccnl_suite2str(valid_suite);
    assert_string_equal(ccnb_result, "ccnb");

    valid_suite = USE_SUITE_CCNTLV;
    const char* ccn_result = ccnl_suite2str(valid_suite);
    assert_string_equal(ccn_result, "ccnx2015");

    valid_suite = USE_SUITE_LOCALRPC;
    const char* local_rpc_result = ccnl_suite2str(valid_suite);
    assert_string_equal(local_rpc_result, "localrpc");
}

void test_ccnl_str2suite_invalid()
{
    int result = ccnl_str2suite("invalid");
    assert_int_equal(result, -1);
}

void test_ccnl_str2suite_valid()
{
    int result = ccnl_str2suite("ccnb");
    assert_int_equal(result, CCNL_SUITE_CCNB);

    result = ccnl_str2suite("ccnx2015");
    assert_int_equal(result, CCNL_SUITE_CCNTLV);

    result = ccnl_str2suite("localrpc");
    assert_int_equal(result, CCNL_SUITE_LOCALRPC);

    result = ccnl_str2suite("ndn2013");
    assert_int_equal(result, CCNL_SUITE_NDNTLV);
}

void test_ccnl_cmp2int_invalid()
{
    int result = ccnl_cmp2int(NULL, 42);
    assert_int_equal(result, 0);
}

void test_ccnl_cmp2int_valid()
{
    int result = ccnl_cmp2int("42", 2);
    assert_int_equal(result, 42);

    /** if we pass a larger than actual size number it is still 42 */
    result = ccnl_cmp2int("42", 42);
    assert_int_equal(result, 42);

    /** if we pass a less than it is the first digit */
    result = ccnl_cmp2int("42", 1);
    assert_int_equal(result, 4);
}

void test_ccnl_pkt2suite_invalid()
{
    int result = ccnl_pkt2suite(NULL, 0, NULL);
    /** length is invalid */
    assert_int_equal(result, -1);
}

void test_ccnl_pkt2suite_valid()
{
    char buffer[2];
    buffer[0] = CCNL_SUITE_NDNTLV;
    buffer[1] = 0x01;
    int result = ccnl_pkt2suite(buffer, 2, NULL);
    assert_int_equal(result, CCNL_SUITE_NDNTLV);

    buffer[0] = 0x04;
    result = ccnl_pkt2suite(buffer, 2, NULL);
    assert_int_equal(result, CCNL_SUITE_CCNB);

    buffer[0] = CCNX_TLV_V1;
    result = ccnl_pkt2suite(buffer, 2, NULL);
    assert_int_equal(result, CCNL_SUITE_CCNTLV);
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(test_ccnl_is_suite_invalid),
        unit_test(test_ccnl_is_suite_valid),
        unit_test(test_ccnl_suite2default_port_invalid),
        unit_test(test_ccnl_suite2default_port_valid),
        unit_test(test_ccnl_suite2str_invalid),
        unit_test(test_ccnl_suite2str_valid),
        unit_test(test_ccnl_str2suite_invalid),
        unit_test(test_ccnl_str2suite_valid),
        unit_test(test_ccnl_cmp2int_invalid),
        unit_test(test_ccnl_cmp2int_valid),
        unit_test(test_ccnl_pkt2suite_invalid),
        unit_test(test_ccnl_pkt2suite_valid),
    };
    
    return run_tests(tests);
}
